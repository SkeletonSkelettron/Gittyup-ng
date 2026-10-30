//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Claude Opus 5.5, an AI model by Anthropic
//

#include "Test.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Remote.h"
#include "log/LogEntry.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QFile>
#include <QProcess>

using namespace Test;
using namespace QTest;

namespace {

QString runGit(const QString &dir, const QStringList &args) {
  QProcess process;
  process.setWorkingDirectory(dir);
  process.start("git", QStringList{"-c", "user.name=Test", "-c",
                                   "user.email=test@example.com"} +
                           args);
  process.waitForFinished(-1);
  return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

// Commit a new file in the repository at 'dir'.
void commit(const QString &dir, const QString &name) {
  QFile file(dir + "/" + name);
  QVERIFY(file.open(QFile::WriteOnly));
  file.write(name.toUtf8() + "\n");
  file.close();
  runGit(dir, {"add", name});
  runGit(dir, {"commit", "-qm", "Add " + name});
}

void script(const QString &path, const QByteArray &content) {
  QFile file(path);
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("#!/bin/sh\n" + content);
  file.close();
  QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner |
                              QFile::ExeOwner));
}

// The entry that the view logged last.
LogEntry *lastEntry(RepoView *view) {
  for (LogEntry *entry : view->findChildren<LogEntry *>()) {
    if (!entry->parentEntry())
      return entry->lastEntry();
  }
  return nullptr;
}

bool contains(LogEntry *entry, const QString &text) {
  if (entry->text().contains(text))
    return true;
  for (LogEntry *child : entry->entries()) {
    if (contains(child, text))
      return true;
  }
  return false;
}

} // namespace

// Pushes to remotes: the log tells what the remote did.
class TestTransfer : public QObject {
  Q_OBJECT

private slots:
  void pushStaleTracking();
  void pushRejected();
};

// The remote-tracking branch may be old: another process may have deleted
// the branch of the remote since. Pushing pushes it again.
void TestTransfer::pushStaleTracking() {
  ScratchRepository repo;
  QString dir = repo->workdir().path();
  commit(dir, "file.txt");
  QString main = repo->head().name();

  QTemporaryDir remoteDir;
  runGit(remoteDir.path(), {"init", "-q", "--bare"});
  git::Remote remote = repo->addRemote("origin", remoteDir.path());
  runGit(dir, {"push", "-q", "-u", "origin", main});
  QVERIFY(git::Branch(repo->head()).upstream().isValid());
  runGit(remoteDir.path(), {"update-ref", "-d", "refs/heads/" + main});

  // A fetch of the window would run before.
  repo->appConfig().setValue("autofetch.enable", false);
  MainWindow window(repo);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));
  RepoView *view = window.currentView();
  view->push(remote);
  LogEntry *entry = lastEntry(view);
  QVERIFY(entry);
  QTRY_VERIFY(entry->progress() < 0);
  QVERIFY(!contains(entry, "Everything up-to-date."));
  QCOMPARE(runGit(remoteDir.path(), {"rev-parse", "refs/heads/" + main}),
           repo->head().target().id().toString());
}

// A push that the remote rejects doesn't set the upstream of the branch.
void TestTransfer::pushRejected() {
#ifdef Q_OS_WIN
  QSKIP("The hook of the remote is a shell script");
#endif

  ScratchRepository repo;
  QString dir = repo->workdir().path();
  commit(dir, "file.txt");
  QString main = repo->head().name();

  QTemporaryDir remoteDir;
  runGit(remoteDir.path(), {"init", "-q", "--bare"});
  git::Remote remote = repo->addRemote("origin", remoteDir.path());
  runGit(dir, {"push", "-q", "origin", main});
  QVERIFY(!git::Branch(repo->head()).upstream().isValid());
  QString before =
      runGit(remoteDir.path(), {"rev-parse", "refs/heads/" + main});

  QDir(remoteDir.path()).mkpath("hooks");
  script(remoteDir.filePath("hooks/pre-receive"), "echo denied >&2\nexit 1\n");
  commit(dir, "second.txt");

  // A fetch of the window would run before.
  repo->appConfig().setValue("autofetch.enable", false);
  MainWindow window(repo);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));
  RepoView *view = window.currentView();
  view->push(remote);
  LogEntry *entry = lastEntry(view);
  QVERIFY(entry);
  QTRY_VERIFY(entry->progress() < 0);
  QVERIFY(contains(entry, "[remote rejected]"));
  QCOMPARE(runGit(remoteDir.path(), {"rev-parse", "refs/heads/" + main}),
           before);
  QVERIFY(!git::Branch(repo->head()).upstream().isValid());
}

TEST_MAIN(TestTransfer)

#include "transfer.moc"
