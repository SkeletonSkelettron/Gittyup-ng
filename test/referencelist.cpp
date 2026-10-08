//
//          Copyright (c) 2022, Gittyup Contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "Test.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "dialogs/CloneDialog.h"
#include "dialogs/ReferenceItems.h"
#include <QMainWindow>

using namespace Test;
using namespace QTest;

class TestReferenceList : public QObject {
  Q_OBJECT

private slots:
  void test();

private:
};

void TestReferenceList::test() {
  CloneDialog *d = new CloneDialog(CloneDialog::Kind::Clone);

  RepoView *view = nullptr;

  bool cloneFinished = false;
  QObject::connect(d, &CloneDialog::accepted, [d, &view, &cloneFinished] {
    cloneFinished = true;
    if (MainWindow *window = MainWindow::open(d->path())) {
      view = window->currentView();
    }
  });

  QTemporaryDir tempdir;
  QVERIFY(tempdir.isValid());
  const auto repoPath = tempdir.path();
  d->setField("url", "https://github.com/Murmele/GittyupTestRepo.git");
  d->setField("name", "GittyupTestRepo");
  d->setField("path", repoPath);
  d->setField("bare", "false");
  d->startClone();

  {
    auto timeout = Timeout(1000e3, "Failed to clone");
    while (!cloneFinished)
      qWait(300);
  }
  QVERIFY(view);
  git::Repository repo = view->repo();
  QVERIFY(repo.isValid());
  initRepo(repo);

  ReferenceItems items(repo, ReferenceItems::LocalBranches |
                                 ReferenceItems::RemoteBranches |
                                 ReferenceItems::Tags);
  auto name = [&items](const git::Commit &commit) {
    int index = items.indexOf(commit);
    return index >= 0 ? items.reference(index).name() : QString();
  };

  {
    // Only one tag
    git::Commit commit =
        repo.lookupCommit("99219268e1f838b0da616761fd7a184676965a69");
    QVERIFY(commit.isValid());
    QCOMPARE(name(commit), QString("Tag"));
  }

  {
    // Only one remote
    git::Commit commit =
        repo.lookupCommit("79f4bee33320391fa99a8ef3f504b2ba229a8181");
    QVERIFY(commit.isValid());
    QCOMPARE(name(commit), QString("origin/Branch"));
  }

  {
    // Only one branch
    git::Commit commit =
        repo.lookupCommit("54ecb63965b50287ceb73095c72f344c1611d94a");
    QVERIFY(commit.isValid());
    QCOMPARE(name(commit), QString("main"));
  }

  {
    // No branch, no remote, no tag
    git::Commit commit =
        repo.lookupCommit("63460da2b069250c34506249516029f2ba7c6057");
    QVERIFY(commit.isValid());
    QCOMPARE(name(commit), QString(""));
  }
}

TEST_MAIN(TestReferenceList)

#include "referencelist.moc"
