#include "Test.h"

#include "ui/CommitMessage.h"

class TestCommitEditor : public QObject {
  Q_OBJECT

private slots:
  void testCreateFileList();
  void applyTemplate1();
  void applyTemplate2();
};

void TestCommitEditor::testCreateFileList() {
  QCOMPARE(CommitMessage::fileList({"file.txt"}, 1),
           QStringLiteral("file.txt"));
  QCOMPARE(CommitMessage::fileList({"file.txt", "file2.txt"}, 1),
           QStringLiteral("file.txt, and 1 more file"));
  QCOMPARE(
      CommitMessage::fileList({"file.txt", "file2.txt", "file2.txt"}, 1),
      QStringLiteral("file.txt, and 2 more files"));
  QCOMPARE(CommitMessage::fileList({"file.txt", "file2.txt"}, 2),
           QStringLiteral("file.txt and file2.txt"));
  QCOMPARE(
      CommitMessage::fileList({"file.txt", "file2.txt", "file2.txt"}, 2),
      QStringLiteral("file.txt, file2.txt, and 1 more file"));
}

void TestCommitEditor::applyTemplate1() {
  CommitMessage::Result result = CommitMessage::applyTemplate(
      QStringLiteral("Description: %|\nfiles: ${files:3}"), {"file.txt"});
  QCOMPARE(result.text, QStringLiteral("Description: \nfiles: file.txt"));
  QCOMPARE(result.cursorPosition, 13);
}

void TestCommitEditor::applyTemplate2() {
  // Cursor after inserted files
  CommitMessage::Result result = CommitMessage::applyTemplate(
      QStringLiteral("Description: \nfiles: ${files:3}\nCursorPosition: %|"),
      {"reallylongfilename.txt"});
  QCOMPARE(
      result.text,
      QStringLiteral(
          "Description: \nfiles: reallylongfilename.txt\nCursorPosition: "));
  QCOMPARE(result.cursorPosition, 60);
}

TEST_MAIN(TestCommitEditor)
#include "commitEditor.moc"
