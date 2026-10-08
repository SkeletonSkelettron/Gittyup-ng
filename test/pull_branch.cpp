//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "conf/Settings.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "git/Remote.h"
#include "log/LogEntry.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QFile>

using namespace Test;
using namespace QTest;

// Pulls branches from the context menu of the sidebar: the checked out branch
// like Pull, and other branches by fast-forwarding them to their upstream.
class TestPullBranch : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void other();
  void upToDate();
  void head();
  void diverged();
  void noUpstream();
  void cleanupTestCase();

private:
  git::Commit commit(git::Repository repo, const QString &name);
  git::Commit target(const QString &branch);
  LogEntry *pull(const QString &branch);
  bool contains(LogEntry *entry, const QString &text);
  QString dump(LogEntry *entry, const QString &indent = QString());

  ScratchRepository mOrigin;
  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  RepoView *mView = nullptr;
  QString mMain;
};

void TestPullBranch::initTestCase() {
  Settings::instance()->setPrompt(Prompt::Kind::Merge, false);

  // A remote with two branches at the same commit.
  git::Commit base = commit(mOrigin, "file.txt");
  QVERIFY(base.isValid());
  mMain = mOrigin->head().name();
  QVERIFY(mOrigin->createBranch("topic", base).isValid());

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));
  mView = mWindow->currentView();

  git::Remote remote =
      mRepo->addRemote("origin", mOrigin->workdir().absolutePath());
  fetch(mView, remote);

  // Local branches that track them, with the main branch checked out.
  git::Branch main = mRepo->lookupBranch("origin/" + mMain, GIT_BRANCH_REMOTE);
  git::Branch topic = mRepo->lookupBranch("origin/topic", GIT_BRANCH_REMOTE);
  QVERIFY(mView->createBranch(mMain, main.target(), main, true).isValid());
  QVERIFY(mView->createBranch("topic", topic.target(), topic).isValid());
  QCOMPARE(mRepo->head().name(), mMain);
}

void TestPullBranch::other() {
  // The remote moves both branches ahead.
  git::Commit ahead = commit(mOrigin, "ahead.txt");
  git::Branch remoteTopic = mOrigin->lookupBranch("topic", GIT_BRANCH_LOCAL);
  QVERIFY(remoteTopic.setTarget(ahead, "test").isValid());

  // Pulling the branch that isn't checked out moves only that branch.
  git::Commit head = target(mMain);
  LogEntry *entry = pull("topic");
  QTRY_VERIFY2(target("topic").id() == ahead.id(), qPrintable(dump(entry)));
  QCOMPARE(target(mMain).id(), head.id());
  QCOMPARE(mRepo->head().name(), mMain);
  QVERIFY(!QFile::exists(mRepo->workdir().filePath("ahead.txt")));
}

void TestPullBranch::upToDate() {
  git::Commit before = target("topic");
  LogEntry *entry = pull("topic");
  QTRY_VERIFY2(contains(entry, "Already up-to-date."), qPrintable(dump(entry)));
  QCOMPARE(target("topic").id(), before.id());
}

void TestPullBranch::head() {
  // The checked out branch is pulled into the working directory.
  git::Commit ahead = mOrigin->head().target();
  LogEntry *entry = pull(mMain);
  QTRY_VERIFY2(target(mMain).id() == ahead.id(), qPrintable(dump(entry)));
  QTRY_VERIFY(QFile::exists(mRepo->workdir().filePath("ahead.txt")));
}

void TestPullBranch::diverged() {
  // The remote replaces the topic branch with a commit beside it.
  git::Commit local = target("topic");
  git::Commit base = mOrigin->lookupCommit(local.parents().first().id());
  git::Branch remoteTopic = mOrigin->lookupBranch("topic", GIT_BRANCH_LOCAL);
  QVERIFY(remoteTopic.setTarget(base, "test").isValid());
  QVERIFY(mOrigin->checkout(base, nullptr, {}, GIT_CHECKOUT_FORCE));
  QVERIFY(mOrigin->setHead(remoteTopic));
  git::Commit other = commit(mOrigin, "other.txt");
  QVERIFY(other.isValid());

  // The branch can't be fast-forwarded, and stays.
  LogEntry *entry = pull("topic");
  QTRY_VERIFY2(contains(entry, "Unable to fast-forward."),
               qPrintable(dump(entry)));
  QCOMPARE(target("topic").id(), local.id());
  QCOMPARE(mRepo->lookupBranch("origin/topic", GIT_BRANCH_REMOTE).target().id(),
           other.id());
}

void TestPullBranch::noUpstream() {
  QVERIFY(mRepo->createBranch("alone", target(mMain)).isValid());
  LogEntry *entry = pull("alone");
  QVERIFY(contains(entry, "has no upstream branch"));
}

void TestPullBranch::cleanupTestCase() { delete mWindow; }

git::Commit TestPullBranch::commit(git::Repository repo, const QString &name) {
  QFile file(repo.workdir().filePath(name));
  if (!file.open(QFile::WriteOnly))
    return git::Commit();
  file.write(name.toUtf8() + "\n");
  file.close();

  repo.index().setStaged({name}, true);
  return repo.commit(QString("Add %1").arg(name));
}

git::Commit TestPullBranch::target(const QString &branch) {
  return mRepo->lookupBranch(branch, GIT_BRANCH_LOCAL).target();
}

LogEntry *TestPullBranch::pull(const QString &branch) {
  // The root of the log is the entry that isn't in another entry.
  LogEntry *root = nullptr;
  for (LogEntry *entry : mView->findChildren<LogEntry *>()) {
    if (!qobject_cast<LogEntry *>(entry->parent()))
      root = entry;
  }

  if (!root)
    return nullptr;

  mView->pullBranch(mRepo->lookupBranch(branch, GIT_BRANCH_LOCAL));
  return root->entries().isEmpty() ? nullptr : root->entries().last();
}

bool TestPullBranch::contains(LogEntry *entry, const QString &text) {
  if (!entry)
    return false;

  if (entry->text().contains(text))
    return true;

  for (LogEntry *child : entry->entries()) {
    if (contains(child, text))
      return true;
  }

  return false;
}

QString TestPullBranch::dump(LogEntry *entry, const QString &indent) {
  if (!entry)
    return "no log entry";

  QString result =
      QString("\n%1%2 %3").arg(indent, entry->title(), entry->text());
  for (LogEntry *child : entry->entries())
    result += dump(child, indent + "  ");
  return result;
}

TEST_MAIN(TestPullBranch)

#include "pull_branch.moc"
