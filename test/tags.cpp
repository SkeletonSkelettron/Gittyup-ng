//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Claude Opus 5.5, an AI model by Anthropic
//

#include "Test.h"
#include "dialogs/DeleteTagDialog.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "git/Tag.h"
#include "git/TagRef.h"
#include "log/LogEntry.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QFile>
#include <QSignalSpy>

using namespace Test;
using namespace QTest;

// Deletes tags, by themselves and from the dialog of the view.
class TestTags : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void removeLightweight();
  void removeAnnotated();
  void removeMissing();
  void removeFromDialog();
  void removeMissingFromDialog();
  void cleanupTestCase();

private:
  git::TagRef createTag(const QString &name,
                        const QString &message = QString());
  void deleteFromDialog(const git::TagRef &tag);
  LogEntry *logRoot() const;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  RepoView *mView = nullptr;
};

void TestTags::initTestCase() {
  QFile file(mRepo->workdir().filePath("file.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("content\n");
  file.close();

  mRepo->index().setStaged({"file.txt"}, true);
  QVERIFY(mRepo->commit("initial").isValid());

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));
  mView = mWindow->currentView();
}

void TestTags::removeLightweight() {
  git::TagRef tag = createTag("light");
  QVERIFY(tag.isValid());

  QSignalSpy removed(mRepo->notifier(),
                     &git::RepositoryNotifier::referenceRemoved);
  QVERIFY(tag.remove());

  // The tag is gone, and so is this reference to it.
  QVERIFY(!mRepo->lookupTag("light").isValid());
  QVERIFY(!tag.isValid());
  QCOMPARE(removed.count(), 1);
  QCOMPARE(removed.first().first().toString(), QString("light"));
}

void TestTags::removeAnnotated() {
  git::TagRef tag = createTag("annotated", "An annotated tag");
  QVERIFY(tag.isValid());
  QVERIFY(tag.tag().isValid());

  QVERIFY(tag.remove());
  QVERIFY(!mRepo->lookupTag("annotated").isValid());
  QVERIFY(!tag.isValid());
}

// A tag that was deleted behind its back stays valid, so that callers can
// tell that it wasn't deleted, and the views still hear about it.
void TestTags::removeMissing() {
  git::TagRef stale = createTag("gone");
  QVERIFY(mRepo->lookupTag("gone").remove());

  QSignalSpy removed(mRepo->notifier(),
                     &git::RepositoryNotifier::referenceRemoved);
  QVERIFY(!stale.remove());
  QVERIFY(stale.isValid());
  QCOMPARE(removed.count(), 1);
}

void TestTags::removeFromDialog() {
  git::TagRef tag = createTag("dialog");
  QSignalSpy errors(logRoot(), &LogEntry::errorInserted);

  deleteFromDialog(tag);

  QVERIFY(!mRepo->lookupTag("dialog").isValid());
  QCOMPARE(errors.count(), 0);
}

// The dialog reports a tag that can't be deleted, once.
void TestTags::removeMissingFromDialog() {
  git::TagRef stale = createTag("missing");
  QVERIFY(mRepo->lookupTag("missing").remove());
  QSignalSpy errors(logRoot(), &LogEntry::errorInserted);

  deleteFromDialog(stale);

  QCOMPARE(errors.count(), 1);
}

void TestTags::cleanupTestCase() {
  delete mWindow;
  mWindow = nullptr;
}

git::TagRef TestTags::createTag(const QString &name, const QString &message) {
  return mRepo->createTag(mRepo->head().target(), name, message);
}

void TestTags::deleteFromDialog(const git::TagRef &tag) {
  // Without a remote, the dialog doesn't offer to delete the upstream tag.
  DeleteTagDialog *dialog = new DeleteTagDialog(tag, mView);
  dialog->accept();
  dialog->deleteLater();
}

// The root of the activity log, which its model owns.
LogEntry *TestTags::logRoot() const {
  for (LogEntry *entry : mView->findChildren<LogEntry *>()) {
    if (!entry->parentEntry())
      return entry;
  }

  return nullptr;
}

TEST_MAIN(TestTags)

#include "tags.moc"
