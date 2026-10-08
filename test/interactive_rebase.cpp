//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "ui/InteractiveRebase.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/UndoHistory.h"
#include <QFile>

using namespace Test;
using namespace QTest;

// Picks, rewords, squashes, drops and reorders commits like GitKraken.
class TestInteractiveRebase : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void rewordAndDrop();
  void squash();
  void reorder();
  void conflict();
  void otherBranch();
  void cleanupTestCase();

private:
  git::Commit commit(const QString &name, const QByteArray &content = {});
  git::Commit head() { return mRepo->head().target(); }
  bool exists(const QString &name) {
    return QFile::exists(mRepo->workdir().filePath(name));
  }
  QStringList summaries();

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  RepoView *mView = nullptr;
  InteractiveRebase *mRebase = nullptr;
  git::Commit mBase;
  QString mMain;
};

void TestInteractiveRebase::initTestCase() {
  mBase = commit("file.txt");
  mMain = mRepo->head().qualifiedName();

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));

  mView = mWindow->currentView();
  mRebase = mView->interactiveRebase();
  QVERIFY(!mRebase->isActive());
}

void TestInteractiveRebase::rewordAndDrop() {
  commit("a.txt");
  commit("b.txt");
  commit("c.txt");
  git::Commit tip = head();

  QVERIFY(!mRebase->canOpen(mMain, tip));
  QVERIFY(mRebase->open(mMain, mBase, mBase.shortId()));
  QVERIFY(mRebase->isActive());
  QCOMPARE(summaries(),
           QStringList({"Add c.txt", "Add b.txt", "Add a.txt"}));
  QVERIFY(!mRebase->isModified());

  // The oldest commit can't be squashed.
  mRebase->setAction(2, InteractiveRebase::Squash);
  QVERIFY(!mRebase->problem().isEmpty());
  QVERIFY(!mRebase->start());
  mRebase->reset();
  QVERIFY(mRebase->problem().isEmpty());

  mRebase->setAction(1, InteractiveRebase::Reword);
  mRebase->setMessage(1, "Add b, reworded\n");
  mRebase->setAction(2, InteractiveRebase::Drop);
  QVERIFY(mRebase->isModified());
  QVERIFY(mRebase->start());
  QVERIFY(!mRebase->isActive());

  git::Commit c = head();
  QCOMPARE(c.summary(), QString("Add c.txt"));
  git::Commit b = c.parents().first();
  QCOMPARE(b.summary(), QString("Add b, reworded"));
  QCOMPARE(b.parents().first().id(), mBase.id());
  QVERIFY(!exists("a.txt"));
  QVERIFY(exists("b.txt"));
  QVERIFY(exists("c.txt"));

  // The rebase is undone like other actions.
  UndoHistory *history = mView->undoHistory();
  history->check();
  QVERIFY(history->undoText().startsWith("Interactive Rebase"));
  QVERIFY(history->undo());
  QCOMPARE(head().id(), tip.id());
  QVERIFY(exists("a.txt"));
  QVERIFY(history->redo());
  QCOMPARE(head().id(), c.id());
}

void TestInteractiveRebase::squash() {
  QVERIFY(mRebase->open(mMain, mBase, mBase.shortId()));
  QCOMPARE(summaries(), QStringList({"Add c.txt", "Add b, reworded"}));
  mRebase->setAction(0, InteractiveRebase::Squash);
  QVERIFY(mRebase->start());

  git::Commit squashed = head();
  QCOMPARE(squashed.parents().first().id(), mBase.id());
  QCOMPARE(squashed.message(), QString("Add b, reworded\n\nAdd c.txt\n"));
  QVERIFY(exists("b.txt"));
  QVERIFY(exists("c.txt"));
}

void TestInteractiveRebase::reorder() {
  git::Commit base = head();
  commit("d.txt");
  commit("e.txt");

  QVERIFY(mRebase->open(mMain, base, base.shortId()));
  QCOMPARE(summaries(), QStringList({"Add e.txt", "Add d.txt"}));
  mRebase->move(0, 1);
  QCOMPARE(summaries(), QStringList({"Add d.txt", "Add e.txt"}));
  QVERIFY(mRebase->start());

  QCOMPARE(head().summary(), QString("Add d.txt"));
  QCOMPARE(head().parents().first().summary(), QString("Add e.txt"));
  QCOMPARE(head().parents().first().parents().first().id(), base.id());
}

void TestInteractiveRebase::conflict() {
  git::Commit base = head();
  commit("x.txt", "1\n");
  commit("x.txt", "2\n");
  git::Commit tip = head();

  // The second change doesn't apply without the first one.
  QVERIFY(mRebase->open(mMain, base, base.shortId()));
  mRebase->setAction(1, InteractiveRebase::Drop);
  QVERIFY(!mRebase->start());
  QCOMPARE(head().id(), tip.id());
  QVERIFY(mRebase->isActive());

  mRebase->cancel();
  QVERIFY(!mRebase->isActive());
  QCOMPARE(head().id(), tip.id());
}

void TestInteractiveRebase::otherBranch() {
  // A branch that isn't checked out is rebased without checking it out.
  git::Commit base = head();
  mView->createBranch("side", base, git::Branch(), true);
  commit("s1.txt");
  commit("s2.txt");
  mView->checkout(mRepo->lookupBranch(mMain.section('/', 2)));
  QCOMPARE(mRepo->head().qualifiedName(), mMain);

  QVERIFY(mRebase->open("refs/heads/side", base, base.shortId()));
  mRebase->setAction(1, InteractiveRebase::Drop);
  QVERIFY(mRebase->start());

  git::Commit side = mRepo->lookupBranch("side").target();
  QCOMPARE(side.summary(), QString("Add s2.txt"));
  QCOMPARE(side.parents().first().id(), base.id());
  QCOMPARE(mRepo->head().qualifiedName(), mMain);
  QVERIFY(!exists("s2.txt"));
}

void TestInteractiveRebase::cleanupTestCase() {
  delete mWindow;
  mWindow = nullptr;
}

git::Commit TestInteractiveRebase::commit(const QString &name,
                                          const QByteArray &content) {
  QFile file(mRepo->workdir().filePath(name));
  if (!file.open(QFile::WriteOnly | QFile::Truncate))
    return git::Commit();
  file.write(content.isEmpty() ? name.toUtf8() + "\n" : content);
  file.close();

  mRepo->index().setStaged({name}, true);
  return mRepo->commit(QString("Add %1").arg(name));
}

QStringList TestInteractiveRebase::summaries() {
  QStringList result;
  for (int row = 0; row < mRebase->rowCount(); ++row)
    result.append(mRebase->index(row).data(InteractiveRebase::SummaryRole)
                      .toString());
  return result;
}

TEST_MAIN(TestInteractiveRebase)

#include "interactive_rebase.moc"
