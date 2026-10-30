//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Remote.h"
#include "Branch.h"
#include "Cli.h"
#include "Config.h"
#include "Id.h"
#include "qtsupport.h"
#include "TagRef.h"
#include "git2/clone.h"
#include "git2/remote.h"
#include "git2/sys/errors.h"
#include <libssh2.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QNetworkProxyFactory>
#include <QRegularExpression>
#include <QSettings>
#include <QTextStream>
#include <QTime>
#include <QUrl>
#include <QVector>
#include <functional>
#include <cstdint>

namespace git {

namespace {

const QString kLogKey = "remote/log";

// The default SSH identity files of OpenSSH, newer kinds first.
const QStringList kKeyKinds = {"ed25519", "ecdsa", "rsa", "dsa"};

// Paths that start with '~' or are relative are in the home directory.
QString homeFilePath(const QString &path) {
  QString result = path;
  if (result == "~" || result.startsWith("~/") || result.startsWith("~\\"))
    result = QDir::homePath() + result.mid(1);
  return QFileInfo(QDir::home(), result).absoluteFilePath();
}

static QRegularExpression urlRegex{"^[0-9a-zA-Z+-]+://"};
static QRegularExpression sshRegex{
    "^(([^:@/\\\\]+)@)?([0-9a-zA-Z+.-]{2,}):(.*)$"};
QUrl parseUrl(const QString &url) {
  QUrl res;

  if (urlRegex.match(url).hasMatch()) {
    res = QUrl(url);
  } else {
    QRegularExpressionMatch matches = sshRegex.match(url);
    if (matches.hasMatch()) {
      res.setScheme("ssh");

      res.setUserName(matches.captured(2));
      res.setHost(matches.captured(3));

      QString path = matches.captured(4);
      if (!path.startsWith('/')) {
        path.push_front('/');
      }

      res.setPath(path);
    } else {
      res = QUrl::fromLocalFile(url);
    }
  }

  return res;
}

class ConfigFile {
public:
  struct Host {
    QString file;
    QString hostName;
    QString user;
    QStringList patterns;
    uint16_t port = 0;
  };

  ConfigFile(const QString &path) {
    QFileInfo file(path);
    QDir dir = QDir::home();
    if (path.isEmpty()) {
      if (!dir.cd(".ssh"))
        return;

      file.setFile(dir.absoluteFilePath("config"));
    }

    if (!file.isAbsolute())
      file.setFile(dir.absolutePath() + '/' + file.filePath());

    mFile.reset(new QFile(file.absoluteFilePath()));
    if (!mFile->open(QFile::ReadOnly))
      return;

    mValid = true;
  }

  bool isValid() const { return mValid; }

  void apply(const QString &hostname,
             std::function<void(const Host &)> handler) const {
    for (Host host : parse()) {
      if (host.patterns.empty()) {
        handler(host);
      } else {
        bool matched = false;

        for (const QString &pattern : host.patterns) {
          QRegularExpression re{
              QRegularExpression::wildcardToRegularExpression(pattern)};
          if (re.match(hostname).hasMatch()) {
            handler(host);
            matched = true;
            break;
          }
        }

        if (matched) {
          break;
        }
      }
    }
  }

private:
  bool mValid = false;
  QScopedPointer<QFile> mFile;

  QList<Host> parse() const {
    Q_ASSERT(isValid());

    QList<Host> hosts;
    QTextStream in(mFile.data());
    hosts.append(Host{});
    while (!in.atEnd()) {
      // Skip comments and empty lines.
      QString line = in.readLine().trimmed();
      if (line.isEmpty() || line.startsWith('#'))
        continue;

      // Split into keyword and arguments.
      // FIXME: Doesn't handle quoted arguments.
      QRegularExpression re("(\\s+|\\s*=\\s*)");
      QStringList words = line.split(re);
      QString keyword = words.takeFirst().toLower();
      if (keyword == "host") {
        Host host;
        host.patterns = words;
        hosts.append(host);
      } else if (keyword == "hostname") {
        if (!hosts.isEmpty() && !words.isEmpty())
          hosts.last().hostName = words.first();
      } else if (keyword == "user") {
        if (!hosts.isEmpty() && !words.isEmpty())
          hosts.last().user = words.first();
      } else if (keyword == "identityfile") {
        if (!hosts.isEmpty() && !words.isEmpty())
          hosts.last().file = homeFilePath(words.first());
      } else if (keyword == "port") {
        if (!hosts.isEmpty() && !words.isEmpty() && hosts.last().port == 0) {
          bool ok;
          uint16_t port = words.first().toUShort(&ok);
          if (ok) {
            hosts.last().port = port;
          }
        }
      }
    }

    return hosts;
  }
};

// The SSH identity files to try for the host of 'url', like OpenSSH: the
// files of the SSH config file, the file of the settings and the default
// files, of those that exist.
QStringList identityFiles(const QString &url, const QString &configFilePath,
                          const QString &keyFilePath) {
  QStringList files;
  ConfigFile configFile(configFilePath);
  if (configFile.isValid()) {
    // Extract hostname from the unresolved URL.
    configFile.apply(parseUrl(url).host(),
                     [&files](const ConfigFile::Host &host) {
                       if (!host.file.isEmpty())
                         files.append(host.file);
                     });
  }

  if (!keyFilePath.isEmpty())
    files.append(homeFilePath(keyFilePath));

  for (const QString &kind : kKeyKinds)
    files.append(homeFilePath(QString("~/.ssh/id_%1").arg(kind)));

  QStringList result;
  for (const QString &file : files) {
    if (QFileInfo(file).isFile() && !result.contains(file))
      result.append(file);
  }

  return result;
}

} // namespace

#ifndef USE_SYSTEM_LIBGIT2
void Remote::Callbacks::connected(git_remote *remote, void *payload) {
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  cbs->mRemote = remote;
}

void Remote::Callbacks::about_to_disconnect(git_remote *remote, void *payload) {
  Q_UNUSED(remote)

  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  cbs->mRemote = nullptr;
}
#endif

int Remote::Callbacks::sideband(const char *str, int len, void *payload) {
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  cbs->sideband(QString::fromUtf8(str, len));
  return 0;
}

static void interactiveCallback(const char *name, int name_len,
                                const char *instruction, int instruction_len,
                                int num_prompts,
                                const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                                LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                                void **abstract) {
  if (num_prompts == 0)
    return;

  Remote::Callbacks *cbs = (Remote::Callbacks *)*abstract;

  QVector<Remote::SshInteractivePrompt> promptsVector(num_prompts);
  QVector<QString> responsesVector(num_prompts);

  for (int i = 0; i < num_prompts; ++i) {
    promptsVector[i] = {
        QString::fromUtf8((const char *)prompts[i].text, prompts[i].length),
        (bool)prompts[i].echo};
  }

  cbs->interactiveAuth(QString::fromUtf8(name, name_len),
                       QString::fromUtf8(instruction, instruction_len),
                       promptsVector, responsesVector);

  if (responsesVector.length() < num_prompts)
    responsesVector.resize(num_prompts);

  for (int i = 0; i < num_prompts; ++i) {
    if (responsesVector[i].isEmpty()) {
      responses[i].text = nullptr;
      responses[i].length = 0;

    } else {
      // The newline is also sent by OpenSSH and needed for some
      // keyboard-interactive prompts
      QByteArray bytes = (responsesVector[i] + "\n").toUtf8();

      // Use malloc and copy the response data to it
      // libssh2 free()s this memory by itself
      responses[i].length = bytes.size() - 1;
      responses[i].text = (char *)malloc(responses[i].length);

      memcpy(responses[i].text, bytes.data(), responses[i].length);
    }
  }
}

int Remote::Callbacks::credentials(git_credential **out, const char *url,
                                   const char *name, unsigned int types,
                                   void *payload) {
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  if (types & GIT_CREDENTIAL_SSH_KEY) {
    const git_error *error = git_error_last();

    // First try to get key from agent.
    if (!cbs->mAgentNames.contains(name)) {
      log(QString("agent: %1").arg(name));
      cbs->mAgentNames.insert(name);
      if (cbs->connectToAgent())
        return git_credential_ssh_key_from_agent(out, name);

    } else if (error) {
      log(error->message);
    }

    // Then try the identity files one after another, like OpenSSH.
    QStringList keys =
        identityFiles(cbs->url(), cbs->configFilePath(), cbs->keyFilePath());
    for (const QString &key : keys) {
      if (cbs->mKeyFiles.contains(key))
        continue;

      cbs->mKeyFiles.append(key);
      log(QString("identity file: %1").arg(key));

      QString pub = QString("%1.pub").arg(key);
      if (!QFile::exists(pub))
        pub = "";

      // Check if the private key is encrypted.
      QFile file(key);
      if (!file.open(QFile::ReadOnly))
        continue;

      QTextStream in(&file);
      in.readLine(); // -----BEGIN PRIVATE KEY-----
      QString line = in.readLine();
      if (!line.startsWith("Proc-Type:") || !line.endsWith("ENCRYPTED")) {
        QByteArray base64 = QByteArray::fromBase64(line.toLocal8Bit());
        if (!base64.contains("aes256-ctr") || !base64.contains("bcrypt"))
          return git_credential_ssh_key_new(
              out, name,
              !pub.isEmpty() ? pub.toLocal8Bit().constData() : nullptr,
              key.toLocal8Bit(), nullptr);
      }

      // Prompt for passphrase to decrypt key.
      QString passphrase;
      if (!cbs->passphrase(url, key, passphrase))
        return -1;

      return git_credential_ssh_key_new(
          out, name, !pub.isEmpty() ? pub.toLocal8Bit().constData() : nullptr,
          key.toLocal8Bit(), passphrase.toUtf8());
    }
  }

  // Say which keys the server didn't accept, rather than only that the
  // password or the keyboard-interactive login failed.
  QString keysError;
  if (types & GIT_CREDENTIAL_SSH_KEY) {
    QStringList keys;
    for (const QString &key : cbs->mKeyFiles)
      keys.append(QDir::toNativeSeparators(key));

    keysError = keys.isEmpty()
                    ? QString("no SSH key was found in %1")
                          .arg(QDir::toNativeSeparators(homeFilePath("~/.ssh")))
                    : QString("the server didn't accept the SSH key %1")
                          .arg(keys.join(", "));
    log(keysError);
  }

  if (types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
    QString password;
    QString username = QUrl::fromPercentEncoding(name);
    if (cbs->credentials(url, username, password)) {
      return git_credential_userpass_plaintext_new(out, username.toUtf8(),
                                                   password.toUtf8());
    }

    if (!keysError.isEmpty())
      git_error_set_str(GIT_ERROR_SSH, keysError.toUtf8());
    return -1;
  }

  if (types & GIT_CREDENTIAL_SSH_INTERACTIVE) {
    return git_credential_ssh_interactive_new(out, name, interactiveCallback,
                                              cbs);
  }

  if (!keysError.isEmpty())
    git_error_set_str(GIT_ERROR_SSH, keysError.toUtf8());
  return -1;
}

int Remote::Callbacks::certificate(git_cert *cert, int valid, const char *host,
                                   void *payload) {
  Q_UNUSED(host)

  if (valid || cert->cert_type == GIT_CERT_HOSTKEY_LIBSSH2)
    return 0;

  int noVerify = 0;
  QByteArray env = qgetenv("GIT_SSL_NO_VERIFY");
  if (!env.isNull() && !git_config_parse_bool(&noVerify, env) && noVerify)
    return 0;

  Repository repo = reinterpret_cast<Remote::Callbacks *>(payload)->repo();
  Config config = repo.isValid() ? repo.gitConfig() : Config::global();
  if (!config.value<bool>("http.sslVerify", true))
    return 0;

  git_error_set_str(GIT_ERROR_SSL, "invalid certificate");
  return -GIT_ERROR_SSL;
}

int Remote::Callbacks::transfer(const git_indexer_progress *stats,
                                void *payload) {
  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  switch (cbs->state()) {
    case Transfer:
      return cbs->transfer(stats->total_objects, stats->received_objects,
                           stats->received_bytes)
                 ? 0
                 : -1;

    case Resolve:
      return cbs->resolve(stats->total_deltas, stats->indexed_deltas) ? 0 : -1;

    case Update:
      return 0;
  }
  return 0;
}

int Remote::Callbacks::update(const char *name, const git_oid *a,
                              const git_oid *b, void *payload) {
  reinterpret_cast<Remote::Callbacks *>(payload)->update(name, a, b);
  return 0;
}

int Remote::Callbacks::remoteReady(git_remote *remote, int direction,
                                   void *payload) {
  Q_UNUSED(direction)

  Remote::Callbacks *cbs = reinterpret_cast<Remote::Callbacks *>(payload);
  QString resolved(git_remote_url(remote));
  if (!cbs->url(resolved))
    return -1;

  QUrl resolvedUrl = parseUrl(resolved);

  // Find matching config entry.
  if (resolvedUrl.scheme().compare("ssh", Qt::CaseInsensitive) == 0) {
    ConfigFile configFile(cbs->configFilePath());
    if (configFile.isValid()) {
      bool explicitPort = resolvedUrl.port() >= 0;
      bool explicitUser = !resolvedUrl.userName().isEmpty();

      configFile.apply(resolvedUrl.host(),
                       [&resolvedUrl, explicitPort,
                        explicitUser](const ConfigFile::Host &host) {
                         if (!host.hostName.isEmpty()) {
                           resolvedUrl.setHost(host.hostName);
                         }

                         if (host.port != 0 && !explicitPort) {
                           resolvedUrl.setPort(host.port);
                         }

                         if (!host.user.isEmpty() && !explicitUser) {
                           resolvedUrl.setUserName(host.user);
                         }
                       });
    }
  }

  resolved = resolvedUrl.toString();
  git_remote_set_instance_url(remote, resolved.toUtf8());
  return 0;
}

void Remote::Callbacks::stop() {
#ifndef USE_SYSTEM_LIBGIT2
  if (mRemote)
    git_remote_stop(mRemote);
#endif
}

Remote::Remote() {}

Remote::Remote(git_remote *remote) : d(remote, git_remote_free) {}

QString Remote::name() const {
  QString name = git_remote_name(d.data());
  return !name.isEmpty() ? name : url();
}

void Remote::setName(const QString &name) {
  Repository repo(git_remote_owner(d.data()));

  QStringList args = {"remote", "rename", this->name(), name};

  git::Cli cli(repo.workdir().path());
  RunResult result = cli.run(args);

  if (result.exitCode == 0)
    d.clear();
  else
    emit repo.notifier()->gitCommandFailed(tr("rename remote"),
                                           QString::fromUtf8(result.errors));
}

QString Remote::url() const { return git_remote_url(d.data()); }

void Remote::setUrl(const QString &url) {
  Repository repo(git_remote_owner(d.data()));

  QStringList args = {"remote", "set-url", name(), url};

  git::Cli cli(repo.workdir().path());
  RunResult result = cli.run(args);

  if (result.exitCode != 0)
    emit repo.notifier()->gitCommandFailed(tr("change remote URL"),
                                           QString::fromUtf8(result.errors));
}

Result Remote::fetch(Callbacks *callbacks, bool tags, bool prune) {
  Repository repo(git_remote_owner(d.data()));

  QStringList args = {"fetch", "--porcelain"};

  if (prune)
    args.append("--prune");

  if (tags)
    args.append("--tags");
  args.append(name());

  // git stops when the fetch is canceled, before it changes references.
  git::Cli cli(repo.workdir().path());
  RunResult result = cli.run(args, {}, {}, [callbacks] {
    return callbacks && callbacks->isCanceled();
  });

  if (result.canceled)
    return Result(GIT_EUSER, tr("The fetch was canceled."));
  if (result.exitCode != 0)
    return Result(-1, QString::fromUtf8(result.errors).trimmed());

  // Tell the callbacks about the updated references like libgit2 does.
  if (callbacks) {
    callbacks->transfer(0, 0, 0);
    for (const QByteArray &line : result.output.split('\n')) {
      // "<flag> <old id> <new id> <local ref>", the flag may be a space.
      if (line.size() < 2 || line.at(0) == '!')
        continue;

      QList<QByteArray> fields = line.mid(2).split(' ');
      if (fields.size() != 3)
        continue;

      git_oid_t type = repo.oidType();
      Id a(QByteArray::fromHex(fields.at(0)), type);
      Id b(QByteArray::fromHex(fields.at(1)), type);
      callbacks->update(QString::fromUtf8(fields.at(2)), a, b);
    }
  }

  return 0;
}

Result Remote::push(Callbacks *callbacks, const QStringList &refspecs) {
  Repository repo(git_remote_owner(d.data()));

  // refs/heads/x on the remote is refs/remotes/<remote>/x here.
  auto tracking = [this](const QString &ref) {
    return ref.startsWith("refs/heads/")
               ? QString("refs/remotes/%1/%2").arg(name(), ref.mid(11))
               : QString();
  };
  auto target = [&repo](const QString &name) {
    Reference ref = repo.lookupRef(name);
    return ref.isValid() ? ref.target().id() : Id();
  };

  // git moves the remote-tracking branches, remember where they were.
  QMap<QString, Id> before;
  for (QString refspec : refspecs) {
    if (refspec.startsWith('+'))
      refspec.remove(0, 1);
    QString name = tracking(refspec.section(':', -1));
    if (!name.isEmpty())
      before.insert(name, target(name));
  }

  QStringList args = {"push", "--porcelain", name()};
  args += refspecs;

  git::Cli cli(repo.workdir().path());
  RunResult result = cli.run(args);

  // What the remote said, like why a hook declined the push.
  if (callbacks) {
    for (const QByteArray &line : result.errors.split('\n')) {
      if (line.startsWith("remote: ") && !line.mid(8).trimmed().isEmpty())
        callbacks->sideband(QString::fromUtf8(line.mid(8)) + '\n');
    }

    callbacks->transfer(0, 0, 0);
  }

  // Tell the callbacks about the updated references like libgit2 does.
  bool rejected = false;
  QString notFastForward;
  for (const QByteArray &line : result.output.split('\n')) {
    // "<flag>\t<src>:<dst>\t<summary>", the flag may be a space.
    QList<QByteArray> fields = line.split('\t');
    if (fields.size() != 3 || fields.at(0).isEmpty())
      continue;

    QString dst = QString::fromUtf8(fields.at(1)).section(':', -1);
    if (fields.at(0).at(0) == '!') {
      // "[rejected] (fetch first)" or "[remote rejected] (<reason>)".
      QString summary = QString::fromUtf8(fields.at(2));
      QRegularExpressionMatch match =
          QRegularExpression("\\((.*)\\)$").match(summary);
      QString reason = match.hasMatch() ? match.captured(1) : summary;

      // Only forcing updates a reference that has commits we don't have.
      if (summary.startsWith("[rejected]") &&
          (reason == "non-fast-forward" || reason == "fetch first" ||
           reason == "already exists")) {
        notFastForward = reason;
        continue;
      }

      rejected = true;
      if (callbacks)
        callbacks->rejected(dst, reason);
      continue;
    }

    // Skip refs that are up to date and tags.
    QString name = tracking(dst);
    if (!callbacks || fields.at(0).at(0) == '=' || name.isEmpty())
      continue;

    callbacks->update(name, before.value(name), target(name));
  }

  if (notFastForward == "already exists")
    return Result(GIT_ENONFASTFORWARD, "The tag already exists in the remote.");
  if (!notFastForward.isEmpty())
    return Result(GIT_ENONFASTFORWARD,
                  "The remote contains commits that you don't have locally.");

  // The log lists the references that the remote rejected.
  if (rejected)
    return Result(-1, tr("The remote rejected the push."));
  if (result.exitCode != 0)
    return Result(-1, QString::fromUtf8(result.errors).trimmed());

  return 0;
}

Result Remote::push(Callbacks *callbacks, const Reference &src,
                    const QString &dst, bool force, bool tags) {
  Repository repo(git_remote_owner(d.data()));
  QString prefix = force ? "+" : QString();
  QString refspec = prefix + src.qualifiedName();
  if (!dst.isEmpty()) {
    refspec += ":" + dst;
  } else {
    // The branch of the upstream is only on the remote of the upstream.
    // Other remotes get the branch of the same name, like git pushes it.
    Config config = repo.gitConfig();
    QString key = QString("branch.%1.").arg(src.name());
    QString upstream = config.value<QString>(key + "merge");
    if (!upstream.isEmpty() && config.value<QString>(key + "remote") == name())
      refspec += ":" + upstream;
  }

  QStringList refspecs(refspec);
  if (tags) {
    // Add tags individually. Wildcard push refspecs aren't supported yet.
    for (const TagRef &tag : repo.tags())
      refspecs.append(prefix + tag.qualifiedName());
  }

  return push(callbacks, refspecs);
}

Result Remote::clone(Callbacks *callbacks, const QString &url,
                     const QString &path, bool bare) {
  git_repository *repo = nullptr;
  git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
#ifndef USE_SYSTEM_LIBGIT2
  opts.fetch_opts.callbacks.connected = &Remote::Callbacks::connected;
  opts.fetch_opts.callbacks.about_to_disconnect =
      &Remote::Callbacks::about_to_disconnect;
#endif
  opts.fetch_opts.callbacks.sideband_progress = &Remote::Callbacks::sideband;
  opts.fetch_opts.callbacks.credentials = &Remote::Callbacks::credentials;
  opts.fetch_opts.callbacks.certificate_check = &Remote::Callbacks::certificate;
  opts.fetch_opts.callbacks.transfer_progress = &Remote::Callbacks::transfer;
  opts.fetch_opts.callbacks.update_tips = &Remote::Callbacks::update;
  opts.fetch_opts.callbacks.remote_ready = &Remote::Callbacks::remoteReady;
  opts.fetch_opts.callbacks.payload = callbacks;
  opts.bare = bare;

  QByteArray proxy = proxyUrl(url, opts.fetch_opts.proxy_opts.type);
  opts.fetch_opts.proxy_opts.url = proxy;

  return git_clone(&repo, url.toUtf8(), path.toUtf8(), &opts);
}

QByteArray Remote::proxyUrl(const QString &url, git_proxy_t &type) {
  type = GIT_PROXY_AUTO;

  QUrl tmp(url);
  QNetworkProxyQuery query(tmp);
  QList<QNetworkProxy> proxies =
      QNetworkProxyFactory::systemProxyForQuery(query);
  if (proxies.isEmpty())
    return QByteArray();

  QNetworkProxy proxy = proxies.first();
  if (proxy.type() == QNetworkProxy::NoProxy)
    type = GIT_PROXY_NONE;

  QString host = proxy.hostName();
  if (host.isEmpty())
    return QByteArray();

  type = GIT_PROXY_SPECIFIED;
  return QString("http://%1:%2").arg(host).arg(proxy.port()).toUtf8();
}

bool Remote::isLoggingEnabled() { return QSettings().value(kLogKey).toBool(); }

void Remote::setLoggingEnabled(bool enable) {
  QSettings().setValue(kLogKey, enable);
}

void Remote::log(const QString &text) {
  if (!isLoggingEnabled())
    return;

  QString name = QCoreApplication::applicationName();
  QDir tempDir = QDir::temp();
  tempDir.mkpath(name);
  tempDir.cd(name);

  QFile file(tempDir.filePath("remote.log"));
  if (!file.open(QFile::WriteOnly | QIODevice::Append))
    return;

  QString time = QTime::currentTime().toString(Qt::ISODateWithMs);
  QTextStream(&file) << time << " - " << text << Qt::endl;
}

} // namespace git
