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
#include "git/TagRef.h"
#include "git/Tree.h"
#include "ui/DetailView.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/UndoHistory.h"
#include <QFile>
#include <QProcess>

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
  void laterTags();
  void laterUpstreams();
  void nestedRename();
  void otherWorktree();
  void lockedHead();
  void firstCommit();
  void cleanupTestCase();

private:
  QString git(const QStringList &args, const QString &dir = QString());
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

// Undoing and redoing an action keeps the tags that came after it, like the
// ones of a fetch, which aren't actions.
void TestUndo::laterTags() {
  mView->createBranch("tagged", mRepo->lookupCommit(mFirst));
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Create Branch tagged"));

  git({"tag", "fetched", mFirst.toString()});
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Create Branch tagged"));

  QVERIFY(mHistory->undo());
  QVERIFY(!mRepo->lookupBranch("tagged").isValid());
  QVERIFY(mRepo->lookupTag("fetched").isValid());
  QVERIFY(mHistory->redo());
  QVERIFY(mRepo->lookupBranch("tagged").isValid());
  QVERIFY(mRepo->lookupTag("fetched").isValid());
}

// Upstreams that change after an action aren't actions. They stay when it's
// undone and redone, also the one of the branch that it moved.
void TestUndo::laterUpstreams() {
  git::Branch branch =
      mView->createBranch("tracked", mRepo->lookupCommit(mFirst));
  mHistory->check();
  git::Commit second = mRepo->lookupBranch("ahead").target();
  QVERIFY(branch.setTarget(second, "test").isValid());
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Update tracked"));

  QStringList names = {"tracked", mMain};
  for (const QString &name : names) {
    QString key = QString("branch.%1.").arg(name);
    mRepo->gitConfig().setValue(key + "remote", QString("origin"));
    mRepo->gitConfig().setValue(key + "merge", QString("refs/heads/upstream"));
  }
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Update tracked"));

  auto upstreams = [this, &names] {
    QStringList merges;
    for (const QString &name : names) {
      QString key = QString("branch.%1.merge").arg(name);
      merges.append(mRepo->gitConfig().value<QString>(key));
    }
    return merges;
  };

  QStringList expected = {"refs/heads/upstream", "refs/heads/upstream"};
  QVERIFY(mHistory->undo());
  QCOMPARE(mRepo->lookupBranch("tracked").target().id(), mFirst);
  QCOMPARE(upstreams(), expected);
  QVERIFY(mHistory->redo());
  QCOMPARE(mRepo->lookupBranch("tracked").target().id(), second.id());
  QCOMPARE(upstreams(), expected);

  for (const QString &name : names)
    git({"config", "--remove-section", QString("branch.%1").arg(name)});
  mHistory->check();
}

// A branch renamed into a directory of its old name, which can't exist next
// to it, is renamed back, also when it's checked out.
void TestUndo::nestedRename() {
  git::Branch branch =
      mView->createBranch("nested", mRepo->lookupCommit(mFirst));
  mHistory->check();
  QVERIFY(branch.rename("nested/inner").isValid());
  mHistory->check();
  QCOMPARE(mHistory->undoText(),
           QString("Rename Branch nested to nested/inner"));

  QVERIFY(mHistory->undo());
  QVERIFY(mRepo->lookupBranch("nested").isValid());
  QVERIFY(!mRepo->lookupBranch("nested/inner").isValid());
  QVERIFY(mHistory->redo());
  QVERIFY(!mRepo->lookupBranch("nested").isValid());
  QVERIFY(mRepo->lookupBranch("nested/inner").isValid());

  mView->checkout(mRepo->lookupBranch("nested/inner"));
  mHistory->check();
  QVERIFY(mRepo->lookupBranch("nested/inner").rename("nested").isValid());
  QCOMPARE(mRepo->head().name(), QString("nested"));
  mHistory->check();

  QVERIFY(mHistory->undo());
  QCOMPARE(mRepo->head().name(), QString("nested/inner"));
  QVERIFY(!mRepo->lookupBranch("nested").isValid());
  QVERIFY(mHistory->redo());
  QCOMPARE(mRepo->head().name(), QString("nested"));
  QVERIFY(!mRepo->lookupBranch("nested/inner").isValid());

  mView->checkout(mRepo->lookupBranch(mMain));
  mHistory->check();
}

// A branch that another worktree checked out doesn't move, and HEAD doesn't
// point to it, also not again, which would leave the index and the files of
// that worktree behind. Nothing changes then.
void TestUndo::otherWorktree() {
  git::Commit first = mRepo->lookupCommit(mFirst);
  git::Commit second = mRepo->lookupBranch("ahead").target();
  QVERIFY(second.isValid() && second.id() != mFirst);

  mView->createBranch("linked", first);
  mHistory->check();
  QVERIFY(mRepo->lookupBranch("linked").setTarget(second, "test").isValid());
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Update linked"));

  QTemporaryDir dir;
  QString path = dir.filePath("linked");
  git({"worktree", "add", "-q", path, "linked"});
  QVERIFY(QFile::exists(path + "/a.txt"));

  QVERIFY(!mRepo->lookupBranch("linked").setTarget(first, "test").isValid());
  QVERIFY(!mHistory->undo());
  QCOMPARE(mRepo->lookupBranch("linked").target().id(), second.id());
  QCOMPARE(git({"status", "--porcelain"}, path), QString());

  git({"worktree", "remove", "--force", path});
  QVERIFY(mHistory->undo());
  QCOMPARE(mRepo->lookupBranch("linked").target().id(), mFirst);

  // Nor checking it out here.
  QVERIFY(mHistory->redo());
  git({"worktree", "add", "-q", path, "linked"});
  mView->checkout(mRepo->lookupBranch("linked"));
  QCOMPARE(mRepo->head().name(), mMain);
  QCOMPARE(git({"status", "--porcelain"}), QString());
  git({"worktree", "remove", "--force", path});

  // Undo checkouts of another commit, from the branch and from a detached
  // HEAD, while another worktree checked the branch out.
  mView->checkout(mRepo->lookupBranch("linked"));
  mHistory->check();
  git({"worktree", "add", "-q", path, mMain});
  QVERIFY(!mHistory->undo());
  QCOMPARE(mRepo->head().name(), QString("linked"));
  QCOMPARE(git({"status", "--porcelain"}), QString());
  git({"worktree", "remove", "--force", path});
  QVERIFY(mHistory->undo());
  QCOMPARE(mRepo->head().name(), mMain);

  mView->checkout(second, git::Reference(), true);
  mHistory->check();
  git({"worktree", "add", "-q", path, mMain});
  QVERIFY(!mHistory->undo());
  QVERIFY(mRepo->head().isDetachedHead());
  QCOMPARE(git({"status", "--porcelain"}), QString());
  git({"worktree", "remove", "--force", path});
  QVERIFY(mHistory->undo());
  QCOMPARE(mRepo->head().name(), mMain);
}

// When HEAD can't change after the files changed, like while another git
// process locks it, undoing fails and changes the files and the index back.
void TestUndo::lockedHead() {
  mView->checkout(mRepo->lookupBranch("ahead"));
  mHistory->check();
  QCOMPARE(mHistory->undoText(), QString("Checkout ahead"));

  QFile lock(mRepo->dir().filePath("HEAD.lock"));
  QVERIFY(lock.open(QFile::WriteOnly));
  lock.close();
  QVERIFY(!mHistory->undo());
  QVERIFY(lock.remove());

  QCOMPARE(mRepo->head().name(), QString("ahead"));
  QCOMPARE(read("a.txt"), QByteArray("one\n"));
  QCOMPARE(git({"status", "--porcelain"}), QString());
  QVERIFY(mHistory->undo());
  QCOMPARE(mRepo->head().name(), mMain);
}

// The first commit of a repository is undone too: its branch is unborn
// again, and the files of the commit stay staged, like before it.
void TestUndo::firstCommit() {
  ScratchRepository repo;
  MainWindow window(repo);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));
  RepoView *view = window.currentView();
  UndoHistory *history = view->undoHistory();

  QFile file(repo->workdir().filePath("first.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("first\n");
  file.close();
  repo->index().setStaged({"first.txt"}, true);
  QVERIFY(view->commit(QString("First")));
  history->check();
  QVERIFY(history->canUndo());

  QString dir = repo->workdir().path();
  QVERIFY(history->undo());
  QVERIFY(repo->isHeadUnborn());
  QCOMPARE(git({"status", "--porcelain"}, dir), QString("A  first.txt"));
  QCOMPARE(view->detailView()->commitMessage(), QString("First"));

  QVERIFY(history->redo());
  QVERIFY(!repo->isHeadUnborn());
  QCOMPARE(repo->head().target().summary(), QString("First"));
  QCOMPARE(git({"status", "--porcelain"}, dir), QString());
}

void TestUndo::cleanupTestCase() {
  delete mWindow;
  mWindow = nullptr;
}

// Run git in the repository, or in 'dir', for what Gittyup doesn't do itself.
QString TestUndo::git(const QStringList &args, const QString &dir) {
  QProcess process;
  process.setWorkingDirectory(dir.isEmpty() ? mRepo->workdir().path() : dir);
  process.start("git", args);
  process.waitForFinished();
  return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
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
