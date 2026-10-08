//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "dialogs/NewBranchDialog.h"
#include <QMainWindow>
#include <QQuickWidget>

using namespace Test;
using namespace QTest;

class TestNewBranchDialog : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void verifyName();
  void cleanupTestCase();

private:
  int closeDelay = 0;

  ScratchRepository mRepo;
  QMainWindow *mWindow = nullptr;
};

void TestNewBranchDialog::initTestCase() {
  mWindow = new QMainWindow;
  NewBranchDialog *dialog = new NewBranchDialog(mRepo, git::Commit(), mWindow);
  dialog->show();
  QVERIFY(qWaitForWindowExposed(dialog));
}

void TestNewBranchDialog::verifyName() {
  NewBranchDialog *dialog = mWindow->findChild<NewBranchDialog *>();
  QVERIFY(dialog);
  QVERIFY(!dialog->isAcceptable());

  // The name field has the focus.
  QQuickWidget *view = dialog->findChild<QQuickWidget *>();
  QVERIFY(view);

  keyClicks(view, "valid");
  QCOMPARE(dialog->name(), QString("valid"));
  QVERIFY(dialog->isAcceptable());

  keyClick(view, 'a', Qt::ControlModifier);
  keyClick(view, Qt::Key_Delete);
  QVERIFY(dialog->name().isEmpty());

  keyClicks(view, "Invalid Name");
  QVERIFY(!dialog->isAcceptable());
  QVERIFY(!dialog->nameError().isEmpty());
}

void TestNewBranchDialog::cleanupTestCase() {
  qWait(closeDelay);
  mWindow->close();
}

TEST_MAIN(TestNewBranchDialog)

#include "new_branch_dialog.moc"
