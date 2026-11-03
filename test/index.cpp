//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "qtsupport.h"
#include "Test.h"
#include "dialogs/ConfirmDialog.h"
#include "ui/ChangedFilesModel.h"
#include "ui/DetailView.h"
#include "ui/DiffModel.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include "git2/sys/filter.h"
#include "git2/sys/errors.h"
#include <QFile>
#include <QProcess>
#include <QTextStream>

using namespace Test;
using namespace QTest;

static git::Index::StagedState stageState(QAbstractItemModel *model, int row) {
  return static_cast<git::Index::StagedState>(
      model->index(row, 0).data(ChangedFilesModel::StageStateRole).toInt());
}

// The visible confirmation dialog of the view.
static ConfirmDialog *confirmDialog(QWidget *parent) {
  for (ConfirmDialog *dialog : parent->findChildren<ConfirmDialog *>()) {
    if (dialog->isVisible())
      return dialog;
  }
  return nullptr;
}

namespace {

// A required clean driver that can fail after the preview was loaded, without
// depending on a shell or changing the user's global filter configuration.
class FailingCleanFilter {
public:
  FailingCleanFilter() {
    filter.attributes = "filter=gittyup-index-required";
    filter.check = [](git_filter *self, void **,
                      const git_filter_source *source, const char **) {
      auto *driver = reinterpret_cast<FailingCleanFilter *>(self);
      return driver->fail && git_filter_source_mode(source) == GIT_FILTER_TO_ODB
                 ? 0
                 : GIT_PASSTHROUGH;
    };
    filter.stream = [](git_writestream **, git_filter *, void **,
                       const git_filter_source *, git_writestream *) {
      git_error_set_str(GIT_ERROR_FILTER, "Required clean filter failed");
      return static_cast<int>(GIT_EUSER);
    };
    registered = !git_filter_register("gittyup-index-required", &filter,
                                      GIT_FILTER_DRIVER_PRIORITY);
  }

  ~FailingCleanFilter() {
    if (registered)
      git_filter_unregister("gittyup-index-required");
  }

  git_filter filter = GIT_FILTER_INIT;
  bool fail = false;
  bool registered = false;
};

} // namespace

class TestIndex : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void stageAddition();
  void stageDeletion();
  void stageDirectory();
  void discardUntracked();
  void stageKeepsStagedByGit();
  void stageLinesOfNewFile();
  void stageLinesRejectsCleanFailure_data();
  void stageLinesRejectsCleanFailure();
  void cleanupTestCase();

private:
  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
};

void TestIndex::initTestCase() {
  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowActive(mWindow));
}

void TestIndex::stageAddition() {
  // Add file and refresh.
  QFile file(mRepo->workdir().filePath("test"));
  QVERIFY(file.open(QFile::WriteOnly));
  QTextStream(&file) << "This is a test." << Qt::endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);

  details->setListMode(false);

  QAbstractItemModel *unstagedModel = details->unstagedFiles();
  QCOMPARE(unstagedModel->rowCount(), 1);

  // Check that it starts unstaged.
  QCOMPARE(stageState(unstagedModel, 0), git::Index::Unstaged);

  // Stage it.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);
  QCoreApplication::processEvents();

  QAbstractItemModel *stagedModel = details->stagedFiles();
  QCOMPARE(stagedModel->rowCount(), 1);
  QCOMPARE(unstagedModel->rowCount(), 0);

  // Check that it's staged now.
  QCOMPARE(stageState(stagedModel, 0), git::Index::Staged);

  // Commit and refresh.
  details->setCommitMessage("addition");
  view->commit();
  refresh(view, false);
}

void TestIndex::stageDeletion() {
  // Remove file and refresh.
  mRepo->workdir().remove("test");

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);


  QAbstractItemModel *unstagedModel = details->unstagedFiles();
  QCOMPARE(unstagedModel->rowCount(), 1);

  // Check that it starts unstaged.
  QCOMPARE(stageState(unstagedModel, 0), git::Index::Unstaged);

  // Stage it.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);
  QCoreApplication::processEvents();

  QAbstractItemModel *stagedModel = details->stagedFiles();
  QCOMPARE(stagedModel->rowCount(), 1);
  QCOMPARE(unstagedModel->rowCount(), 0);

  // Check that it's staged now.
  QCOMPARE(stageState(stagedModel, 0), git::Index::Staged);

  // Commit and refresh.
  details->setCommitMessage("deletion");
  view->commit();
  refresh(view, false);
}

void TestIndex::stageDirectory() {
  QDir dir = mRepo->workdir();
  dir.mkdir("dir");
  QVERIFY(dir.cd("dir"));

  QFile file1(dir.filePath("test1"));
  QVERIFY(file1.open(QFile::WriteOnly));
  QTextStream(&file1) << "This is a test." << Qt::endl;

  QFile file2(dir.filePath("test2"));
  QVERIFY(file2.open(QFile::WriteOnly));
  QTextStream(&file2) << "This is a test." << Qt::endl;

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);

  details->setListMode(false);

  // The directory followed by its two files.
  QAbstractItemModel *unstagedModel = details->unstagedFiles();
  QCOMPARE(unstagedModel->rowCount(), 3);
  QVERIFY(unstagedModel->index(0, 0)
              .data(ChangedFilesModel::IsDirRole)
              .toBool());

  // Check that they start unstaged.
  QCOMPARE(stageState(unstagedModel, 1), git::Index::Unstaged);
  QCOMPARE(stageState(unstagedModel, 2), git::Index::Unstaged);

  // Stage the directory.
  details->stageFiles(DetailView::UnstagedFiles, 0, true);
  QCoreApplication::processEvents();

  // Check for two staged files.
  QAbstractItemModel *stagedModel = details->stagedFiles();
  QCOMPARE(stagedModel->rowCount(), 3);
  QCOMPARE(unstagedModel->rowCount(), 0);

  // Check that they're staged now.
  QCOMPARE(stageState(stagedModel, 1), git::Index::Staged);
  QCOMPARE(stageState(stagedModel, 2), git::Index::Staged);
}

// Discarding an untracked file removes it, from the diff and from the list.
void TestIndex::discardUntracked() {
  QDir dir = mRepo->workdir();
  for (const QString &name : {"untracked1", "untracked2"}) {
    QFile file(dir.filePath(name));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "This is a test." << Qt::endl;
  }

  RepoView *view = mWindow->currentView();
  refresh(view);

  auto details = view->findChild<DetailView *>();
  QVERIFY(details);

  // The button above the diff.
  details->discardFile("untracked1");
  ConfirmDialog *dialog = confirmDialog(view);
  QVERIFY(dialog);
  dialog->accept();
  QVERIFY(!dir.exists("untracked1"));

  // The button in the row of the list.
  refresh(view);
  auto unstagedModel =
      static_cast<ChangedFilesModel *>(details->unstagedFiles());
  int row = unstagedModel->rowOf("untracked2");
  QVERIFY(row >= 0);
  details->discardFiles(DetailView::UnstagedFiles, row);
  dialog = confirmDialog(view);
  QVERIFY(dialog);
  dialog->accept();
  QVERIFY(!dir.exists("untracked2"));
}

// Staging keeps what git staged since the index was read.
void TestIndex::stageKeepsStagedByGit() {
  QDir dir = mRepo->workdir();
  for (const QString &name : {"mine", "theirs"}) {
    QFile file(dir.filePath(name));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "This is a test." << Qt::endl;
  }

  RepoView *view = mWindow->currentView();
  refresh(view);

  QProcess git;
  git.setWorkingDirectory(dir.path());
  git.start("git", {"add", "theirs"});
  QVERIFY(git.waitForFinished());
  QCOMPARE(git.exitCode(), 0);

  git::Index index = mRepo->index();
  index.setStaged({"mine"}, true);

  git.start("git", {"diff", "--cached", "--name-only"});
  QVERIFY(git.waitForFinished());
  QStringList staged =
      QString::fromUtf8(git.readAllStandardOutput()).split('\n');
  QVERIFY(staged.contains("mine"));
  QVERIFY(staged.contains("theirs"));
  QCOMPARE(index.isStaged("theirs"), git::Index::Staged);
}

// Lines of a new file are staged like git adds the file, with the line
// endings that its attributes ask for.
void TestIndex::stageLinesOfNewFile() {
  ScratchRepository repo;
  QDir dir = repo->workdir();
  auto git = [&dir](const QStringList &args) {
    QProcess process;
    process.setWorkingDirectory(dir.path());
    process.start("git", args);
    process.waitForFinished();
    return process.readAllStandardOutput();
  };

  QFile attributes(dir.filePath(".gitattributes"));
  QVERIFY(attributes.open(QFile::WriteOnly));
  attributes.write("*.txt text eol=lf\n");
  attributes.close();
  git({"add", ".gitattributes"});
  git({"commit", "-qm", "Add attributes"});

  QFile file(dir.filePath("file.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("first\r\nsecond\r\n");
  file.close();

  MainWindow window(repo);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));
  git::Index index = repo->index();
  index.read();
  DiffModel model(window.currentView());
  model.setDiff(repo->status(index, nullptr, false), "file.txt");
  QCOMPARE(model.hunkCount(), 1);

  model.toggleLine(model.row(0, 0));
  QTRY_COMPARE(git({"show", ":file.txt"}), QByteArray("first\n"));
}

void TestIndex::stageLinesRejectsCleanFailure_data() {
  QTest::addColumn<bool>("failAtLoad");
  QTest::newRow("when-loading") << true;
  QTest::newRow("after-loading") << false;
}

// A required clean failure never stages raw content or changes the worktree,
// even when the driver starts failing after a usable preview was loaded.
void TestIndex::stageLinesRejectsCleanFailure() {
  QFETCH(bool, failAtLoad);
  ScratchRepository repo;
  FailingCleanFilter driver;
  QVERIFY(driver.registered);
  QDir dir = repo->workdir();
  auto write = [&dir](const QString &name, const QByteArray &content) {
    QFile file(dir.filePath(name));
    return file.open(QFile::WriteOnly) && file.write(content) == content.size();
  };
  auto read = [](const QString &path) {
    QFile file(path);
    return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
  };
  auto git = [&dir](const QStringList &args) {
    QProcess process;
    process.setWorkingDirectory(dir.path());
    process.start("git", args);
    process.waitForFinished();
    return process.readAllStandardOutput();
  };

  QVERIFY(write(".gitattributes", "file.txt filter=gittyup-index-required\n"));
  repo->gitConfig().setValue("filter.gittyup-index-required.required", true);
  repo->index().setStaged({".gitattributes"}, true);
  QVERIFY(repo->commit("Add attributes"));
  QVERIFY(write("staged.txt", "keep staged\n"));
  repo->index().setStaged({"staged.txt"}, true);
  QVERIFY(write("file.txt", "first\nsecond\n"));

  MainWindow window(repo);
  git::Index index = repo->index();
  index.read();
  const QString indexPath = repo->dir().filePath("index");
  const QByteArray beforeIndex = read(indexPath);
  QVERIFY(!beforeIndex.isEmpty());
  DiffModel model(window.currentView());
  driver.fail = failAtLoad;
  model.setDiff(repo->status(index, nullptr, false), "file.txt");
  QCOMPARE(model.hunkCount(), 1);

  if (failAtLoad) {
    QVERIFY(!model.isEditable());
  } else {
    QVERIFY(model.isEditable());
    driver.fail = true;
  }
  model.toggleLine(model.row(0, 0));
  QTRY_VERIFY(!model.isEditable());
  QVERIFY(model.notice().contains("Required clean filter failed"));
  QCOMPARE(read(indexPath), beforeIndex);
  QCOMPARE(read(dir.filePath("file.txt")), QByteArray("first\nsecond\n"));
  QCOMPARE(git({"diff", "--cached", "--name-only"}),
           QByteArray("staged.txt\n"));
  QCOMPARE(git({"show", ":staged.txt"}), QByteArray("keep staged\n"));

  // A corrected driver allows a fresh preview to be staged normally.
  driver.fail = false;
  model.setDiff(repo->status(index, nullptr, false), "file.txt");
  QVERIFY(model.isEditable());
  QVERIFY(model.notice().isEmpty());
  model.toggleLine(model.row(0, 0));
  QTRY_COMPARE(git({"show", ":file.txt"}), QByteArray("first\n"));
}

void TestIndex::cleanupTestCase() { mWindow->close(); }

TEST_MAIN(TestIndex)

#include "index.moc"
