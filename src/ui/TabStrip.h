//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef TABSTRIP_H
#define TABSTRIP_H

#include <QObject>
#include <QVariantList>

class MainWindow;
class TabWidget;
class QQuickWidget;

// The repository tabs above the tool bar. qrc:/qml/TabStrip.qml draws them
// from the tabs of the main window's TabWidget, whose own tab bar is hidden.
// The repository tabs above the tool bar, drawn by qrc:/qml/TabStrip.qml as
// 'tabStrip' in the view of the main window.
class TabStrip : public QObject {
  Q_OBJECT

  Q_PROPERTY(QVariantList tabs READ tabs NOTIFY tabsChanged)
  Q_PROPERTY(int current READ current NOTIFY tabsChanged)
  Q_PROPERTY(bool welcome READ welcome NOTIFY tabsChanged)

public:
  TabStrip(MainWindow *parent);
  ~TabStrip() override;

  void setTabWidget(TabWidget *tabs);

  // The view that draws the tabs, for the positions of menus.
  void setView(QQuickWidget *view) { mView = view; }

  QVariantList tabs() const { return mTabs; }
  int current() const { return mCurrent; }
  bool welcome() const { return mWelcome; }

  Q_INVOKABLE void selectTab(int index);
  Q_INVOKABLE void closeTab(int index);
  Q_INVOKABLE void newTab();
  Q_INVOKABLE void closeWelcome();
  Q_INVOKABLE void moveTab(int from, int to);
  Q_INVOKABLE void showMenu(int index, qreal x, qreal y);

signals:
  void tabsChanged();

private:
  // Update after the pending changes of the tab widget are done.
  void scheduleUpdate();
  void updateTabs();

  TabWidget *mTabWidget = nullptr;
  QQuickWidget *mView = nullptr;
  QVariantList mTabs;
  int mCurrent = -1;
  bool mWelcome = false;
  bool mUpdatePending = false;
};

#endif
