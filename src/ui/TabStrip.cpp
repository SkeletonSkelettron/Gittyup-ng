//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "TabStrip.h"
#include "MainWindow.h"
#include "RepoView.h"
#include "TabWidget.h"
#include "qml/QmlSupport.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QMenu>
#include <QPointer>
#include <QQuickWidget>
#include <QTabBar>
#include <QTimer>

TabStrip::TabStrip(MainWindow *parent) : QObject(parent) {}

TabStrip::~TabStrip() {}

void TabStrip::setTabWidget(TabWidget *tabs) {
  mTabWidget = tabs;
  connect(tabs, &TabWidget::currentChanged, this, &TabStrip::scheduleUpdate);
  connect(tabs, QOverload<>::of(&TabWidget::tabInserted), this,
          &TabStrip::scheduleUpdate);
  connect(tabs, QOverload<>::of(&TabWidget::tabRemoved), this,
          &TabStrip::scheduleUpdate);
  connect(tabs, &TabWidget::welcomeChanged, this, &TabStrip::scheduleUpdate);
  connect(tabs->tabBar(), &QTabBar::tabMoved, this, &TabStrip::scheduleUpdate);
  updateTabs();
}

void TabStrip::selectTab(int index) {
  if (!mTabWidget || index < 0 || index >= mTabWidget->count())
    return;

  mTabWidget->setCurrentIndex(index);
  mTabWidget->setWelcomeVisible(false);
}

void TabStrip::closeTab(int index) {
  if (!mTabWidget || index < 0 || index >= mTabWidget->count())
    return;

  // Closing can ask to save the files of editor windows.
  QPointer<QWidget> tab = mTabWidget->widget(index);
  QmlSupport::callLater(this, [this, tab] { closeTab(tab); });
}

void TabStrip::closeTab(QWidget *tab) {
  int index = mTabWidget->indexOf(tab);
  if (index >= 0)
    emit mTabWidget->tabCloseRequested(index);
}

void TabStrip::newTab() {
  if (mTabWidget)
    mTabWidget->setWelcomeVisible(true);
}

void TabStrip::closeWelcome() {
  if (mTabWidget)
    mTabWidget->setWelcomeVisible(false);
}

void TabStrip::moveTab(int from, int to) {
  if (!mTabWidget || from == to || from < 0 || to < 0 ||
      from >= mTabWidget->count() || to >= mTabWidget->count())
    return;

  mTabWidget->tabBar()->moveTab(from, to);
}

void TabStrip::showMenu(int index, qreal x, qreal y) {
  if (!mTabWidget || index < 0 || index >= mTabWidget->count())
    return;

  QPointer<QWidget> tab = mTabWidget->widget(index);
  QPoint pos = QmlSupport::host(mView)->mapToGlobal(x, y);
  QmlSupport::callLater(this, [this, tab, pos] {
    if (tab)
      showMenu(tab, pos);
  });
}

void TabStrip::showMenu(QWidget *widget, const QPoint &pos) {
  int index = mTabWidget->indexOf(widget);
  if (index < 0)
    return;

  // The tabs can change while the menu is shown.
  QPointer<QWidget> tab = widget;
  QMenu menu;
  menu.addAction(tr("Close Tab"), this, [this, tab] { closeTab(tab); });

  QAction *others = menu.addAction(tr("Close Other Tabs"), this, [this, tab] {
    for (int i = mTabWidget->count() - 1; tab && i >= 0; --i) {
      if (mTabWidget->widget(i) != tab)
        closeTab(mTabWidget->widget(i));
    }
  });
  others->setEnabled(mTabWidget->count() > 1);

  QAction *right =
      menu.addAction(tr("Close Tabs to the Right"), this, [this, tab] {
        int index = mTabWidget->indexOf(tab);
        for (int i = mTabWidget->count() - 1; index >= 0 && i > index; --i)
          closeTab(mTabWidget->widget(i));
      });
  right->setEnabled(index < mTabWidget->count() - 1);

  menu.addSeparator();

  RepoView *view = static_cast<RepoView *>(widget);
  QString path = view->repo().dir(false).path();
  menu.addAction(tr("Copy Path"), this, [path] {
    QApplication::clipboard()->setText(QDir::toNativeSeparators(path));
  });

  QmlSupport::execMenu(&menu, pos);
}

void TabStrip::scheduleUpdate() {
  if (mUpdatePending)
    return;

  // The tab names are updated after the tab is inserted or removed.
  mUpdatePending = true;
  QTimer::singleShot(0, this, &TabStrip::updateTabs);
}

void TabStrip::updateTabs() {
  mUpdatePending = false;
  if (!mTabWidget)
    return;

  QVariantList tabs;
  for (int i = 0; i < mTabWidget->count(); ++i) {
    RepoView *view = static_cast<RepoView *>(mTabWidget->widget(i));
    tabs.append(QVariantMap{
        {"name", mTabWidget->tabText(i)},
        {"path", QDir::toNativeSeparators(view->repo().dir(false).path())}});
  }

  mTabs = tabs;
  mCurrent = mTabWidget->currentIndex();
  mWelcome = mTabWidget->isWelcomeVisible();
  emit tabsChanged();
}
