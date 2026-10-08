//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Test.h"
#include "conf/Settings.h"
#include "dialogs/ConfirmDialog.h"
#include "ui/EditorWindow.h"
#include "ui/FileEditor.h"
#include "ui/FindController.h"
#include "ui/MenuBar.h"
#include <QQuickItem>
#include <QQuickWidget>

using namespace QTest;

class TestEditor : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void insertText();
  void copyPaste();
  void indent();
  void find();
  void cleanupTestCase();

private:
  QString text() const;

  EditorWindow *mWindow = nullptr;
  QQuickWidget *mView = nullptr;
  QQuickItem *mTextArea = nullptr;
};

void TestEditor::initTestCase() {
  mWindow = new EditorWindow;
  mWindow->show();
  QVERIFY(qWaitForWindowActive(mWindow));

  mView = mWindow->findChild<QQuickWidget *>();
  QVERIFY(mView);
  for (QQuickItem *item : mView->rootObject()->findChildren<QQuickItem *>()) {
    if (item->inherits("QQuickTextEdit"))
      mTextArea = item;
  }
  QVERIFY(mTextArea);
  QTRY_VERIFY(mTextArea->hasActiveFocus());
}

void TestEditor::insertText() {
  keyClicks(mView, "This is a test.");
  keyClick(mView, Qt::Key_Return);
  QCOMPARE(text(), QString("This is a test.\n"));
  QVERIFY(mWindow->editor()->isModified());
}

void TestEditor::copyPaste() {
  keyClick(mView, 'A', Qt::ControlModifier);
  keyClick(mView, 'C', Qt::ControlModifier);
  keyClick(mView, Qt::Key_Right);
  keyClick(mView, 'V', Qt::ControlModifier);
  QCOMPARE(text(), QString("This is a test.\nThis is a test.\n"));
}

void TestEditor::indent() {
  // The tab key indents with spaces or a tab, like the settings say.
  Settings *settings = Settings::instance();
  bool tabs = settings->value(Setting::Id::UseTabsForIndent).toBool();
  int width = settings->value(Setting::Id::IndentWidth).toInt();

  keyClick(mView, Qt::Key_End, Qt::ControlModifier);
  keyClick(mView, Qt::Key_Tab);
  QString indent = tabs ? QString("\t") : QString(width, ' ');
  QVERIFY(text().endsWith("\n" + indent));

  keyClick(mView, 'A', Qt::ControlModifier);
  keyClick(mView, Qt::Key_Right);
  keyClick(mView, Qt::Key_Backspace);
  for (int i = 1; i < indent.length(); ++i)
    keyClick(mView, Qt::Key_Backspace);
  QCOMPARE(text(), QString("This is a test.\nThis is a test.\n"));
}

void TestEditor::find() {
  // It's difficult to trigger the shortcut on macOS.
  MenuBar *menuBar = MenuBar::instance(mWindow);
  QAction *findAction = menuBar->findChild<QAction *>("Find");
  QVERIFY(findAction);
  findAction->trigger();

  FindController *finder =
      qobject_cast<FindController *>(mWindow->editor()->finder());
  QVERIFY(finder && finder->isVisible());

  keyClicks(mView, "test");
  QCOMPARE(finder->hitsText(), QString("1 of 2"));

  // The first match is selected.
  QCOMPARE(mTextArea->property("selectedText").toString(), QString("test"));
}

void TestEditor::cleanupTestCase() {
  // Set up timer to dismiss the dialog.
  QTimer::singleShot(0, [] {
    ConfirmDialog *dialog =
        qobject_cast<ConfirmDialog *>(QApplication::activeModalWidget());
    QVERIFY(dialog && qWaitForWindowActive(dialog));

    // Don't save.
    QCOMPARE(dialog->buttons().size(), 1);
    dialog->clickButton(0);
    QVERIFY(!dialog->isVisible());
  });

  mWindow->close();
}

QString TestEditor::text() const {
  return mTextArea->property("text").toString();
}

TEST_MAIN(TestEditor)

#include "editor.moc"
