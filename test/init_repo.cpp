//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Test.h"
#include "dialogs/AmendDialog.h"
#include "dialogs/CloneDialog.h"
#include "qnamespace.h"
#include "ui/CommitList.h"
#include "ui/DetailView.h"
#include "ui/DiffModel.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QFile>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTextEdit>
#include <QTextStream>
#include <QToolButton>
#include <qtestcase.h>

using namespace Test;
using namespace QTest;

class TestInitRepo : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void addFile();
  void commitFile();
  void amendCommit();
  void editFile();
  void cleanupTestCase();

private:
  MainWindow *mWindow = nullptr;
};

void TestInitRepo::initTestCase() {
  QDir dir = QDir::temp();
  if (dir.cd("test_init_repo"))
    QVERIFY(dir.removeRecursively());

  // Initialize a repository like the welcome page does.
  CloneDialog *cloneDialog = new CloneDialog(CloneDialog::Init);
  QObject::connect(cloneDialog, &CloneDialog::accepted, [cloneDialog] {
    MainWindow::open(cloneDialog->path());
  });
  cloneDialog->open();
  QVERIFY(qWaitForWindowExposed(cloneDialog));

  // Set fields.
  cloneDialog->setField("name", "test_init_repo");
  cloneDialog->setField("path", QDir::tempPath());

  // Initialize.
  QVERIFY(cloneDialog->canContinue());
  cloneDialog->next();

  // Wait on the new window.
  mWindow = MainWindow::activeWindow();
  QVERIFY(mWindow && qWaitForWindowExposed(mWindow));

  RepoView *view = mWindow->currentView();
  QVERIFY(view);
  git::Repository repo = view->repo();
  QVERIFY(repo.isValid());
  initRepo(repo);
}

void TestInitRepo::addFile() {
  // Create a file.
  QDir dir = QDir::temp();
  QVERIFY(dir.cd("test_init_repo"));

  QFile file(dir.filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test.";
  file.close();

  // Check for a single file called "test".
  RepoView *view = mWindow->currentView();
  DetailView *detailView = view->findChild<DetailView *>();
  QVERIFY(detailView);

  QAbstractItemModel *model = detailView->unstagedFiles();

  {
    // Wait for refresh
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (model->rowCount() < 1)
      qWait(300);
  }

  QCOMPARE(model->rowCount(), 1);
  QCOMPARE(model->data(model->index(0, 0)).toString(), QString("test"));
}

void TestInitRepo::commitFile() {
  RepoView *view = mWindow->currentView();
  DetailView *detailView = view->findChild<DetailView *>();
  QVERIFY(detailView);

  QVERIFY(detailView->isStageEnabled());
  detailView->stage();
  view->commit();
}

void TestInitRepo::amendCommit() {
  RepoView *view = mWindow->currentView();
  QVERIFY(view);

  bool finished = false;
  connect(view, &RepoView::statusChanged, [&finished]() { finished = true; });

  view->amendCommit();

  auto dialog = view->findChild<AmendDialog *>();
  QVERIFY(dialog);
  dialog->setCommitMessage("Some other commit message");
  dialog->accept();

  qWait(300);

  {
    auto timeout =
        Timeout(10000, "Repository didn't detect status change in time");
    while (!finished)
      qWait(300);
  }

  // Verify commit amended
  CommitList *commitList = view->findChild<CommitList *>();
  QVERIFY(commitList);
  QAbstractItemModel *commitModel = commitList->model();
  QModelIndex index = commitModel->index(0, 0);
  QVERIFY(index.isValid());
  auto commit = commitModel->data(index, CommitList::Role::CommitRole)
                    .value<git::Commit>();
  QCOMPARE(commit.message(), QString("Some other commit message"));
}

void TestInitRepo::editFile() {
  RepoView *view = mWindow->currentView();
  DetailView *detailView = view->findChild<DetailView *>();
  QVERIFY(detailView);

  // Select the file of the commit.
  view->selectFirstCommit();
  QAbstractItemModel *files = detailView->files();
  {
    auto timeout = Timeout(10000, "Diff didn't finish loading in time");
    while (files->rowCount() < 1)
      qWait(300);
  }

  detailView->selectFile(DetailView::AllFiles, 0);
  QCOMPARE(detailView->file(), QString("test"));

  auto diff = qobject_cast<DiffModel *>(detailView->diffModel());
  QVERIFY(diff);
  QCOMPARE(diff->path(), QString("test"));

  // Open the editor.
  detailView->editFile(detailView->file());
}

void TestInitRepo::cleanupTestCase() {
  if (mWindow) {
    mWindow->close();
  }
  QDir dir = QDir::temp();
  QVERIFY(dir.cd("test_init_repo"));
  QVERIFY(dir.removeRecursively());
}

TEST_MAIN(TestInitRepo)

#include "init_repo.moc"
