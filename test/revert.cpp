//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Claude Opus 5.5, an AI model by Anthropic
//

#include "Test.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "git/Reference.h"
#include "git2/repository.h"
#include "log/LogEntry.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QFile>
#include <QProcess>
#include <QSignalSpy>

using namespace Test;
using namespace QTest;

// Reverts commits that conflict and that can't be reverted, and refuses
// staged changes like cherry-picking. Reverting without a conflict is in
// CommitAuthorCommitter.
class TestRevert : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void conflictIsLeft();
  void mergeCommitFails();
  void viewShowsConflict();
  void stagedChangesAreKept();
  void stagedSubmoduleIsKept();
  void cleanupTestCase();

private:
  void commit(const QString &name, const QByteArray &content);
  QString git(const QStringList &args);
  QStringList errors(LogEntry *entry) const;
  LogEntry *logRoot() const;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  git::Commit mChange;
};

void TestRevert::initTestCase() {
  commit("a.txt", "base\n");
  commit("a.txt", "change\n");
  mChange = mRepo->head().target();

  // Change the same line again, so that reverting the change conflicts.
  commit("a.txt", "again\n");
}

// A conflict leaves the revert in progress with the conflict in the index,
// as libgit2 did, and isn't an error: the view reports the conflict.
void TestRevert::conflictIsLeft() {
  git::Id head = mRepo->head().target().id();
  QSignalSpy stateChanged(mRepo->notifier(),
                          &git::RepositoryNotifier::stateChanged);
  QSignalSpy failed(mRepo->notifier(),
                    &git::RepositoryNotifier::gitCommandFailed);

  QVERIFY(mChange.revert());
  QCOMPARE(mRepo->state(), int(GIT_REPOSITORY_STATE_REVERT));
  QVERIFY(mRepo->index().hasConflicts());
  QCOMPARE(stateChanged.count(), 1);
  QCOMPARE(failed.count(), 0);
  QCOMPARE(mRepo->head().target().id(), head);

  git({"revert", "--abort"});
  mRepo->index().read();
  QCOMPARE(mRepo->state(), int(GIT_REPOSITORY_STATE_NONE));
}

// A merge commit can't be reverted without choosing one of its parents, as
// with libgit2, and nothing changes.
void TestRevert::mergeCommitFails() {
  QString main = mRepo->head().name();
  git({"checkout", "-q", "-b", "side", "HEAD~1"});
  commit("b.txt", "side\n");
  git({"checkout", "-q", main});
  git({"merge", "-q", "--no-ff", "-m", "Merge side", "side"});

  git::Commit merge = mRepo->head().target();
  QCOMPARE(merge.parents().size(), 2);

  QSignalSpy failed(mRepo->notifier(),
                    &git::RepositoryNotifier::gitCommandFailed);
  QVERIFY(!merge.revert());
  QCOMPARE(failed.count(), 1);
  QVERIFY(failed.first().at(1).toString().contains("merge"));
  QCOMPARE(mRepo->state(), int(GIT_REPOSITORY_STATE_NONE));
  QCOMPARE(mRepo->head().target().id(), merge.id());
}

// The view shows the conflict and the help to resolve it as its only error,
// and doesn't commit.
void TestRevert::viewShowsConflict() {
  git::Id head = mRepo->head().target().id();

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));
  mWindow->currentView()->revert(mChange);

  QStringList texts = errors(logRoot());
  QCOMPARE(texts.size(), 1);
  QVERIFY(texts.first().contains("merge conflict"));
  QCOMPARE(mRepo->state(), int(GIT_REPOSITORY_STATE_REVERT));
  QCOMPARE(mRepo->head().target().id(), head);

  git({"revert", "--abort"});
  mRepo->index().read();
}

// Reverting and cherry-picking apply the commit to the index. Staged changes
// would be committed with it, and lost when it's aborted: both refuse them.
void TestRevert::stagedChangesAreKept() {
  git({"checkout", "-q", "-b", "side"});
  commit("picked.txt", "picked\n");
  git::Commit picked = mRepo->head().target();
  git({"checkout", "-q", "-"});
  commit("reverted.txt", "reverted\n");
  git::Commit reverted = mRepo->head().target();

  QFile file(mRepo->workdir().filePath("staged.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("staged\n");
  file.close();
  git({"add", "staged.txt"});
  mRepo->index().read();

  RepoView *view = mWindow->currentView();
  view->revert(reverted);
  view->cherryPick(picked);

  QCOMPARE(mRepo->state(), int(GIT_REPOSITORY_STATE_NONE));
  QCOMPARE(mRepo->head().target().id(), reverted.id());
  QVERIFY(QFile::exists(mRepo->workdir().filePath("reverted.txt")));
  QVERIFY(!QFile::exists(mRepo->workdir().filePath("picked.txt")));
  QCOMPARE(git({"diff", "--cached", "--name-only"}), QString("staged.txt"));

  QStringList texts = errors(logRoot());
  QVERIFY(texts.contains("Unable to revert commit"));
  QCOMPARE(texts.filter("You have staged changes").size(), 1);
}

// A staged submodule counts too, also when the configuration leaves
// submodules out of diffs.
void TestRevert::stagedSubmoduleIsKept() {
  // Only the submodule is staged.
  git({"reset", "-q"});
  QStringList sub = {
      "-c", "user.name=Test", "-c", "user.email=test@example.com", "-C", "sm"};
  git({"init", "-q", "sm"});
  QFile file(mRepo->workdir().filePath("sm/a.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("one\n");
  file.close();
  git(sub + QStringList{"add", "a.txt"});
  git(sub + QStringList{"commit", "-qm", "One"});
  git({"add", "sm"});
  git({"commit", "-qm", "Add sm"});
  git(sub + QStringList{"commit", "-q", "--allow-empty", "-m", "Two"});
  git({"add", "sm"});
  git({"config", "diff.ignoreSubmodules", "all"});
  mRepo->index().read();

  QVERIFY(!mRepo->stagedChangesError().isEmpty());
  QVERIFY(!mRepo->cherryPick(mRepo->lookupBranch("side").target()));
  QCOMPARE(git({"diff", "--cached", "--name-only", "--ignore-submodules=none"}),
           QString("sm"));
  git({"config", "--unset", "diff.ignoreSubmodules"});
}

void TestRevert::cleanupTestCase() {
  delete mWindow;
  mWindow = nullptr;
}

void TestRevert::commit(const QString &name, const QByteArray &content) {
  QFile file(mRepo->workdir().filePath(name));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write(content);
  file.close();

  git({"add", name});
  git({"commit", "-qm", QString("Change %1").arg(name)});
}

// Run git in the repository, for what Gittyup doesn't do itself.
QString TestRevert::git(const QStringList &args) {
  QProcess process;
  process.setWorkingDirectory(mRepo->workdir().path());
  process.start("git", args);
  process.waitForFinished();
  return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

// The texts of the errors in the log under the entry.
QStringList TestRevert::errors(LogEntry *entry) const {
  QStringList texts;
  for (LogEntry *child : entry->entries()) {
    if (child->kind() == LogEntry::Error)
      texts.append(child->text());
    texts.append(errors(child));
  }

  return texts;
}

// The root of the activity log, which its model owns.
LogEntry *TestRevert::logRoot() const {
  RepoView *view = mWindow->currentView();
  for (LogEntry *entry : view->findChildren<LogEntry *>()) {
    if (!entry->parentEntry())
      return entry;
  }

  return nullptr;
}

TEST_MAIN(TestRevert)

#include "revert.moc"
