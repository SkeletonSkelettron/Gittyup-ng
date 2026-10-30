#include "Test.h"

#include "ui/FileContextMenu.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/IgnoreDialog.h"
#include "git/Reference.h"

#include "dialogs/ConfirmDialog.h"
#include "log/LogEntry.h"
#include <QProcess>
#include <QPushButton>

#define INIT_REPO(repoPath)                                                    \
  QString path = Test::extractRepository(repoPath);                            \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());                                                     \
  MainWindow window(repo);                                                     \
  window.show();                                                               \
  QVERIFY(QTest::qWaitForWindowExposed(&window));                              \
  RepoView *repoView = window.currentView();

namespace {

// The visible confirmation dialog of the view.
ConfirmDialog *confirmDialog(QWidget *parent) {
  for (ConfirmDialog *dialog : parent->findChildren<ConfirmDialog *>()) {
    if (dialog->isVisible())
      return dialog;
  }
  return nullptr;
}

} // namespace

class TestFileContextMenu : public QObject {
  Q_OBJECT

private slots:
  void testDiscardFile();
  void testDiscardSubmodule();
  void testDiscardFolder();
  void testDiscardNothing();
  void testIgnoreFile();
  void testIgnoreFileUntracked();
  void testIgnoreFolder();
  void testRemoveUntrackedFolder();
  void testCheckoutKeepsLocalFiles();
};

using namespace git;

void TestFileContextMenu::testDiscardFile() {
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

  // refresh repo and wait for the (asynchronous) status/selection update to
  // actually finish, instead of hoping a fixed sleep was long enough.
  Test::refresh(repoView);

  QStringList files = {"file.txt"};
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QVERIFY(action->isEnabled());
  action->triggered(true);

  auto *dialog = confirmDialog(repoView);
  QVERIFY(dialog);
  dialog->accept();

  // original text
  //  {"file.txt", "File.txt\n"},
  //  {"file2.txt", "file2.txt\n"},
  //  {"folder1/file.txt", "file in folder1\n"},
  //  {"folder1/file2.txt", "file2 in folder1\n"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},

  {
    QHash<QString, QString> fileContentRef = fileContent;
    fileContentRef.insert("file.txt", "File.txt\n"); // this file was discarded
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testDiscardSubmodule() {
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

  // refresh repo and wait for the (asynchronous) status/selection update to
  // actually finish, instead of hoping a fixed sleep was long enough.
  Test::refresh(repoView);

  QStringList files = {"GittyupTestRepo"};
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QCOMPARE(action->isEnabled(), true);
  action->triggered(true);

  auto *dialog = confirmDialog(repoView);
  QVERIFY(dialog);
  dialog->accept();

  // The submodule is updated on another thread, by git.
  QString readme = repo.workdir().filePath("GittyupTestRepo/README.md");
  auto content = [readme] {
    QFile file(readme);
    return file.open(QFile::ReadOnly) ? QString::fromUtf8(file.readAll())
                                      : QString();
  };
  QTRY_COMPARE_WITH_TIMEOUT(
      content(),
      QString(
          "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"),
      10000);

  // original text
  //  {"file.txt", "File.txt\n"},
  //  {"file2.txt", "file2.txt\n"},
  //  {"folder1/file.txt", "file in folder1\n"},
  //  {"folder1/file2.txt", "file2 in folder1\n"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},

  {
    QHash<QString, QString> fileContentRef = fileContent;
    fileContentRef.insert("GittyupTestRepo/README.md",
                          "# GittyupTestRepo\nTest repo for Gittyup used in "
                          "the unittests\n"); // this submodule was discarded
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testDiscardFolder() {
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

  // refresh repo and wait for the (asynchronous) status/selection update to
  // actually finish, instead of hoping a fixed sleep was long enough.
  Test::refresh(repoView);

  QStringList files = {"folder1"};
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QCOMPARE(action->isEnabled(), true);
  action->triggered(true);

  auto *dialog = confirmDialog(repoView);
  QVERIFY(dialog);
  dialog->accept();

  // original text
  //  {"file.txt", "File.txt\n"},
  //  {"file2.txt", "file2.txt\n"},
  //  {"folder1/file.txt", "file in folder1\n"},
  //  {"folder1/file2.txt", "file2 in folder1\n"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},

  {
    QHash<QString, QString> fileContentRef = fileContent;
    // files in the folder got discarded
    fileContentRef.insert("folder1/file.txt", "file in folder1\n");
    fileContentRef.insert("folder1/file2.txt", "file2 in folder1\n");
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testDiscardNothing() {
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

  // refresh repo and wait for the (asynchronous) status/selection update to
  // actually finish, instead of hoping a fixed sleep was long enough.
  Test::refresh(repoView);

  QStringList files; // no files passed
  FileContextMenu m(repoView, files, repo.index());

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->text() == tr("Discard Changes")) {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QCOMPARE(action->isEnabled(), false);

  // original text
  //  {"file.txt", "File.txt\n"},
  //  {"file2.txt", "file2.txt\n"},
  //  {"folder1/file.txt", "file in folder1\n"},
  //  {"folder1/file2.txt", "file2 in folder1\n"},
  //  {"GittyupTestRepo/README.md",
  //   "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},

  {
    // No file shall be discarded
    QHash<QString, QString> fileContentRef = fileContent;
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

void TestFileContextMenu::testIgnoreFile() {
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

  // refresh repo and wait for the (asynchronous) status/selection update to
  // actually finish, instead of hoping a fixed sleep was long enough.
  Test::refresh(repoView);

  QStringList files = {"file.txt"};
  FileContextMenu m(repoView, files, repo.index(), repoView);

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->objectName() == "IgnoreAction") {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QCOMPARE(action->isEnabled(), false); // Modified file cannot be ignored
}

void TestFileContextMenu::testIgnoreFileUntracked() {
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

  {
    // Create new file which shall be ignored
    QFile file(repo.workdir().filePath("newFile.txt"));
    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.write("Content of new file") > 0);
  }

  // refresh repo and wait for the (asynchronous) status/selection update to
  // actually finish, instead of hoping a fixed sleep was long enough.
  Test::refresh(repoView);

  QStringList files = {"newFile.txt"};
  FileContextMenu m(repoView, files, repo.index(), repoView);

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->objectName() == "IgnoreAction") {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QVERIFY(action->isEnabled());
  action->triggered(true);

  auto *ignoreDialog = repoView->findChild<IgnoreDialog *>();
  QVERIFY(ignoreDialog);
  emit ignoreDialog->accepted();

  QFile file(repo.workdir().filePath(".gitignore"));
  QVERIFY(file.exists());
  QVERIFY(file.open(QFile::ReadOnly));
  const QString ignores = file.readAll();

#ifdef Q_OS_WIN
  QCOMPARE(ignores, "newFile.txt\r\n");
#else
  QCOMPARE(ignores, "newFile.txt\n");
#endif
}

void TestFileContextMenu::testIgnoreFolder() {
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

  // refresh repo and wait for the (asynchronous) status/selection update to
  // actually finish, instead of hoping a fixed sleep was long enough.
  Test::refresh(repoView);

  QStringList files = {"folder1"};
  FileContextMenu m(repoView, files, repo.index(), repoView);

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->objectName() == "IgnoreAction") {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QVERIFY(action->isEnabled());
  action->triggered(true);

  auto *ignoreDialog = repoView->findChild<IgnoreDialog *>();
  QVERIFY(ignoreDialog);
  emit ignoreDialog->accepted();

  QFile file(repo.workdir().filePath(".gitignore"));
  QVERIFY(file.exists());
  QVERIFY(file.open(QFile::ReadOnly));
  const QString ignores = file.readAll();

#ifdef Q_OS_WIN
  QCOMPARE(ignores, "folder1\r\n");
#else
  QCOMPARE(ignores, "folder1\n");
#endif
}

void TestFileContextMenu::testRemoveUntrackedFolder() {
  INIT_REPO("TestRepository.zip");

  git::Commit commit =
      repo.lookupCommit("5c61b24e236310ad4a8a64f7cd1ccc968f1eec20");
  QVERIFY(commit);

  // modifying all files
  QHash<QString, QString> fileContent{
      {"folder_new/file.txt", "Modified file"},
      {"folder_new/file2.txt", "Modified file2"},
  };

  repo.workdir().mkdir("folder_new");

  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.open(QFile::WriteOnly));
      file.write(i.value().toLatin1());
    }
  }

  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QCOMPARE(file.exists(), true);
    }
  }

  // refresh repo and wait for the (asynchronous) status/selection update to
  // actually finish, instead of hoping a fixed sleep was long enough.
  Test::refresh(repoView);

  QStringList files = {"folder_new"};
  FileContextMenu m(repoView, files, repo.index(), repoView);

  QAction *action = nullptr;
  for (auto *a : m.actions()) {
    if (a->objectName() == "RemoveAction") {
      action = a;
      break;
    }
  }
  QVERIFY(action);
  QVERIFY(action->isEnabled());
  action->triggered(true);

  // Click remove in dialog
  auto *dialog = confirmDialog(repoView);
  QVERIFY(dialog);
  dialog->accept();

  // Check that files do not exist anymore
  {
    QHashIterator<QString, QString> i(fileContent);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QCOMPARE(file.exists(), false);
    }
  }

  // Check that other files are still available
  {
    // No file shall be discarded
    QHash<QString, QString> fileContentRef = {
        {"file.txt", "File.txt\n"},
        {"file2.txt", "file2.txt\n"},
        {"folder1/file.txt", "file in folder1\n"},
        {"folder1/file2.txt", "file2 in folder1\n"},
        {"GittyupTestRepo/README.md",
         "# GittyupTestRepo\nTest repo for Gittyup used in the unittests\n"},
    };
    QHashIterator<QString, QString> i(fileContentRef);
    while (i.hasNext()) {
      i.next();
      QFile file(repo.workdir().filePath(i.key()));
      QVERIFY(file.exists());
      QVERIFY(file.open(QFile::ReadOnly));
      QVERIFY2(file.readAll() == i.value(), qPrintable(i.key()));
    }
  }
}

// Checking out files of a commit keeps untracked files and changes, staged
// ones too like renames, also of files in the way of their directories and
// of directories in the way of them, like libgit2 did. The view reports them.
void TestFileContextMenu::testCheckoutKeepsLocalFiles() {
  Test::ScratchRepository repo;
  QDir dir = repo->workdir();
  auto runGit = [&dir](const QStringList &args) {
    QProcess process;
    process.setWorkingDirectory(dir.path());
    process.start("git", args);
    process.waitForFinished();
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
  };
  auto write = [&dir](const QString &name, const QByteArray &content) {
    dir.mkpath(QFileInfo(name).path());
    QFile file(dir.filePath(name));
    return file.open(QFile::WriteOnly) && file.write(content) >= 0;
  };
  auto read = [&dir](const QString &name) {
    QFile file(dir.filePath(name));
    return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
  };

  // The second commit deletes or changes the files of the first.
  QStringList changed = {"changed.txt", "kept/file.txt", "staged.txt",
                         "renamed.txt"};
  for (const QString &name :
       changed + QStringList{"deleted.txt", "dir/file.txt", "e"})
    QVERIFY(write(name, "old\n"));
  runGit({"add", "."});
  runGit({"commit", "-q", "-m", "first"});
  git::Commit first = repo->head().target();

  runGit({"rm", "-q", "-r", "deleted.txt", "dir", "e"});
  for (const QString &name : changed + QStringList{"e/x"})
    QVERIFY(write(name, "new\n"));
  runGit({"add", "."});
  runGit({"commit", "-q", "-m", "second"});

  QVERIFY(write("deleted.txt", "untracked\n"));
  QVERIFY(write("dir", "untracked\n"));
  QVERIFY(write("e/x", "changed\n"));
  QVERIFY(QDir(dir.filePath("kept")).removeRecursively());
  QVERIFY(write("staged.txt", "staged\n"));
  runGit({"add", "staged.txt"});
  runGit({"mv", "renamed.txt", "moved.txt"});

  int strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_DONT_UPDATE_INDEX;
  for (const QString &path : {"deleted.txt", "dir/file.txt", "e", "kept",
                              "staged.txt", "renamed.txt"})
    QVERIFY(!repo->checkout(first, nullptr, {path}, strategy));
  for (const QString &path : {"staged.txt", "renamed.txt"})
    QVERIFY(!repo->checkout(first, nullptr, {path}, GIT_CHECKOUT_SAFE));
  QCOMPARE(read("deleted.txt"), QByteArray("untracked\n"));
  QCOMPARE(read("dir"), QByteArray("untracked\n"));
  QCOMPARE(read("e/x"), QByteArray("changed\n"));
  QVERIFY(!dir.exists("kept"));
  QCOMPARE(read("staged.txt"), QByteArray("staged\n"));
  QCOMPARE(runGit({"show", ":staged.txt"}), QString("staged"));
  QVERIFY(!dir.exists("renamed.txt"));
  QCOMPARE(runGit({"status", "--porcelain", "--", "renamed.txt", "moved.txt"}),
           QString("R  renamed.txt -> moved.txt"));

  MainWindow window(repo);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  RepoView *view = window.currentView();
  view->checkout(first, QStringList{"deleted.txt"});
  QCOMPARE(read("deleted.txt"), QByteArray("untracked\n"));

  QStringList reported;
  for (LogEntry *entry : view->findChildren<LogEntry *>()) {
    if (entry->kind() != LogEntry::Error)
      continue;

    for (LogEntry *file : entry->entries())
      reported.append(file->text());
  }
  QCOMPARE(reported, QStringList({"deleted.txt"}));

  // Files without local changes are checked out, the index stays.
  QVERIFY(repo->checkout(first, nullptr, {"changed.txt"}, strategy));
  QCOMPARE(read("changed.txt"), QByteArray("old\n"));
  QCOMPARE(runGit({"status", "--porcelain", "--", "changed.txt"}),
           QString("M changed.txt"));
}

TEST_MAIN(TestFileContextMenu)
#include "fileContextMenu.moc"
