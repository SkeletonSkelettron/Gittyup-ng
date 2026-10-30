//
//          Copyright (c) 2022, Gittyup Team
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "Test.h"

#include "qtsupport.h"
#include "dialogs/CloneDialog.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/RepoView.h"
#include "conf/Settings.h"
#include "git/Submodule.h"
#include "ui/ChangedFilesModel.h"
#include "ui/DetailView.h"

#include <QToolButton>
#include <QMenu>
#include <QLineEdit>
#include <QProcess>
#include "dialogs/ConfirmDialog.h"

#define INIT_REPO(repoPath)                                                    \
  QString path = Test::extractRepository(repoPath);                            \
  QVERIFY(!path.isEmpty());                                                    \
  auto repo = git::Repository::open(path);                                     \
  QVERIFY(repo.isValid());                                                     \
  Test::initRepo(repo);                                                        \
  MainWindow window(repo);                                                     \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
                                                                               \
  RepoView *repoView = window.currentView();                                   \
  auto diff = repo.status(repo.index(), nullptr, false);

using namespace Test;
using namespace QTest;

namespace {

// Run git in 'dir' as a user that may clone local submodules.
QString runGit(const QString &dir, const QStringList &args,
               int *exitCode = nullptr) {
  QProcess process;
  process.setWorkingDirectory(dir);
  process.start("git", QStringList{"-c", "user.name=Test", "-c",
                                   "user.email=test@example.com", "-c",
                                   "protocol.file.allow=always"} +
                           args);
  process.waitForFinished(-1);
  if (exitCode)
    *exitCode = process.exitCode();
  return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

void write(const QString &path, const QByteArray &content) {
  QFile file(path);
  QVERIFY(file.open(QFile::WriteOnly));
  file.write(content);
}

// A repository with a commit, for submodules to clone.
QString source(const QTemporaryDir &root) {
  QString path = root.filePath("source");
  runGit(root.path(), {"init", "-q", path});
  write(path + "/file.txt", "origin\n");
  runGit(path, {"add", "."});
  runGit(path, {"commit", "-qm", "origin"});
  return path;
}

} // namespace

class TestSubmodule : public QObject {
  Q_OBJECT

private slots:
  void updateSubmoduleClone();
  void noUpdateSubmoduleClone();
  void discardFile();
  void deinitializeKeepsHistory();
  void deinitializeOnlyItself();

private:
};

void TestSubmodule::updateSubmoduleClone() {
  // Update submodules after cloning
  QString remote = Test::extractRepository("SubmoduleTest.zip");
  QCOMPARE(remote.isEmpty(), false);

  Settings *settings = Settings::instance();
  settings->setValue(Setting::Id::UpdateSubmodulesAfterPullAndClone, true);
  CloneDialog *d = new CloneDialog(CloneDialog::Kind::Clone);

  RepoView *view = nullptr;
  MainWindow *window = nullptr;

  bool cloneFinished = false;
  QObject::connect(d, &CloneDialog::accepted,
                   [d, &window, &view, &cloneFinished] {
                     cloneFinished = true;
                     window = MainWindow::open(d->path());
                     if (window) {
                       view = window->currentView();
                     }
                   });

  QTemporaryDir tempdir;
  QVERIFY(tempdir.isValid());
  d->setField("url", remote);
  d->setField("name", "TestrepoSubmodule");
  d->setField("path", tempdir.path());
  d->setField("bare", "false");
  d->startClone();

  {
    auto timeout = Timeout(10e3, "Failed to clone");
    while (!cloneFinished)
      qWait(300);
  }

  QVERIFY(view);
  QCOMPARE(view->repo().submodules().count(), 1);
  for (const auto &s : view->repo().submodules()) {
    QVERIFY(s.isValid());
    QVERIFY(s.isInitialized());
  }

  // Close the window so it doesn't outlive this test: MainWindow::open()
  // heap-allocates it, and while alive it stays connected to the global
  // RecentRepositories signal, reacting to later tests' clones by
  // re-reading this repo's directory after tempdir (above) has been
  // deleted.
  window->close();
  qWait(0); // let the WA_DeleteOnClose deferred deletion run now
}

void TestSubmodule::noUpdateSubmoduleClone() {
  // Don't update submodules after cloning
  QString remote = Test::extractRepository("SubmoduleTest.zip");
  QCOMPARE(remote.isEmpty(), false);

  Settings *settings = Settings::instance();
  settings->setValue(Setting::Id::UpdateSubmodulesAfterPullAndClone, false);
  CloneDialog *d = new CloneDialog(CloneDialog::Kind::Clone);

  RepoView *view = nullptr;
  MainWindow *window = nullptr;

  bool cloneFinished = false;
  QObject::connect(d, &CloneDialog::accepted,
                   [d, &window, &view, &cloneFinished] {
                     cloneFinished = true;
                     window = MainWindow::open(d->path());
                     if (window) {
                       view = window->currentView();
                     }
                   });

  QTemporaryDir tempdir;
  QVERIFY(tempdir.isValid());
  d->setField("url", remote);
  d->setField("name", "TestrepoSubmodule");
  d->setField("path", tempdir.path());
  d->setField("bare", "false");
  d->startClone();

  {
    auto timeout = Timeout(10e3, "Failed to clone");
    while (!cloneFinished)
      qWait(300);
  }

  QVERIFY(view);
  QCOMPARE(view->repo().submodules().count(), 1);
  for (const auto &s : view->repo().submodules()) {
    QVERIFY(s.isValid());
    QCOMPARE(s.isInitialized(), false);
  }

  // Close the window so it doesn't outlive this test; see comment in
  // updateSubmoduleClone().
  window->close();
  qWait(0); // let the WA_DeleteOnClose deferred deletion run now
}

void TestSubmodule::discardFile() {
  // Discarding a file should not reset the submodule
  INIT_REPO("SubmoduleTest.zip");
  repoView->updateSubmodules(repo.submodules(), true, true);

  qWait(1000); // Not needed if the test is long enough and the fetch operation
               // finishes

  QCOMPARE(repo.submodules().count(), 1);
  for (const auto &submodule : repo.submodules())
    QVERIFY(submodule.isInitialized());

  {
    QFile file(repo.workdir().filePath("README.md"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "Changing readme of main repository" << Qt::endl;
    file.close();
  }

  {
    QFile file(repo.workdir().filePath("GittyupTestRepo/README.md"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "Changing content of submodule readme" << Qt::endl;
    file.close();
  }

  auto details = repoView->findChild<DetailView *>();
  QVERIFY(details);
  auto unstagedModel =
      static_cast<ChangedFilesModel *>(details->unstagedFiles());

  // Select head
  // Does not work
  // repoView->selectHead();
  // repoView->selectFirstCommit();

  refresh(repoView); // Do a refresh to simulate selecting the working directory
                     // entry in the commit list

  {
    // wait for refresh!
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (unstagedModel->rowCount() < 2 ||
           unstagedModel->rowOf("README.md") < 0)
      qWait(300);
  }

  {
    QCOMPARE(unstagedModel->rowCount(), 2);
    QVERIFY(unstagedModel->rowOf("GittyupTestRepo") >= 0);
    int readme = unstagedModel->rowOf("README.md");
    QVERIFY(readme >= 0);

    // Discard README.md and confirm.
    details->discardFiles(DetailView::UnstagedFiles, readme);
    ConfirmDialog *popup = repoView->findChild<ConfirmDialog *>();
    QVERIFY(popup);
    popup->accept();
  }

  QFile file(repo.workdir().filePath("GittyupTestRepo/README.md"));
  QVERIFY(file.open(QFile::ReadOnly));
  QCOMPARE(file.readAll(), "Changing content of submodule readme\n");
}

// A submodule whose repository is in its working directory keeps its
// commits when it's deinitialized, also the ones that its origin doesn't
// have: git moves the repository into the one of the parent first.
void TestSubmodule::deinitializeKeepsHistory() {
  QTemporaryDir root;
  QString origin = source(root);
  QString parent = root.filePath("parent");
  runGit(root.path(), {"init", "-q", parent});
  runGit(parent, {"clone", "-q", origin, "sm"});
  write(parent + "/sm/file.txt", "local\n");
  runGit(parent + "/sm", {"commit", "-qam", "local"});
  QString id = runGit(parent + "/sm", {"rev-parse", "HEAD"});
  runGit(parent, {"submodule", "add", "-q", origin, "sm"});
  runGit(parent, {"commit", "-qm", "Add sm"});
  QVERIFY(QFileInfo(parent + "/sm/.git").isDir());

  git::Repository repo = git::Repository::open(parent);
  git::Submodule submodule = repo.lookupSubmodule("sm");
  QVERIFY(submodule.isValid());
  submodule.deinitialize();

  int exitCode = -1;
  runGit(parent, {"config", "--get", "submodule.sm.url"}, &exitCode);
  QCOMPARE(exitCode, 1);
  QVERIFY(QDir(parent + "/sm").isEmpty());
  runGit(parent, {"--git-dir=.git/modules/sm", "cat-file", "-e", id},
         &exitCode);
  QCOMPARE(exitCode, 0);
}

// Deinitializing a submodule leaves the others, also the ones whose names
// it matches as a pattern.
void TestSubmodule::deinitializeOnlyItself() {
  QTemporaryDir root;
  QString origin = source(root);
  QString parent = root.filePath("parent");
  runGit(root.path(), {"init", "-q", parent});
  for (const QString &name : {"a.b", "axb"})
    runGit(parent, {"submodule", "add", "-q", origin, name});
  runGit(parent, {"commit", "-qm", "Add submodules"});

  git::Repository repo = git::Repository::open(parent);
  repo.lookupSubmodule("a.b").deinitialize();

  int exitCode = -1;
  runGit(parent, {"config", "--get", "submodule.a.b.url"}, &exitCode);
  QCOMPARE(exitCode, 1);
  QCOMPARE(runGit(parent, {"config", "--get", "submodule.axb.url"}), origin);
  QVERIFY(QFile::exists(parent + "/axb/file.txt"));
}

TEST_MAIN(TestSubmodule)

#include "Submodule.moc"
