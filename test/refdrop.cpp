//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "conf/Settings.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "git/TagRef.h"
#include "ui/MainWindow.h"
#include "ui/RefDrop.h"
#include "ui/RepoView.h"
#include <QFile>

using namespace Test;
using namespace QTest;

// Drops branches onto other branches and remotes like in GitKraken.
class TestRefDrop : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void fastForward();
  void rebase();
  void merge();
  void push();
  void cleanupTestCase();

private:
  git::Id commit(const QString &name);
  QStringList texts(const QString &source, const QString &target);
  bool run(const QString &source, const QString &target, const QString &text);
  QString branch(const QString &name) { return "refs/heads/" + name; }

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  RepoView *mView = nullptr;
  RefDrop *mDrop = nullptr;
  QString mMain;
};

void TestRefDrop::initTestCase() {
  Settings::instance()->setPrompt(Prompt::Kind::Merge, false);

  commit("file.txt");
  mMain = mRepo->head().name();

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));

  mView = mWindow->currentView();
  mDrop = mView->refDrop();
}

void TestRefDrop::fastForward() {
  git::Commit first = mRepo->head().target();
  commit("a.txt");
  git::Id second = mRepo->head().target().id();
  mRepo->createBranch("old", first);

  // A branch that is behind is fast-forwarded without checking it out.
  QCOMPARE(texts(branch(mMain), branch("old")),
           QStringList({QString("Fast-forward old to %1").arg(mMain),
                        QString("Merge %1 into old").arg(mMain)}));
  QVERIFY(run(branch(mMain), branch("old"), "Fast-forward"));
  QCOMPARE(mRepo->lookupBranch("old").target().id(), second);
  QCOMPARE(mRepo->head().name(), mMain);

  // Nothing to do for branches at the same commit.
  QVERIFY(texts(branch(mMain), branch("old")).isEmpty());

  // Dragging works while the reference is dragged.
  mDrop->start(branch(mMain), 10, 10);
  QVERIFY(mDrop->isActive());
  QCOMPARE(mDrop->label(), mMain);
  QVERIFY(!mDrop->accepts(branch("old")));
  QVERIFY(!mDrop->accepts(branch(mMain)));
  mDrop->cancel();
  QVERIFY(!mDrop->isActive());
}

void TestRefDrop::rebase() {
  // A topic branch and the main branch move apart.
  mView->createBranch("topic", mRepo->head().target(), git::Branch(), true);
  QCOMPARE(mRepo->head().name(), QString("topic"));
  commit("b.txt");
  mView->checkout(mRepo->lookupBranch(mMain));
  QCOMPARE(mRepo->head().name(), mMain);
  git::Id main = commit("c.txt");

  QCOMPARE(texts(branch("topic"), branch(mMain)),
           QStringList({QString("Merge topic into %1").arg(mMain),
                        QString("Rebase topic onto %1").arg(mMain),
                        QString("Interactive Rebase topic onto %1").arg(mMain)}));

  // Rebasing checks out the dragged branch first.
  QVERIFY(run(branch("topic"), branch(mMain), "Rebase"));
  QCOMPARE(mRepo->head().name(), QString("topic"));
  git::Commit topic = mRepo->head().target();
  QCOMPARE(topic.parents().first().id(), main);
  QVERIFY(QFile::exists(mRepo->workdir().filePath("b.txt")));
  QVERIFY(QFile::exists(mRepo->workdir().filePath("c.txt")));
}

void TestRefDrop::merge() {
  // Merging checks out the target branch first.
  mView->checkout(mRepo->lookupBranch(mMain));
  commit("d.txt");
  QVERIFY(run(branch("topic"), branch(mMain), "Merge"));
  QCOMPARE(mRepo->head().name(), mMain);
  QCOMPARE(mRepo->head().target().parents().size(), 2);
  QVERIFY(QFile::exists(mRepo->workdir().filePath("b.txt")));

  // Merged branches can't be merged again.
  QVERIFY(!texts(branch("topic"), branch(mMain))
               .contains(QString("Merge topic into %1").arg(mMain)));
}

void TestRefDrop::push() {
  QVERIFY(texts(branch(mMain), "remote:origin").isEmpty());
  mRepo->addRemote("origin", "https://example.com/repo.git");
  QCOMPARE(texts(branch(mMain), "remote:origin"),
           QStringList({QString("Push %1 to origin").arg(mMain)}));

  // Tags aren't pushed by dropping.
  mRepo->createTag(mRepo->head().target(), "v1");
  QVERIFY(texts("refs/tags/v1", "remote:origin").isEmpty());
}

void TestRefDrop::cleanupTestCase() {
  delete mWindow;
  mWindow = nullptr;
}

git::Id TestRefDrop::commit(const QString &name) {
  QFile file(mRepo->workdir().filePath(name));
  if (!file.open(QFile::WriteOnly))
    return git::Id();
  file.write(name.toUtf8() + "\n");
  file.close();

  mRepo->index().setStaged({name}, true);
  git::Commit commit = mRepo->commit(QString("Add %1").arg(name));
  return commit.id();
}

QStringList TestRefDrop::texts(const QString &source, const QString &target) {
  QStringList result;
  for (const RefDrop::Choice &choice : mDrop->choices(source, target))
    result.append(choice.text);
  return result;
}

bool TestRefDrop::run(const QString &source, const QString &target,
                      const QString &text) {
  for (const RefDrop::Choice &choice : mDrop->choices(source, target)) {
    if (choice.text.startsWith(text)) {
      choice.run();
      return true;
    }
  }
  return false;
}

TEST_MAIN(TestRefDrop)

#include "refdrop.moc"
