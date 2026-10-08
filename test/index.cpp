//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "qtsupport.h"
#include "Test.h"
#include "ui/ChangedFilesModel.h"
#include "ui/DetailView.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "conf/Settings.h"
#include <QFile>
#include <QTextStream>

using namespace Test;
using namespace QTest;

static git::Index::StagedState stageState(QAbstractItemModel *model, int row) {
  return static_cast<git::Index::StagedState>(
      model->index(row, 0).data(ChangedFilesModel::StageStateRole).toInt());
}

class TestIndex : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void stageAddition();
  void stageDeletion();
  void stageDirectory();
  void cleanupTestCase();

private:
  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
};

void TestIndex::initTestCase() {
  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowActive(mWindow));
}

void TestIndex::stageAddition() {
  // Add file and refresh.
  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test." << Qt::endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);

  details->setListMode(false);

  QAbstractItemModel *unstagedModel = details->unstagedFiles();
  QCOMPARE(unstagedModel->rowCount(), 1);

  // Check that it starts unstaged.
  QCOMPARE(stageState(unstagedModel, 0), git::Index::Unstaged);

  // Stage it.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);

  QAbstractItemModel *stagedModel = details->stagedFiles();
  QCOMPARE(stagedModel->rowCount(), 1);
  QCOMPARE(unstagedModel->rowCount(), 0);

  // Check that it's staged now.
  QCOMPARE(stageState(stagedModel, 0), git::Index::Staged);

  // Commit and refresh.
  details->setCommitMessage("addition");
  view->commit();
  refresh(view, false);
}

void TestIndex::stageDeletion() {
  // Remove file and refresh.
  mRepo->workdir().remove("test");

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);


  QAbstractItemModel *unstagedModel = details->unstagedFiles();
  QCOMPARE(unstagedModel->rowCount(), 1);

  // Check that it starts unstaged.
  QCOMPARE(stageState(unstagedModel, 0), git::Index::Unstaged);

  // Stage it.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);

  QAbstractItemModel *stagedModel = details->stagedFiles();
  QCOMPARE(stagedModel->rowCount(), 1);
  QCOMPARE(unstagedModel->rowCount(), 0);

  // Check that it's staged now.
  QCOMPARE(stageState(stagedModel, 0), git::Index::Staged);

  // Commit and refresh.
  details->setCommitMessage("deletion");
  view->commit();
  refresh(view, false);
}

void TestIndex::stageDirectory() {
  QDir dir = mRepo->workdir();
  dir.mkdir("dir");
  QVERIFY(dir.cd("dir"));

  QFile file1(dir.filePath("test1"));
  QVERIFY(file1.open(QFile::WriteOnly));
  QTextStream(&file1) << "This is a test." << Qt::endl;

  QFile file2(dir.filePath("test2"));
  QVERIFY(file2.open(QFile::WriteOnly));
  QTextStream(&file2) << "This is a test." << Qt::endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);

  details->setListMode(false);

  // The directory followed by its two files.
  QAbstractItemModel *unstagedModel = details->unstagedFiles();
  QCOMPARE(unstagedModel->rowCount(), 3);
  QVERIFY(unstagedModel->index(0, 0)
              .data(ChangedFilesModel::IsDirRole)
              .toBool());

  // Check that they start unstaged.
  QCOMPARE(stageState(unstagedModel, 1), git::Index::Unstaged);
  QCOMPARE(stageState(unstagedModel, 2), git::Index::Unstaged);

  // Stage the directory.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);

  // Check for two staged files.
  QAbstractItemModel *stagedModel = details->stagedFiles();
  QCOMPARE(stagedModel->rowCount(), 3);
  QCOMPARE(unstagedModel->rowCount(), 0);

  // Check that they're staged now.
  QCOMPARE(stageState(stagedModel, 1), git::Index::Staged);
  QCOMPARE(stageState(stagedModel, 2), git::Index::Staged);
}

void TestIndex::cleanupTestCase() { mWindow->close(); }

TEST_MAIN(TestIndex)

#include "index.moc"
