#include "Test.h"
#include "conf/Settings.h"
#include "ui/DetailView.h"
#include "ui/DiffModel.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"

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

TEST_MAIN(TestDiffView)

#include "DiffView.moc"
