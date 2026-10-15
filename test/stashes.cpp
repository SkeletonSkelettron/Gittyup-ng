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
#include "log/LogEntry.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QFile>
#include <QProcess>
#include <QSignalSpy>

using namespace Test;
using namespace QTest;

// Stashes changes, and applies, pops and drops the stashes.
class TestStashes : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void stashWithMessage();
  void stashWithoutMessage();
  void stashNothing();
  void applyOlder();
  void applyConflict();
  void popConflict();
  void pop();
  void dropMissing();
  void dropLast();
  void viewReportsOnce();
  void cleanupTestCase();

private:
  void write(const QString &name, const QByteArray &content);
  QByteArray read(const QString &name);
  void commit(const QString &name, const QByteArray &content);
  void resetHard();
  LogEntry *logRoot() const;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
};

void TestStashes::initTestCase() {
  // The signals name the type of their argument without its namespace.
  qRegisterMetaType<git::Reference>("Reference");

  commit("a.txt", "a\n");
  commit("b.txt", "b\n");
}

void TestStashes::stashWithMessage() {
  write("a.txt", "a stashed\n");

  QSignalSpy updated(mRepo->notifier(),
                     &git::RepositoryNotifier::referenceUpdated);
  git::Commit stash = mRepo->stash("my message");

  QVERIFY(stash.isValid());
  QVERIFY(stash.summary().contains("my message"));
  QCOMPARE(read("a.txt"), QByteArray("a\n"));
  QCOMPARE(mRepo->stashes().size(), 1);
  QCOMPARE(updated.count(), 1);
}

void TestStashes::stashWithoutMessage() {
  write("b.txt", "b stashed\n");

  git::Commit stash = mRepo->stash();
  QVERIFY(stash.isValid());
  QVERIFY(stash.summary().startsWith("WIP on"));
  QCOMPARE(mRepo->stashes().size(), 2);
}

// Without changes, nothing is stashed, and the reason is reported.
void TestStashes::stashNothing() {
  QSignalSpy failed(mRepo->notifier(),
                    &git::RepositoryNotifier::gitCommandFailed);

  QVERIFY(!mRepo->stash("nothing").isValid());
  QCOMPARE(mRepo->stashes().size(), 2);
  QCOMPARE(failed.count(), 1);
  QVERIFY(failed.first().at(1).toString().contains("No local changes"));
}

// Applying the older stash applies only its changes, and keeps it.
void TestStashes::applyOlder() {
  QVERIFY(mRepo->applyStash(1));
  QCOMPARE(read("a.txt"), QByteArray("a stashed\n"));
  QCOMPARE(read("b.txt"), QByteArray("b\n"));
  QCOMPARE(mRepo->stashes().size(), 2);

  resetHard();
}

// A conflict applies the stash with the conflict, and keeps the stash.
void TestStashes::applyConflict() {
  commit("a.txt", "a changed\n");

  QSignalSpy failed(mRepo->notifier(),
                    &git::RepositoryNotifier::gitCommandFailed);
  QVERIFY(!mRepo->applyStash(1));
  QCOMPARE(failed.count(), 1);
  QVERIFY(failed.first().at(1).toString().contains("CONFLICT"));

  mRepo->index().read();
  QVERIFY(mRepo->index().hasConflicts());
  QCOMPARE(mRepo->stashes().size(), 2);

  resetHard();
}

// Popping with a conflict keeps the stash, like applying it.
void TestStashes::popConflict() {
  QSignalSpy failed(mRepo->notifier(),
                    &git::RepositoryNotifier::gitCommandFailed);
  QVERIFY(!mRepo->popStash(1));
  QCOMPARE(failed.count(), 1);
  QVERIFY(failed.first().at(1).toString().contains("kept"));
  QCOMPARE(mRepo->stashes().size(), 2);

  resetHard();
}

void TestStashes::pop() {
  QSignalSpy updated(mRepo->notifier(),
                     &git::RepositoryNotifier::referenceUpdated);
  QVERIFY(mRepo->popStash(0));

  QCOMPARE(read("b.txt"), QByteArray("b stashed\n"));
  QCOMPARE(mRepo->stashes().size(), 1);
  QCOMPARE(updated.count(), 1);

  resetHard();
}

void TestStashes::dropMissing() {
  QSignalSpy failed(mRepo->notifier(),
                    &git::RepositoryNotifier::gitCommandFailed);
  QVERIFY(!mRepo->dropStash(5));
  QCOMPARE(failed.count(), 1);
  QVERIFY(!failed.first().at(1).toString().isEmpty());
  QCOMPARE(mRepo->stashes().size(), 1);
}

// Dropping the last stash removes the stash reference.
void TestStashes::dropLast() {
  QVERIFY(mRepo->dropStash(0));
  QVERIFY(mRepo->stashes().isEmpty());
  QVERIFY(!mRepo->stashRef().isValid());
}

// The view shows a stash that can't be made, and a pop with a conflict, as
// one error each.
void TestStashes::viewReportsOnce() {
  write("a.txt", "a stashed again\n");
  QVERIFY(mRepo->stash("again").isValid());
  commit("a.txt", "a changed again\n");

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));
  RepoView *view = mWindow->currentView();

  QSignalSpy errors(logRoot(), &LogEntry::errorInserted);
  view->stash("nothing");
  QCOMPARE(errors.count(), 1);

  view->popStash(0);
  QCOMPARE(errors.count(), 2);
  QCOMPARE(mRepo->stashes().size(), 1);
}

void TestStashes::cleanupTestCase() {
  delete mWindow;
  mWindow = nullptr;
}

void TestStashes::write(const QString &name, const QByteArray &content) {
  QFile file(mRepo->workdir().filePath(name));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write(content);
}

QByteArray TestStashes::read(const QString &name) {
  QFile file(mRepo->workdir().filePath(name));
  return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
}

void TestStashes::commit(const QString &name, const QByteArray &content) {
  write(name, content);

  // Read the index that git changed, as a refresh of the view would.
  mRepo->index().read();
  mRepo->index().setStaged({name}, true);
  QVERIFY(mRepo->commit(QString("Change %1").arg(name)).isValid());
}

// Throw away the changes and conflicts that a stash left.
void TestStashes::resetHard() {
  QProcess git;
  git.setWorkingDirectory(mRepo->workdir().path());
  git.start("git", {"reset", "-q", "--hard"});
  QVERIFY(git.waitForFinished());
  QCOMPARE(git.exitCode(), 0);

  mRepo->index().read();
}

// The root of the activity log, which its model owns.
LogEntry *TestStashes::logRoot() const {
  RepoView *view = mWindow->currentView();
  for (LogEntry *entry : view->findChildren<LogEntry *>()) {
    if (!entry->parentEntry())
      return entry;
  }

  return nullptr;
}

TEST_MAIN(TestStashes)

#include "stashes.moc"
