//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Claude Opus 5.5, an AI model by Anthropic
//

#include "Test.h"
#include "conf/Settings.h"
#include "ui/MainWindow.h"
#include "ui/WindowFrame.h"
#include <QQuickItem>
#include <QQuickWidget>
#include <QSignalSpy>

using namespace Test;
using namespace QTest;

// The view draws the title bar and the edges of the window in the colors of
// the theme, unless the setting asks for the title bar of the system.
class TestWindowFrame : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void customByDefault();
  void maximize();
  void systemTitleBar();
  void cleanupTestCase();

private:
  QQuickItem *item(const QString &type) const;

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
  WindowFrame *mFrame = nullptr;
};

void TestWindowFrame::initTestCase() {
  Settings::instance()->setValue(Setting::Id::SystemTitleBar, QVariant());

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));

  mFrame = mWindow->findChild<WindowFrame *>();
  QVERIFY(mFrame);
}

void TestWindowFrame::customByDefault() {
  QVERIFY(mFrame->isCustom());
  QVERIFY(mWindow->windowFlags().testFlag(Qt::FramelessWindowHint));
  QCOMPARE(mFrame->title(), mWindow->windowTitle());

  // The menu row is the title bar, and the edges resize the window.
  QQuickItem *bar = item("MenuBarRow");
  QVERIFY(bar);
  QVERIFY(bar->property("titleBar").toBool());
  QQuickItem *edges = item("WindowEdges");
  QVERIFY(edges);
  QVERIFY(edges->isVisible());
}

// A maximized window has no edges to resize it.
void TestWindowFrame::maximize() {
  QSignalSpy maximized(mFrame, &WindowFrame::maximizedChanged);
  QQuickItem *edges = item("WindowEdges");

  mFrame->toggleMaximized();
  QTRY_VERIFY(mWindow->isMaximized());
  QVERIFY(mFrame->isMaximized());
  QVERIFY(maximized.count() >= 1);
  QTRY_VERIFY(!edges->isVisible());

  mFrame->toggleMaximized();
  QTRY_VERIFY(!mWindow->isMaximized());
  QTRY_VERIFY(edges->isVisible());
}

// The setting brings back the title bar of the system, and the window stays
// open while it changes.
void TestWindowFrame::systemTitleBar() {
  QSignalSpy custom(mFrame, &WindowFrame::customChanged);
  QQuickItem *bar = item("MenuBarRow");

  Settings::instance()->setValue(Setting::Id::SystemTitleBar, true);
  QCOMPARE(custom.count(), 1);
  QVERIFY(!mFrame->isCustom());
  QVERIFY(!mWindow->windowFlags().testFlag(Qt::FramelessWindowHint));
  QVERIFY(mWindow->isVisible());
  QVERIFY(!bar->property("titleBar").toBool());
  QVERIFY(!item("WindowEdges")->isVisible());

  Settings::instance()->setValue(Setting::Id::SystemTitleBar, false);
  QCOMPARE(custom.count(), 2);
  QVERIFY(mFrame->isCustom());
  QVERIFY(mWindow->windowFlags().testFlag(Qt::FramelessWindowHint));
  QVERIFY(mWindow->isVisible());
  QVERIFY(bar->property("titleBar").toBool());
}

void TestWindowFrame::cleanupTestCase() {
  Settings::instance()->setValue(Setting::Id::SystemTitleBar, QVariant());
  mWindow->close();
}

// The item of a QML type of the main page.
QQuickItem *TestWindowFrame::item(const QString &type) const {
  QQuickItem *root = mWindow->quickView()->rootObject();
  for (QQuickItem *item : root->findChildren<QQuickItem *>()) {
    if (QString(item->metaObject()->className()).startsWith(type + "_"))
      return item;
  }

  return nullptr;
}

TEST_MAIN(TestWindowFrame)

#include "window_frame.moc"
