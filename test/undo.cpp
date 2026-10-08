//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "ui/DetailView.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/UndoHistory.h"
#include <QFile>

using namespace Test;
using namespace QTest;

// Undoes and redoes the actions of a repository like GitKraken.
class TestUndo : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void commit();
  void branches();
  void checkout();
  void reset();
  void localChanges();
  void cleanupTestCase();

private:
  void write(const QString &name, const QByteArray &content);
  QByteArray read(const QString &name);
  git::Id head();

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  RepoView *mView = nullptr;
  UndoHistory *mHistory = nullptr;
  git::Id mFirst;
  QString mMain;
};

void TestUndo::initTestCase() {
  write("file.txt", "content\n");
  mRepo->index().setStaged({"file.txt"}, true);
  QVERIFY(mRepo->commit("initial"));
  mFirst = head();
  mMain = mRepo->head().name();

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));

  mView = mWindow->currentView();
  mHistory = mView->undoHistory();
  QVERIFY(!mHistory->canUndo());
  QVERIFY(!mHistory->canRedo());
}

void TestUndo::commit() {
  write("a.txt", "one\n");
  mRepo->index().setStaged({"a.txt"}, true);
  QVERIFY(mView->commit(QString("Add a")));
  git::Id second = head();
  QVERIFY(second != mFirst);

  mHistory->check();
  QVERIFY(mHistory->canUndo());
  QVERIFY(mHistory->undoText().startsWith("Commit"));
  QVERIFY(mHistory->undoText().contains("Add a"));

  // Undoing a commit keeps its changes staged and offers its message.
  QVERIFY(mHistory->undo());
  QCOMPARE(head(), mFirst);
  QCOMPARE(mRepo->index().isStaged("a.txt"), git::Index::Staged);
  QCOMPARE(mView->detailView()->commitMessage(), QString("Add a"));
  QVERIFY(!mHistory->canUndo());
  QVERIFY(mHistory->canRedo());

  QVERIFY(mHistory->redo());
  QCOMPARE(head(), second);
  QCOMPARE(mView->detailView()->commitMessage(), QString());
  QVERIFY(mHistory->canUndo());
  QVERIFY(!mHistory->canRedo());
}

void TestUndo::branches() {
  git::Commit commit = mRepo->lookupCommit(mFirst);

  // Create.
  mView->createBranch("feature", commit);
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Create Branch feature"));
  QVERIFY(mHistory->undo());
  QVERIFY(!mRepo->lookupBranch("feature").isValid());
  QVERIFY(mHistory->redo());
  QVERIFY(mRepo->lookupBranch("feature").isValid());

  // Delete restores the branch and its upstream.
  git::Config config = mRepo->gitConfig();
  config.setValue("branch.feature.remote", QString("origin"));
  config.setValue("branch.feature.merge", QString("refs/heads/feature"));
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Create Branch feature"));
  mRepo->lookupBranch("feature").remove();
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Delete Branch feature"));
  QVERIFY(mHistory->undo());
  git::Branch branch = mRepo->lookupBranch("feature");
  QVERIFY(branch.isValid());
  QCOMPARE(branch.target().id(), mFirst);
  QCOMPARE(mRepo->gitConfig().value<QString>("branch.feature.remote"),
           QString("origin"));
  QCOMPARE(mRepo->gitConfig().value<QString>("branch.feature.merge"),
           QString("refs/heads/feature"));

  // Rename.
  branch.rename("renamed");
  mHistory->check();
  QCOMPARE(mHistory->undoText(),
           QString("Rename Branch feature to renamed"));
  QVERIFY(mHistory->undo());
  QVERIFY(mRepo->lookupBranch("feature").isValid());
  QVERIFY(!mRepo->lookupBranch("renamed").isValid());
}

void TestUndo::checkout() {
  git::Id second = head();
  mView->checkout(mRepo->lookupBranch("feature"));
  QCOMPARE(mRepo->head().name(), QString("feature"));
  QVERIFY(!QFile::exists(mRepo->workdir().filePath("a.txt")));

  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Checkout feature"));
  QVERIFY(mHistory->undo());
  QCOMPARE(mRepo->head().name(), mMain);
  QCOMPARE(head(), second);
  QCOMPARE(read("a.txt"), QByteArray("one\n"));

  QVERIFY(mHistory->redo());
  QCOMPARE(mRepo->head().name(), QString("feature"));
  QVERIFY(mHistory->undo());
  QCOMPARE(mRepo->head().name(), mMain);
}

void TestUndo::reset() {
  git::Id second = head();
  git::Commit first = mRepo->lookupCommit(mFirst);

  // Hard reset changes the working directory back.
  mView->reset(first, GIT_RESET_HARD);
  QCOMPARE(head(), mFirst);
  QVERIFY(!QFile::exists(mRepo->workdir().filePath("a.txt")));
  mHistory->check();
  QVERIFY(mHistory->undoText().startsWith("Reset"));
  QVERIFY(mHistory->undo());
  QCOMPARE(head(), second);
  QCOMPARE(read("a.txt"), QByteArray("one\n"));

  // Soft reset keeps the working directory and the index.
  mView->reset(first, GIT_RESET_SOFT);
  QCOMPARE(mRepo->index().isStaged("a.txt"), git::Index::Staged);
  QVERIFY(mHistory->undo());
  QCOMPARE(head(), second);
  git::Tree tree = mRepo->lookupCommit(second).tree();
  QCOMPARE(mRepo->diffTreeToIndex(tree).count(), 0);

  // Mixed reset resets the index back too.
  mView->reset(first, GIT_RESET_MIXED);
  QVERIFY(mHistory->undo());
  QCOMPARE(head(), second);
  QCOMPARE(mRepo->diffTreeToIndex(tree).count(), 0);
  QCOMPARE(read("a.txt"), QByteArray("one\n"));
}

void TestUndo::localChanges() {
  git::Id second = head();

  // Fast-forward from the first commit.
  git::Commit first = mRepo->lookupCommit(mFirst);
  mView->createBranch("ahead", mRepo->lookupCommit(second));
  mView->reset(first, GIT_RESET_HARD);
  mView->merge(RepoView::FastForward, mRepo->lookupBranch("ahead"));
  QCOMPARE(head(), second);
  mHistory->check();
  QVERIFY(mHistory->undoText().startsWith("Fast-forward"));

  // Undo doesn't overwrite local changes.
  write("a.txt", "local\n");
  QVERIFY(!mHistory->undo());
  QCOMPARE(head(), second);
  QCOMPARE(read("a.txt"), QByteArray("local\n"));

  write("a.txt", "one\n");
  QVERIFY(mHistory->undo());
  QCOMPARE(head(), mFirst);
  QVERIFY(!QFile::exists(mRepo->workdir().filePath("a.txt")));

  // A new action forgets the actions that were undone.
  QVERIFY(mHistory->canRedo());
  mView->createBranch("other", first);
  mHistory->check();
  QVERIFY(!mHistory->canRedo());
}

void TestUndo::cleanupTestCase() {
  delete mWindow;
  mWindow = nullptr;
}

void TestUndo::write(const QString &name, const QByteArray &content) {
  QFile file(mRepo->workdir().filePath(name));
  QVERIFY(file.open(QFile::WriteOnly | QFile::Truncate));
  file.write(content);
}

QByteArray TestUndo::read(const QString &name) {
  QFile file(mRepo->workdir().filePath(name));
  return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
}

git::Id TestUndo::head() { return mRepo->head().target().id(); }

TEST_MAIN(TestUndo)

#include "undo.moc"
