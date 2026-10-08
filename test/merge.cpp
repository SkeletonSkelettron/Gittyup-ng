//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "qtsupport.h"
#include "Test.h"
#include "ui/MainWindow.h"
#include "ui/DetailView.h"
#include "ui/DiffModel.h"
#include "ui/RepoView.h"
#include "git/Patch.h"
#include <QFile>

using namespace Test;
using namespace QTest;

class TestMerge : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void firstCommit();
  void secondCommit();
  void thirdCommit();
  void mergeConflict();
  void resolve();
  void cleanupTestCase();

private:
  int inputDelay = 0;
  int closeDelay = 0;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  QString mMainBranch;
};

void TestMerge::initTestCase() {
  mMainBranch = mRepo->unbornHeadName();
  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));
}

void TestMerge::firstCommit() {
  // Add file and refresh.
  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This will be a test." << Qt::endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);

  QAbstractItemModel *model = details->unstagedFiles();
  QCOMPARE(model->rowCount(), 1);

  // Stage the file.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);

  // Commit and refresh.
  details->setCommitMessage("base commit");
  view->commit();
  refresh(view, false);
}

void TestMerge::secondCommit() {
  RepoView *view = mWindow->currentView();
  git::Branch branch = mRepo->createBranch("branch2", mRepo->head().target());
  QVERIFY(branch.isValid());

  view->checkout(branch);
  QCOMPARE(mRepo->head().name(), QString("branch2"));

  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a conflict." << Qt::endl;

  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);

  QAbstractItemModel *model = details->unstagedFiles();
  QCOMPARE(model->rowCount(), 1);

  // Stage the file.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);

  // Commit and refresh.
  details->setCommitMessage("conflicting commit b");
  view->commit();
  refresh(view, false);
}

void TestMerge::thirdCommit() {
  RepoView *view = mWindow->currentView();
  git::Reference ref =
      mRepo->lookupRef(QString("refs/heads/%1").arg(mMainBranch));
  QVERIFY(ref);

  view->checkout(ref);
  QCOMPARE(mRepo->head().name(), mMainBranch);

  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test." << Qt::endl;

  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);

  QAbstractItemModel *model = details->unstagedFiles();
  QCOMPARE(model->rowCount(), 1);

  // Stage the file.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);

  // Commit and refresh.
  details->setCommitMessage("conflicting commit a");
  view->commit();
  refresh(view, false);
}

void TestMerge::mergeConflict() {
  RepoView *view = mWindow->currentView();
  git::Reference master =
      mRepo->lookupRef(QString("refs/heads/%1").arg(mMainBranch));
  QVERIFY(master);

  git::Reference branch2 = mRepo->lookupRef("refs/heads/branch2");
  QVERIFY(branch2);

  QCOMPARE(mRepo->head().name(), mMainBranch);

  view->merge(RepoView::Merge, branch2);

  // Diff is in a conflicted state
  git::Diff diff = mRepo->diffIndexToWorkdir();
  QVERIFY(diff.isConflicted());
}

void TestMerge::resolve() {
  RepoView *view = mWindow->currentView();
  DetailView *details = view->findChild<DetailView *>();
  QVERIFY(details);

  // Wait for refresh
  QAbstractItemModel *model = details->unstagedFiles();
  qWait(1000); // Because before the merge, there is already an item in the
               // unstaged model
  while (model->rowCount() < 1)
    qWait(300);

  details->selectFile(DetailView::UnstagedFiles, 0);

  auto diff = qobject_cast<DiffModel *>(details->diffModel());
  QVERIFY(diff);
  QVERIFY(diff->isConflicted());
  QVERIFY(diff->hunkCount() > 0);

  // Theirs, undo, ours and save.
  diff->chooseConflict(0, git::Patch::Theirs);
  diff->chooseConflict(0, git::Patch::Unresolved);
  diff->chooseConflict(0, git::Patch::Ours);
  diff->saveConflict(0);

  {
    auto timeout = Timeout(10000, "Conflict wasn't resolved in time");
    while (!details->isStageEnabled())
      qWait(100);
  }
  details->stage();

  // Commit and refresh.
  details->setCommitMessage("conflicts resolved");
  view->commit();
  refresh(view, false);

  // Diff is not in a conflicted state
  git::Diff wdiff = mRepo->diffIndexToWorkdir();
  QVERIFY(!wdiff.isConflicted());
}

void TestMerge::cleanupTestCase() {
  qWait(closeDelay);
  mWindow->close();
}

TEST_MAIN(TestMerge)

#include "merge.moc"
