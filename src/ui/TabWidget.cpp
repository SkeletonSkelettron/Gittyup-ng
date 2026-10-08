//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TabWidget.h"
#include "MenuBar.h"
#include "WelcomePage.h"
#include <QTabBar>

TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent) {
  // The tab bar only keeps the tab names.
  QTabBar *bar = new QTabBar(this);
  bar->setMovable(true);
  bar->setTabsClosable(true);
  setTabBar(bar);
  setDocumentMode(true);
  bar->hide();

  // Dialogs of the welcome page open on the main window.
  mWelcomePage = new WelcomePage(parent ? parent : this);
  connect(mWelcomePage, &WelcomePage::closeRequested, this,
          [this] { setWelcomeVisible(false); });
  updateWelcome();

  // Handle tab close.
  connect(this, &TabWidget::tabCloseRequested, [this](int index) {
    emit tabAboutToBeRemoved();
    widget(index)->close();
  });

  // Switching to a tab hides the welcome page.
  connect(this, &TabWidget::currentChanged, this,
          [this] { setWelcomeVisible(false); });
}

void TabWidget::setWelcomeVisible(bool visible) {
  if (visible == mWelcomeRequested)
    return;

  mWelcomeRequested = visible;
  updateWelcome();
}

void TabWidget::tabInserted(int index) {
  QTabWidget::tabInserted(index);
  MenuBar::instance(this)->updateWindow();
  emit tabInserted();

  mWelcomeRequested = false;
  updateWelcome();
}

void TabWidget::tabRemoved(int index) {
  QTabWidget::tabRemoved(index);
  MenuBar::instance(this)->updateWindow();
  emit tabRemoved();

  updateWelcome();
}

void TabWidget::updateWelcome() {
  bool visible = !count() || mWelcomeRequested;
  mWelcomePage->setClosable(count() > 0);
  if (visible != mWelcomeVisible) {
    mWelcomeVisible = visible;
    emit welcomeChanged();
  }
}
