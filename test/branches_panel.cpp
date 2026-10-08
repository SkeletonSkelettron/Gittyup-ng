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
#include <QQuickWidget>
#include "dialogs/ConfigDialog.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QDialog>
#include <QMenu>
#include <QPushButton>

using namespace Test;
using namespace QTest;

class TestBranchesPanel : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void createBranch();
  void cleanupTestCase();

private:
  int inputDelay = 0;
  int closeDelay = 0;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  ConfigDialog *mConfigDialog = nullptr;
};

void TestBranchesPanel::initTestCase() {
  mWindow = new MainWindow(mRepo);
  RepoView *view = mWindow->currentView();

  git::Remote remote = mRepo->addRemote(
      "origin", "https://github.com/Murmele/GittyupTestRepo.git");
  fetch(view, remote);

  git::Branch upstream =
      mRepo->lookupBranch("origin/master", GIT_BRANCH_REMOTE);
  QVERIFY(upstream.isValid());

  git::Branch branch =
      view->createBranch("master", upstream.target(), upstream, true);
  QVERIFY(branch.isValid());

  mWindow->show();
  mConfigDialog = view->configureSettings(ConfigDialog::Branches);
  QVERIFY(qWaitForWindowExposed(mConfigDialog));
}

void TestBranchesPanel::createBranch() {
  QCOMPARE(mConfigDialog->section(), static_cast<int>(ConfigDialog::Branches));

  // Add a branch.
  mConfigDialog->newBranch();

  // The new branch dialog opens with the name field focused.
  NewBranchDialog *dialog = mConfigDialog->findChild<NewBranchDialog *>();
  QVERIFY(dialog);
  QVERIFY(qWaitForWindowExposed(dialog));
  QQuickWidget *view = dialog->findChild<QQuickWidget *>();
  QVERIFY(view);
  keyClicks(view, "feature");
  QCOMPARE(dialog->name(), QString("feature"));

  // Select upstream origin/master.
  QVariantList upstreams = dialog->upstreams();
  int upstream = -1;
  for (int i = 0; i < upstreams.size(); ++i) {
    if (upstreams.at(i).toMap().value("text") == "origin/master")
      upstream = i;
  }
  QVERIFY(upstream > 0);
  dialog->setUpstreamIndex(upstream);

  // Accept.
  QVERIFY(dialog->isAcceptable());
  dialog->accept();

  git::Branch branch = mRepo->lookupBranch("feature", GIT_BRANCH_LOCAL);
  QVERIFY(branch.isValid());
  QCOMPARE(branch.upstream().name(), QString("origin/master"));

  // The branch is listed with its upstream.
  bool listed = false;
  QStringList upstreamNames = mConfigDialog->upstreams();
  for (const QVariant &var : mConfigDialog->branches()) {
    QVariantMap map = var.toMap();
    if (map.value("name") == "feature") {
      listed = true;
      QCOMPARE(upstreamNames.at(map.value("upstream").toInt()),
               QString("origin/master"));
    }
  }
  QVERIFY(listed);
}

void TestBranchesPanel::cleanupTestCase() {
  qWait(closeDelay);
  mWindow->close();
}

TEST_MAIN(TestBranchesPanel)

#include "branches_panel.moc"
