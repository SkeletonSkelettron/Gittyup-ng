//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "Debug.h"
#include "dialogs/CloneDialog.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QMenu>
#include <QToolButton>

using namespace Test;
using namespace QTest;

class TestBareRepo : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void checkDir();
  void cleanupTestCase();

private:
  MainWindow *mWindow = nullptr;
};

void TestBareRepo::initTestCase() {
  // Initialize a repository like the welcome page does.
  CloneDialog *cloneDialog = new CloneDialog(CloneDialog::Init);
  QObject::connect(cloneDialog, &CloneDialog::accepted, [cloneDialog] {
    MainWindow::open(cloneDialog->path());
  });
  cloneDialog->open();
  QVERIFY(qWaitForWindowExposed(cloneDialog));

  // Set fields.
  cloneDialog->setField("name", "test_bare_repo");
  cloneDialog->setField("path", QDir::tempPath());
  cloneDialog->setField("bare", true);
  QVERIFY(cloneDialog->field("bare").toBool());

  // Initialize.
  QVERIFY(cloneDialog->canContinue());
  cloneDialog->next();

  // Wait on the new window.
  mWindow = MainWindow::activeWindow();
  QVERIFY(mWindow && qWaitForWindowActive(mWindow));
}

void TestBareRepo::checkDir() {
  RepoView *view = mWindow->currentView();
  QVERIFY(view->repo().isBare());

  QDir dir = QDir::temp();
  QVERIFY(dir.cd("test_bare_repo"));

  QVERIFY(!dir.exists(".git"));

  QVERIFY(dir.exists("config"));
  QVERIFY(dir.exists("objects"));
  QVERIFY(dir.exists("HEAD"));
  QVERIFY(dir.exists("info"));
  QVERIFY(dir.exists("description"));
  QVERIFY(dir.exists("hooks"));
  QVERIFY(dir.exists("refs"));
}

void TestBareRepo::cleanupTestCase() {
  if (mWindow) {
    mWindow->close();
  }
  QDir dir = QDir::temp();
  QVERIFY(dir.cd("test_bare_repo"));
  QVERIFY(dir.removeRecursively());
}

TEST_MAIN(TestBareRepo)

#include "bare_repo.moc"
