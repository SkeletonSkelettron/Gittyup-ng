#include "Test.h"
#include "conf/Settings.h"
#include "ui/DetailView.h"
#include "ui/DiffModel.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "git/Patch.h"
#include "git/Reference.h"
#include "git/Tree.h"

using namespace Test;
using namespace QTest;

namespace {

// Far more hunks than fit in the viewport.
const int kHunkCount = 60;

QMap<QString, QString> contents(bool manyFiles, const QString &state) {
  QMap<QString, QString> files;
  if (manyFiles) {
    for (int i = 0; i < kHunkCount; ++i) {
      QString name = QString("files/file%1.txt").arg(i, 3, 10, QChar('0'));
      files.insert(name, state + "\n");
    }
    return files;
  }

  // Enough unchanged lines between the edits to keep each one in its own hunk.
  QString text;
  for (int i = 0; i < kHunkCount; ++i) {
    for (int j = 0; j < 10; ++j)
      text += QString("line %1.%2\n").arg(i).arg(j);
    text += QString("%1 %2\n").arg(state).arg(i);
  }
  files.insert("files/hunks.txt", text);
  return files;
}

bool writeFiles(const QDir &workdir, const QMap<QString, QString> &files) {
  for (auto it = files.begin(); it != files.end(); ++it) {
    QFile file(workdir.filePath(it.key()));
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
      return false;
    file.write(it.value().toUtf8());
  }
  return true;
}

} // namespace

class TestDiffView : public QObject {
  Q_OBJECT

private slots:
  void loadsAllHunks_data();
  void loadsAllHunks();
  void wholeFile_data();
  void wholeFile();
};

void TestDiffView::loadsAllHunks_data() {
  QTest::addColumn<bool>("manyFiles");
  QTest::addColumn<bool>("committed");

  QTest::newRow("many files, working tree") << true << false;
  QTest::newRow("many files, commit") << true << true;
  QTest::newRow("many hunks in one file, working tree") << false << false;
  QTest::newRow("many hunks in one file, commit") << false << true;
}

// The diff panel shows one file at a time, with all of its hunks.
void TestDiffView::loadsAllHunks() {
  QFETCH(bool, manyFiles);
  QFETCH(bool, committed);

  ScratchRepository repo;
  QDir workdir = repo->workdir();
  QVERIFY(workdir.mkpath("files"));

  auto before = contents(manyFiles, "before");
  QVERIFY(writeFiles(workdir, before));
  repo->index().setStaged(before.keys(), true);
  QVERIFY(repo->commit("initial"));

  QVERIFY(writeFiles(workdir, contents(manyFiles, "after")));
  if (committed) {
    repo->index().setStaged(before.keys(), true);
    QVERIFY(repo->commit("change"));
  }

  MainWindow window(repo);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));

  RepoView *repoView = window.currentView();
  if (committed)
    repoView->selectFirstCommit();

  auto details = repoView->findChild<DetailView *>();
  QVERIFY(details);

  QAbstractItemModel *files =
      committed ? details->files() : details->unstagedFiles();
  QTRY_COMPARE_WITH_TIMEOUT(
      static_cast<int>(files->property("fileCount").toInt()),
      static_cast<int>(before.size()), 10000);

  // Select the last file.
  QString file = before.lastKey();
  details->selectPath(file);
  QCOMPARE(details->file(), file);

  auto diff = qobject_cast<DiffModel *>(details->diffModel());
  QVERIFY(diff);
  QCOMPARE(diff->path(), file);
  QCOMPARE(diff->hunkCount(), manyFiles ? 1 : kHunkCount);

  // Every changed line is there.
  QCOMPARE(diff->additions(), manyFiles ? 1 : kHunkCount);
  QCOMPARE(diff->deletions(), manyFiles ? 1 : kHunkCount);

  // Closing the file shows the graph again.
  details->closeFile();
  QVERIFY(details->file().isEmpty());
}

void TestDiffView::wholeFile_data() {
  QTest::addColumn<bool>("committed");

  QTest::newRow("working tree") << false;
  QTest::newRow("commit") << true;
}

// The whole file is a single hunk with the lines that didn't change, until
// another file is shown. Its lines are staged like those of the diff.
void TestDiffView::wholeFile() {
  QFETCH(bool, committed);

  auto text = [](const QMap<int, QString> &changes) {
    QString text;
    for (int i = 1; i <= 30; ++i)
      text += changes.value(i, QString("line %1").arg(i)) + "\n";
    return text;
  };

  ScratchRepository repo;
  QDir workdir = repo->workdir();
  QVERIFY(writeFiles(workdir, {{"file.txt", text({})}, {"other.txt", "a\n"}}));
  repo->index().setStaged({"file.txt", "other.txt"}, true);
  QVERIFY(repo->commit("initial"));

  QMap<int, QString> changes = {{5, "changed 5"}, {25, "changed 25"}};
  QVERIFY(writeFiles(workdir, {{"file.txt", text(changes)},
                               {"other.txt", "b\n"},
                               {"new.txt", "new\n"}}));
  if (committed) {
    repo->index().setStaged({"file.txt", "other.txt", "new.txt"}, true);
    QVERIFY(repo->commit("change"));
  }

  MainWindow window(repo);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));

  RepoView *repoView = window.currentView();
  if (committed)
    repoView->selectFirstCommit();

  auto details = repoView->findChild<DetailView *>();
  QVERIFY(details);

  QAbstractItemModel *files =
      committed ? details->files() : details->unstagedFiles();
  QTRY_COMPARE_WITH_TIMEOUT(files->property("fileCount").toInt(), 3, 10000);

  auto diff = qobject_cast<DiffModel *>(details->diffModel());
  QVERIFY(diff);

  // Added and untracked files are shown in full anyway.
  details->selectPath("new.txt");
  QCOMPARE(diff->path(), QString("new.txt"));
  QVERIFY(!diff->canShowWholeFile());

  details->selectPath("file.txt");
  QVERIFY(diff->canShowWholeFile());
  QVERIFY(!diff->isWholeFile());
  QCOMPARE(diff->hunkCount(), 2);

  diff->setWholeFile(true);
  QVERIFY(diff->isWholeFile());
  QCOMPARE(diff->hunkCount(), 1);
  QCOMPARE(diff->status(), QString("M"));
  QCOMPARE(diff->additions(), 2);
  QCOMPARE(diff->deletions(), 2);
  // The header, 28 lines that didn't change and the two changed lines.
  QCOMPARE(diff->rowCount(), 1 + 28 + 2 * 2);
  QList<DiffLines::Line> lines = diff->lines(0);
  QCOMPARE(lines.first().content, QByteArray("line 1\n"));
  QCOMPARE(lines.last().content, QByteArray("line 30\n"));

  if (!committed) {
    // Stage only the change of line 5.
    int row = -1;
    for (int i = 0; i < lines.size() && row < 0; ++i) {
      if (lines.at(i).content == "changed 5\n")
        row = diff->row(0, i);
    }
    QVERIFY(row > 0);
    diff->setLinesStaged(row - 1, row, true);

    git::Commit head = repo->head().target();
    auto staged = [&repo, &head] {
      git::Diff staged = repo->diffTreeToIndex(head.tree());
      int index = staged.indexOf("file.txt");
      return index < 0 ? git::Patch() : staged.patch(index);
    };
    QTRY_VERIFY(staged().isValid());
    git::Patch patch = staged();
    QCOMPARE(patch.lineStats().additions, 1);
    QCOMPARE(patch.lineStats().deletions, 1);
    QCOMPARE(patch.count(), 1);
    QCOMPARE(repo->index().isStaged("file.txt"), git::Index::PartiallyStaged);
  }

  // The lines that didn't change are gone for another file.
  details->selectPath("other.txt");
  QVERIFY(!diff->isWholeFile());
  details->selectPath("file.txt");
  QVERIFY(!diff->isWholeFile());
  QCOMPARE(diff->hunkCount(), 2);

  details->closeFile();
}

TEST_MAIN(TestDiffView)

#include "DiffView.moc"
