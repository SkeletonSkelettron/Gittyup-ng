//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Test.h"
#include "log/LogEntry.h"
#include "ui/LogPanel.h"
#include <QAbstractItemModel>
#include <QClipboard>
#include <QMimeData>
#include <QtTest/QtTest>

using namespace QTest;

class TestLog : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void copy();
  void copyNested();
  void copyAll();
  void cleanupTestCase();

private:
  QStringList clipboardLines() const;

  LogPanel *mPanel = nullptr;
};

void TestLog::initTestCase() {
  LogEntry *rootEntry = new LogEntry;
  rootEntry->addEntry("Entry", "Title");
  for (LogEntry::Kind kind :
       {LogEntry::File, LogEntry::Hint, LogEntry::Warning, LogEntry::Error})
    rootEntry->addEntry(kind, "Entry");
  LogEntry *nestedEntry = rootEntry->addEntry("Entry", "Nested 1");
  nestedEntry->addEntry(LogEntry::Entry, "Nested 2")
      ->addEntry(LogEntry::File, "Nested 3")
      ->addEntry(LogEntry::File, "Message");

  mPanel = new LogPanel(rootEntry);
  QCOMPARE(mPanel->model()->rowCount(), 6);
}

QStringList TestLog::clipboardLines() const {
  const QMimeData *data = QApplication::clipboard()->mimeData();
  return data ? data->text().split('\n', Qt::SkipEmptyParts) : QStringList();
}

void TestLog::copy() {
  // Each top-level entry without children copies as one line.
  QAbstractItemModel *model = mPanel->model();
  for (int i = 0; i < 5; ++i) {
    mPanel->copy(model->index(i, 0));
    QStringList lines = clipboardLines();
    QCOMPARE(lines.size(), 1);
    QVERIFY(lines.first().endsWith("Entry"));
    QVERIFY(!lines.first().contains('<'));
  }

  // Titles are kept without markup.
  mPanel->copy(model->index(0, 0));
  QVERIFY(clipboardLines().first().endsWith("Title - Entry"));

  // The rich text keeps the markup.
  QVERIFY(QApplication::clipboard()->mimeData()->html().contains("<b>Title</b>"));
}

void TestLog::copyNested() {
  // Nested entries are indented.
  QAbstractItemModel *model = mPanel->model();
  mPanel->copy(model->index(5, 0));
  QStringList lines = clipboardLines();
  QCOMPARE(lines.size(), 4);
  QVERIFY(lines.at(0).endsWith("Nested 1 - Entry"));
  QCOMPARE(lines.at(1), QString("    Nested 2"));
  QCOMPARE(lines.at(2), QString("        Nested 3"));
  QCOMPARE(lines.at(3), QString("            Message"));
}

void TestLog::copyAll() {
  mPanel->copyAll();
  QCOMPARE(clipboardLines().size(), 9);
}

void TestLog::cleanupTestCase() { delete mPanel; }

TEST_MAIN(TestLog)

#include "log.moc"
