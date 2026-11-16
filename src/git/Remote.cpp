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
#include "git2/remote.h"
#include <QDir>
#include <QRegularExpression>

namespace git {

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
  if (url.isEmpty() || path.isEmpty())
    return Result(-1, tr("The URL and the folder of the clone are needed."));

  // The repository doesn't exist yet, git runs in the folder above it.
  QString parent = QFileInfo(path).absolutePath();
  QDir().mkpath(parent);

  QStringList args = {"clone", "--progress"};
  if (bare)
    args.append("--bare");
  args << "--" << url << path;

  git::Cli cli(parent);
  RunResult result = cli.run(args, {}, {}, [callbacks] {
    return callbacks && callbacks->isCanceled();
  });

  // What the remote said, also when the clone failed.
  QStringList errors;
  for (QByteArray line : result.errors.split('\n')) {
    // Progress rewrites its line after '\r'. Keep what was there last.
    while (line.endsWith('\r'))
      line.chop(1);
    line = line.mid(line.lastIndexOf('\r') + 1);

    if (line.startsWith("fatal: ") || line.startsWith("error: "))
      errors.append(QString::fromUtf8(line));

    QString text = QString::fromUtf8(line.mid(8)).trimmed();
    if (callbacks && line.startsWith("remote: ") && !text.isEmpty())
      callbacks->sideband(text + '\n');
  }

  if (result.canceled)
    return Result(GIT_EUSER, tr("The clone was canceled."));
  if (result.exitCode != 0)
    return Result(-1, errors.isEmpty()
                          ? QString::fromUtf8(result.errors).trimmed()
                          : errors.join('\n'));

  // Tell the callbacks about the new references like libgit2 does.
  Repository repo = Repository::open(path);
  if (callbacks && repo.isValid()) {
    for (const Reference &ref : repo.refs()) {
      // A bare clone has the branches of the remote as its own.
      bool branch = bare ? ref.isLocalBranch() : ref.isRemoteBranch();
      if ((branch || ref.isTag()) && !ref.name().endsWith("/HEAD"))
        callbacks->update(ref.qualifiedName(), Id(), ref.targetId());
    }
  }

  return 0;
}

} // namespace git
