//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Signing.h"
#include "git2/buffer.h"
#include "git2/config.h"
#include "git2/errors.h"
#include "git2/refs.h"
#include "git2/repository.h"
#include "git2/sys/errors.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

namespace git {

namespace Signing {

namespace {

// Waiting for a passphrase can take a while.
const int kTimeout = 5 * 60 * 1000;

QString tr(const char *text) {
  return QCoreApplication::translate("Signing", text);
}

QString configString(git_repository *repo, const char *key) {
  git_config *config = nullptr;
  if (git_repository_config_snapshot(&config, repo))
    return QString();

  QString result;
  git_buf buf = GIT_BUF_INIT;
  if (!git_config_get_string_buf(&buf, config, key)) {
    result = QString::fromUtf8(buf.ptr, buf.size);
    git_buf_dispose(&buf);
  }

  git_config_free(config);
  git_error_clear();
  return result;
}

Format format(git_repository *repo) {
  QString format = configString(repo, "gpg.format").toLower();
  if (format == "ssh")
    return Ssh;
  if (format == "x509")
    return X509;
  return OpenPgp;
}

QString program(git_repository *repo, Format format) {
  QString program;
  switch (format) {
    case Ssh:
      program = configString(repo, "gpg.ssh.program");
      return program.isEmpty() ? QString("ssh-keygen") : program;
    case X509:
      program = configString(repo, "gpg.x509.program");
      return program.isEmpty() ? QString("gpgsm") : program;
    case OpenPgp:
      program = configString(repo, "gpg.openpgp.program");
      if (program.isEmpty())
        program = configString(repo, "gpg.program");
      return program.isEmpty() ? QString("gpg") : program;
  }

  return program;
}

QString expandHome(const QString &path) {
  if (path == "~" || path.startsWith("~/"))
    return QDir::homePath() + path.mid(1);
  return path;
}

bool run(const QString &program, const QStringList &args,
         const QByteArray &input, QByteArray *out, QByteArray *err,
         QString *error) {
  QProcess process;
  process.start(program, args);
  if (!process.waitForStarted()) {
    *error = tr("Unable to start %1 to sign the commit").arg(program);
    return false;
  }

  process.write(input);
  process.closeWriteChannel();
  if (!process.waitForFinished(kTimeout)) {
    process.kill();
    process.waitForFinished();
    *error = tr("%1 took too long to sign the commit").arg(program);
    return false;
  }

  *out = process.readAllStandardOutput();
  *err = process.readAllStandardError();
  if (process.exitStatus() != QProcess::NormalExit ||
      process.exitCode() != 0) {
    // Show the last message of the program.
    QStringList lines;
    for (const QString &line : QString::fromUtf8(*err).split('\n')) {
      if (!line.trimmed().isEmpty() && !line.startsWith("[GNUPG:]"))
        lines.append(line.trimmed());
    }

    QString detail = lines.isEmpty() ? QString() : lines.last();
    *error = detail.isEmpty()
                 ? tr("%1 failed to sign the commit").arg(program)
                 : tr("%1 failed to sign the commit: %2").arg(program, detail);
    return false;
  }

  return true;
}

} // namespace

bool isEnabled(git_repository *repo) {
  git_config *config = nullptr;
  if (git_repository_config_snapshot(&config, repo))
    return false;

  int enabled = 0;
  if (git_config_get_bool(&enabled, config, "commit.gpgsign"))
    enabled = 0;

  git_config_free(config);
  git_error_clear();
  return enabled;
}

QByteArray sign(git_repository *repo, const QByteArray &content,
                const git_signature *committer, QString *error) {
  Format format = Signing::format(repo);
  QString key = configString(repo, "user.signingkey").trimmed();
  QString program = Signing::program(repo, format);
  QByteArray out;
  QByteArray err;

  if (format == Ssh) {
    if (key.isEmpty()) {
      *error = tr("Set the signing key to sign commits with SSH");
      return QByteArray();
    }

    QTemporaryDir dir;
    if (!dir.isValid()) {
      *error = tr("Unable to create a temporary directory to sign the commit");
      return QByteArray();
    }

    // The key is either written literally or in a file.
    QString keyFile = expandHome(key);
    bool literal = key.startsWith("key::") || key.startsWith("ssh-");
    if (literal) {
      keyFile = dir.filePath("key.pub");
      QFile file(keyFile);
      if (!file.open(QFile::WriteOnly)) {
        *error = tr("Unable to write the signing key");
        return QByteArray();
      }
      file.write((key.startsWith("key::") ? key.mid(5) : key).toUtf8());
    }

    QString buffer = dir.filePath("buffer");
    QFile file(buffer);
    if (!file.open(QFile::WriteOnly) || file.write(content) != content.size()) {
      *error = tr("Unable to write the commit to sign it");
      return QByteArray();
    }
    file.close();

    QStringList args = {"-Y", "sign", "-n", "git", "-f", keyFile};
    if (literal)
      args.append("-U");
    args.append(buffer);

    if (!run(program, args, QByteArray(), &out, &err, error))
      return QByteArray();

    QFile signature(buffer + ".sig");
    if (!signature.open(QFile::ReadOnly)) {
      *error = tr("%1 didn't write a signature").arg(program);
      return QByteArray();
    }

    return signature.readAll();
  }

  // gpg and gpgsm sign with the committer's key by default.
  if (key.isEmpty() && committer)
    key = QString("%1 <%2>").arg(QString::fromUtf8(committer->name),
                                 QString::fromUtf8(committer->email));

  QStringList args = {"--status-fd=2", "-bsau", key};
  if (!run(program, args, content, &out, &err, error))
    return QByteArray();

  if (!err.startsWith("[GNUPG:] SIG_CREATED ") &&
      !err.contains("\n[GNUPG:] SIG_CREATED ")) {
    *error = tr("%1 failed to sign the commit").arg(program);
    return QByteArray();
  }

  return out;
}

int createCommit(git_oid *id, git_repository *repo, const char *updateRef,
                 const git_signature *author, const git_signature *committer,
                 const char *message, const git_tree *tree, size_t count,
                 const git_commit *parents[]) {
  if (!isEnabled(repo))
    return git_commit_create(id, repo, updateRef, author, committer, nullptr,
                             message, tree, count, parents);

  git_buf buf = GIT_BUF_INIT;
  if (int error = git_commit_create_buffer(&buf, repo, author, committer,
                                           nullptr, message, tree, count,
                                           parents))
    return error;

  QByteArray content(buf.ptr, buf.size);
  git_buf_dispose(&buf);

  QString error;
  QByteArray signature = sign(repo, content, committer, &error);
  if (signature.isEmpty()) {
    git_error_set_str(GIT_ERROR_INVALID, error.toUtf8().constData());
    return -1;
  }

  if (int error = git_commit_create_with_signature(
          id, repo, content.constData(), signature.constData(), nullptr))
    return error;

  if (!updateRef)
    return 0;

  // Move the reference like git_commit_create() does.
  QByteArray summary = QByteArray(message).split('\n').value(0);
  QByteArray log = (count == 0 ? QByteArray("commit (initial): ")
                    : count > 1 ? QByteArray("commit (merge): ")
                                : QByteArray("commit: ")) +
                   summary;
  return updateReference(repo, updateRef, id, log.constData());
}

int updateReference(git_repository *repo, const char *name,
                    const git_oid *id, const char *log) {
  git_reference *ref = nullptr;
  git_reference *updated = nullptr;
  int result = 0;
  if (!git_reference_lookup(&ref, repo, name) &&
      git_reference_type(ref) == GIT_REFERENCE_SYMBOLIC) {
    result = git_reference_create(&updated, repo,
                                  git_reference_symbolic_target(ref), id, 1,
                                  log);
  } else if (ref) {
    result = git_reference_set_target(&updated, ref, id, log);
  } else {
    result = git_reference_create(&updated, repo, name, id, 1, log);
  }

  git_reference_free(ref);
  git_reference_free(updated);
  return result;
}

int createRebaseCommit(git_oid *id, const git_signature *author,
                       const git_signature *committer, const char *encoding,
                       const char *message, const git_tree *tree,
                       size_t count, const git_commit *parents[],
                       void *payload) {
  Q_UNUSED(encoding)

  git_repository *repo = static_cast<git_repository *>(payload);
  if (!isEnabled(repo))
    return GIT_PASSTHROUGH;

  return createCommit(id, repo, nullptr, author, committer, message, tree,
                      count, parents);
}

QString signatureKind(git_repository *repo, const git_oid *commit) {
  git_buf signature = GIT_BUF_INIT;
  git_buf data = GIT_BUF_INIT;
  if (git_commit_extract_signature(&signature, &data, repo,
                                   const_cast<git_oid *>(commit), nullptr)) {
    git_error_clear();
    return QString();
  }

  QByteArray text(signature.ptr, signature.size);
  git_buf_dispose(&signature);
  git_buf_dispose(&data);

  if (text.contains("BEGIN SSH SIGNATURE"))
    return QStringLiteral("SSH");
  if (text.contains("BEGIN SIGNED MESSAGE"))
    return QStringLiteral("X.509");
  return QStringLiteral("GPG");
}

} // namespace Signing

} // namespace git
