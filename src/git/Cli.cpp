//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Misha Sulikashvili
//

#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include "Cli.h"

namespace git {

QString Cli::gitPath() {
  static const QString path = [] {
    QString found = QStandardPaths::findExecutable("git");

#ifdef Q_OS_WIN
    if (found.isEmpty()) {
      QFileInfo checkFile("C:/Program Files/Git/cmd/git.exe");
      if (checkFile.exists())
        found = checkFile.filePath();
    }
#endif

    if (found.isEmpty())
      qCritical() << "git was not found";
    else
      qDebug() << "git found at:" << found;

    return found;
  }();

  return path;
}

Cli::Cli(const QString &repositoryPath) : mRepositoryPath(repositoryPath) {}

RunResult Cli::run(const QStringList &arguments, const QByteArray &input,
                   const QMap<QString, QString> &envs,
                   const std::function<bool()> &canceled) {
  RunResult result;
  QProcess process;
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("GIT_TERMINAL_PROMPT", "0");
  env.insert("LC_ALL", "C");
  for (const auto &[key, value] : envs.asKeyValueRange())
    env.insert(key, value);
  process.setProcessEnvironment(env);
  process.setWorkingDirectory(mRepositoryPath);
  process.start(gitPath(), arguments);
  if (!process.waitForStarted()) {
    result.errors = process.errorString().toUtf8();
    qWarning().noquote() << "could not start git:"
                         << QString::fromUtf8(result.errors).trimmed();
    return result;
  }

  if (!input.isEmpty() && process.write(input) != input.size()) {
    result.errors = process.errorString().toUtf8();
    process.kill();
    process.waitForFinished();
    return result;
  }
  process.closeWriteChannel();
  while (!process.waitForFinished(canceled ? 50 : -1) &&
         process.state() != QProcess::NotRunning) {
    if (canceled()) {
      // git removes its lock files when it's terminated, killing it doesn't.
      process.terminate();
      if (!process.waitForFinished(2000)) {
        process.kill();
        process.waitForFinished();
      }

      result.canceled = true;
      return result;
    }
  }

  if (process.exitStatus() == QProcess::CrashExit) {
    result.errors = process.errorString().toUtf8();
    qWarning().noquote() << "git" << arguments.value(0)
                         << "crashed:" << process.errorString();
    return result;
  }
  result.exitCode = process.exitCode();
  result.output = process.readAllStandardOutput();
  result.errors = process.readAllStandardError();
  if (result.exitCode != 0)
    qWarning().noquote() << "git" << arguments.value(0)
                         << "failed:" << result.exitCode
                         << QString::fromUtf8(result.errors).trimmed();
  return result;
}

bool Cli::isAvailable() { return !gitPath().isEmpty(); }
} // namespace git
