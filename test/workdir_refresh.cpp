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
#include "ui/DetailView.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <cstdio>

using namespace Test;
using namespace QTest;

// Changes of the working directory show up, and a commit is made once.
class TestWorkdirRefresh : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void modifiedFile();
  void renamedIntoPlace();
  void manyChanges();
  void commitOnce();
  void nothingToCommit();
  void checkoutDuringStatus();
  void cleanupTestCase();

private:
  void write(const QString &path, const QByteArray &content);
  bool waitForStatus(bool dirty);
  git::Id head();

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  RepoView *mView = nullptr;
};

void TestWorkdirRefresh::initTestCase() {
  write(mRepo->workdir().filePath("file.txt"), "content\n");
  QDir(mRepo->workdir()).mkpath("dir");
  write(mRepo->workdir().filePath("dir/other.txt"), "other\n");
  mRepo->index().setStaged({"file.txt", "dir/other.txt"}, true);
  QVERIFY(mRepo->commit("initial"));

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));
  mView = mWindow->currentView();
  refresh(mView, false);
}

// A file changed by another program shows up without a refresh.
void TestWorkdirRefresh::modifiedFile() {
  write(mRepo->workdir().filePath("file.txt"), "changed\n");
  QVERIFY(waitForStatus(true));

  write(mRepo->workdir().filePath("file.txt"), "content\n");
  QVERIFY(waitForStatus(false));
}

// Editors save files by renaming a new file over the old one.
void TestWorkdirRefresh::renamedIntoPlace() {
  // Write it outside of the working directory, on the same file system.
  QString temp =
      QDir(mRepo->workdir().path() + "/..")
          .absoluteFilePath(
              QString("saved-%1.txt").arg(QCoreApplication::applicationPid()));
  write(temp, "renamed\n");

  // Replace the file in one step.
  QString path = mRepo->workdir().filePath("dir/other.txt");
#ifdef Q_OS_WIN
  QVERIFY(QFile::remove(path));
#endif
  QVERIFY(!std::rename(QFile::encodeName(temp), QFile::encodeName(path)));
  QVERIFY(waitForStatus(true));

  write(path, "other\n");
  QVERIFY(waitForStatus(false));
}

// More changes than the watcher can hold, like from a build, don't stop it.
void TestWorkdirRefresh::manyChanges() {
  QDir dir(mRepo->workdir());
  dir.mkpath("burst");
  for (int i = 0; i < 5000; ++i) {
    QString name = QString("burst/a-file-with-a-rather-long-name-%1.txt").arg(i);
    write(mRepo->workdir().filePath(name), "burst\n");
  }
  QVERIFY(waitForStatus(true));

  QVERIFY(QDir(dir.filePath("burst")).removeRecursively());
  QVERIFY(waitForStatus(false));

  // A change after them is still seen.
  write(mRepo->workdir().filePath("file.txt"), "after\n");
  QVERIFY(waitForStatus(true));

  write(mRepo->workdir().filePath("file.txt"), "content\n");
  QVERIFY(waitForStatus(false));
}

// Committing again before the files are listed again doesn't commit nothing.
void TestWorkdirRefresh::commitOnce() {
  write(mRepo->workdir().filePath("file.txt"), "once\n");
  refresh(mView, true);

  DetailView *details = mView->detailView();
  details->stage();
  details->setCommitMessage("Commit once");
  QVERIFY(details->isCommitEnabled());

  git::Id before = head();
  details->commitChanges();
  git::Id after = head();
  QVERIFY(after != before);

  // The button waits for the new status of the files.
  QVERIFY(!details->isCommitEnabled());
  details->commitChanges();
  details->commitChanges();
  QCOMPARE(head(), after);

  // And doesn't make up a message for the files that were committed.
  QCOMPARE(details->commitMessage(), QString());

  refresh(mView, false);
  QVERIFY(!details->isCommitEnabled());
  QCOMPARE(head(), after);
}

// Like git, a commit needs changes, unless it's a merge.
void TestWorkdirRefresh::nothingToCommit() {
  git::Id before = head();
  QVERIFY(!mView->commit(QString("Nothing")));
  QCOMPARE(head(), before);
}

// Checking out while the status of many files is read leaves no changes.
void TestWorkdirRefresh::checkoutDuringStatus() {
  QString main = mRepo->head().name();
  QStringList files;
  QDir(mRepo->workdir()).mkpath("many");
  for (int i = 0; i < 2000; ++i) {
    QString name = QString("many/file%1.txt").arg(i);
    write(mRepo->workdir().filePath(name), "main\n");
    files.append(name);
  }
  mRepo->index().setStaged(files, true);
  QVERIFY(mView->commit(QString("Many files")));

  git::Branch other = mView->createBranch("other", mRepo->head().target());
  QVERIFY(other.isValid());
  mView->checkout(other);
  for (const QString &name : files)
    write(mRepo->workdir().filePath(name), "other\n");
  mRepo->index().setStaged(files, true);
  QVERIFY(mView->commit(QString("Change many files")));
  refresh(mView, false);

  for (int i = 0; i < 5; ++i) {
    QString name = (i % 2) ? "other" : main;
    mView->refresh();
    mView->checkout(mRepo->lookupBranch(name, GIT_BRANCH_LOCAL));
    QCOMPARE(mRepo->head().name(), name);
    refresh(mView, false);
  }
}

void TestWorkdirRefresh::cleanupTestCase() { mWindow->close(); }

void TestWorkdirRefresh::write(const QString &path, const QByteArray &content) {
  QFile file(path);
  QVERIFY(file.open(QFile::WriteOnly));
  file.write(content);
}

// Wait for the watcher to refresh the status.
bool TestWorkdirRefresh::waitForStatus(bool dirty) {
  QSignalSpy spy(mView, &RepoView::statusChanged);
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < 10000) {
    if (!spy.wait(10000 - timer.elapsed()))
      return false;
    if (spy.last().first().toBool() == dirty)
      return true;
  }

  return false;
}

git::Id TestWorkdirRefresh::head() { return mRepo->head().target().id(); }

TEST_MAIN(TestWorkdirRefresh)

#include "workdir_refresh.moc"
