#include "Test.h"
#include "ui/ChangedFilesModel.h"
#include "ui/DetailView.h"
#include "ui/DiffModel.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "conf/Settings.h"
#include "git/Patch.h"

using namespace Test;
using namespace QTest;

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
  DetailView *details = repoView->findChild<DetailView *>();                   \
  QVERIFY(details);                                                            \
  details->setListMode(false);

namespace {

// The rows of a file model as paths, with a trailing slash for directories.
QStringList rows(QAbstractItemModel *model) {
  QStringList result;
  for (int i = 0; i < model->rowCount(); ++i) {
    QModelIndex index = model->index(i, 0);
    QString path = index.data(ChangedFilesModel::PathRole).toString();
    if (index.data(ChangedFilesModel::IsDirRole).toBool())
      path += '/';
    result.append(path);
  }
  return result;
}

ChangedFilesModel *model(DetailView *details, DetailView::List list) {
  QAbstractItemModel *model = list == DetailView::StagedFiles
                                  ? details->stagedFiles()
                                  : details->unstagedFiles();
  return static_cast<ChangedFilesModel *>(model);
}

// The row of the first file called 'name'.
int fileRow(ChangedFilesModel *model, const QString &name) {
  for (int i = 0; i < model->rowCount(); ++i) {
    QModelIndex index = model->index(i, 0);
    if (!index.data(ChangedFilesModel::IsDirRole).toBool() &&
        index.data(ChangedFilesModel::NameRole).toString() == name)
      return i;
  }
  return -1;
}

} // namespace

class TestTreeView : public QObject {
  Q_OBJECT

private slots:
  void restoreStagedFileAfterCommit();
  void discardFiles();
  void fileMergeCrash();
  void selectionSurvivesPush();
  void dirtySubmoduleAndStagedSubmodule();
  void conflictedAndStagedFile();

private:
};

void TestTreeView::restoreStagedFileAfterCommit() {
  INIT_REPO("TreeViewCollapseCount.zip");

  ChangedFilesModel *unstagedModel = model(details, DetailView::UnstagedFiles);
  {
    // Wait for refresh
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (unstagedModel->rowCount() < 1)
      qWait(10);
  }

  QCOMPARE(unstagedModel->fileCount(), 2);

  // Stage file.txt.
  int row = fileRow(unstagedModel, "file.txt");
  QVERIFY(row >= 0);
  details->selectFile(DetailView::UnstagedFiles, row);
  details->stageFiles(DetailView::UnstagedFiles, row, true);

  refresh(repoView, true);

  // Select it in the staged files.
  ChangedFilesModel *stagedModel = model(details, DetailView::StagedFiles);
  QCOMPARE(stagedModel->fileCount(), 1);
  row = fileRow(stagedModel, "file.txt");
  QVERIFY(row >= 0);
  details->selectFile(DetailView::StagedFiles, row);

  details->setCommitMessage("conflicting commit b");
  repoView->commit();

  // The application should not crash!
}

void TestTreeView::discardFiles() {
  // staging single files and discard files afterwards. It should not discard
  // not selected files Discarding a folder in staged treeview should only
  // delete the staged files, but not the unstaged files in that folder!

  INIT_REPO("TestRepository.zip");

  git::Commit commit =
      repo.lookupCommit("5c61b24e236310ad4a8a64f7cd1ccc968f1eec20");
  QVERIFY(commit);

  // modifying all files
  QHash<QString, QString> fileContent{
      {"file.txt", "Modified file"},
      {"file2.txt", "Modified file2"},
      {"folder1/file.txt", "Modified file in folder1"},
      {"folder1/file2.txt", "Modified file2 in folder1"},
      {"GittyupTestRepo/README.md", "Modified readme in submodule"},
  };
  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::WriteOnly));
      file.write(i.value().toLatin1());
    }
  }

  // refresh repo
  refresh(repoView);

  // let the changes settle
  QApplication::processEvents();

  // stage folder1/file.txt
  ChangedFilesModel *unstagedModel = model(details, DetailView::UnstagedFiles);
  {
    // Wait for refresh
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (unstagedModel->rowCount() < 1)
      qWait(10);
  }

  QCOMPARE(unstagedModel->fileCount(), 5);
  int row = unstagedModel->rowOf("folder1/file.txt");
  QVERIFY(row >= 0);
  details->stageFiles(DetailView::UnstagedFiles, row, true);

  refresh(repoView, true);

  // Only folder1/file.txt is staged.
  ChangedFilesModel *stagedModel = model(details, DetailView::StagedFiles);
  QCOMPARE(rows(stagedModel),
           QStringList({"folder1/", "folder1/file.txt"}));

  // Discarding the staged folder1 must only discard folder1/file.txt, not
  // folder1/file2.txt.
  QCOMPARE(stagedModel->files(0), QStringList({"folder1/file.txt"}));

  // From here on everything is tested in TestFileContextMenu
}

void TestTreeView::fileMergeCrash() {
  INIT_REPO("CrashMerge.zip");

  git::Reference otherBranch = repo.lookupRef("refs/heads/otherBranch");
  QVERIFY(otherBranch);

  git::Reference master =
      repo.lookupRef(QString("refs/heads/%1").arg("master"));
  QVERIFY(master);

  QCOMPARE(repo.head().name(), "master");

  repoView->merge(RepoView::Merge, otherBranch);

  // Diff is in a conflicted state
  git::Diff diff = repo.diffIndexToWorkdir();
  QVERIFY(diff.isConflicted());

  ChangedFilesModel *stagedModel = model(details, DetailView::StagedFiles);
  {
    // Wait for refresh
    auto timeout = Timeout(10000, "Repository didn't refresh in time");
    while (stagedModel->fileCount() < 1)
      qWait(10);
  }

  // The conflicted file.
  ChangedFilesModel *unstagedModel = model(details, DetailView::UnstagedFiles);
  int row = -1;
  for (int i = 0; row < 0 && i < unstagedModel->rowCount(); ++i) {
    QModelIndex index = unstagedModel->index(i, 0);
    if (index.data(ChangedFilesModel::StatusRole).toString() == "!")
      row = i;
  }
  QVERIFY(row >= 0);
  QCOMPARE(unstagedModel->index(row, 0)
               .data(ChangedFilesModel::NameRole)
               .toString(),
           QString("File_security_configs"));
  details->selectFile(DetailView::UnstagedFiles, row);

  auto diffModel = qobject_cast<DiffModel *>(details->diffModel());
  QVERIFY(diffModel);
  QVERIFY(diffModel->isConflicted());
  QVERIFY(diffModel->hunkCount() > 0);

  diffModel->chooseConflict(0, git::Patch::Theirs);
  diffModel->saveConflict(0);

  // should not crash
}

void TestTreeView::selectionSurvivesPush() {
  // Pushing must not leave the diff/file views permanently blank: they're
  // allowed to clear while the push's resulting ref update is processed,
  // but must come back populated afterward.
  QTemporaryDir remoteDir;
  QVERIFY(remoteDir.isValid());
  git::Repository remoteRepo = git::Repository::init(remoteDir.path(), true);
  QVERIFY(remoteRepo.isValid());

  INIT_REPO("TreeViewCollapseCount.zip");

  git::Remote remote = repo.addRemote("origin", remoteDir.path());
  QVERIFY(remote.isValid());

  // Set up tracking with an initial push, same as any already-established
  // branch would have. This first push takes the setUpstream=true path,
  // which (unlike a plain push) also goes through a HEAD-related ref
  // update
  repoView->push(remote, git::Reference(), QString(), true, false, false);
  {
    auto timeout = Timeout(10000, "Initial push didn't complete in time");
    while (!remoteRepo.lookupRef("refs/heads/master").isValid())
      qWait(10);
  }

  // Create a second commit so the real push below has something to send
  {
    QFile file(repo.workdir().filePath("newfile.txt"));
    QVERIFY(file.open(QFile::WriteOnly));
    file.write("content");
    file.close();
    repo.index().setStaged({"newfile.txt"}, true);
    QVERIFY(repo.commit("second commit"));
  }

  // Wait for the resulting selection/diff to settle before the real push.
  {
    auto timeout = Timeout(10000, "Selection didn't settle in time");
    while (!repoView->diff().isValid())
      qWait(10);
  }

  // Pushing more commits on an already-tracked branch, i.e. setUpstream=false.
  // That only updates the remote-tracking ref, not HEAD, so it must not leave
  // the diff/file views permanently blank.
  repoView->push(remote, git::Reference(), QString(), false, false, false);

  // The diff view must come back populated, not stay cleared. Without the
  // fix this hits the Timeout below and aborts.
  auto timeout =
      Timeout(10000, "Diff view didn't get repopulated after the push");
  while (!repoView->diff().isValid())
    qWait(10);
}

void TestTreeView::dirtySubmoduleAndStagedSubmodule() {
  INIT_REPO("DirtySubmoduleUnstagedTree.zip");

  {
    ChangedFilesModel *stagedModel = model(details, DetailView::StagedFiles);
    {
      // Wait for refresh
      auto timeout = Timeout(10000, "Repository didn't refresh in time");
      while (stagedModel->rowCount() < 1)
        qWait(10);
    }

    QCOMPARE(rows(stagedModel),
             QStringList({"submodules/", "submodules/submodule1"}));
  }

  {
    ChangedFilesModel *unstagedModel =
        model(details, DetailView::UnstagedFiles);
    {
      // Wait for refresh
      auto timeout = Timeout(10000, "Repository didn't refresh in time");
      while (unstagedModel->rowCount() < 1)
        qWait(300);
    }

    QCOMPARE(rows(unstagedModel),
             QStringList({"submodules/", "submodules/submodule2"}));
  }
}

void TestTreeView::conflictedAndStagedFile() {
  INIT_REPO("ConflictedAndStagedFile.zip");

  {
    ChangedFilesModel *stagedModel = model(details, DetailView::StagedFiles);
    {
      // Wait for refresh
      auto timeout = Timeout(10000, "Repository didn't refresh in time");
      while (stagedModel->rowCount() < 1)
        qWait(10);
    }

    QCOMPARE(rows(stagedModel),
             QStringList({"folder/", "folder/NotConflictedFile.txt"}));
  }

  {
    ChangedFilesModel *unstagedModel =
        model(details, DetailView::UnstagedFiles);
    {
      // Wait for refresh
      auto timeout = Timeout(10000, "Repository didn't refresh in time");
      while (unstagedModel->rowCount() < 1)
        qWait(300);
    }

    QCOMPARE(rows(unstagedModel),
             QStringList({"folder/", "folder/conflictedFile.txt"}));
  }
}

TEST_MAIN(TestTreeView)

#include "TreeView.moc"
