//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "git/Repository.h"
#include <QMainWindow>
#include <QVariantMap>

class MenuBar;
class QQmlContext;
class QQuickItem;
class QQuickWidget;
class RepoView;
class CommandPalette;
class SideBar;
class TabWidget;
class ToolBar;

namespace git {
class Submodule;
}

class TabStrip;

// A window with repositories in tabs. Everything but the menu bar is drawn
// by qrc:/qml/MainPage.qml in a single view, which reads the window as
// 'mainWindow'. The pages of the repositories are created in that view.
class MainWindow : public QMainWindow {
  Q_OBJECT

  Q_PROPERTY(bool sideBarVisible READ isSideBarVisible NOTIFY
                 sideBarVisibleChanged)
  Q_PROPERTY(bool welcomeVisible READ isWelcomeVisible NOTIFY
                 welcomeVisibleChanged)
  // The view draws the menu bar unless the platform shows the menus.
  Q_PROPERTY(bool menuBarVisible READ isMenuBarVisible NOTIFY
                 menuBarVisibleChanged)
  Q_PROPERTY(QStringList menuTitles READ menuTitles CONSTANT)

public:
  MainWindow(const git::Repository &repo, QWidget *parent = nullptr,
             Qt::WindowFlags flags = Qt::WindowFlags());

  ToolBar *toolBar() const { return mToolBar; }
  // Finds commands, branches, files and repositories.
  CommandPalette *commandPalette() const { return mPalette; }

  ~MainWindow() override;

  bool isSideBarVisible() const;
  void setSideBarVisible(bool visible);

  bool isWelcomeVisible() const;

  bool isMenuBarVisible() const;
  QStringList menuTitles() const;
  // Show the menu of the menu bar at 'index' below a point of the view.
  Q_INVOKABLE void showMenu(int index, qreal x, qreal y);

  // The view that draws the window.
  QQuickWidget *quickView() const { return mView; }

  // Map a point in the scene of the view to global coordinates.
  QPoint mapFromScene(qreal x, qreal y) const;

  // Create a page from qrc:/qml/<name>.qml with its own context objects in
  // the view. It's hidden until it's shown. The page and its context belong
  // to 'owner', which has to delete them before the objects of the context.
  QQuickItem *createPage(const QString &name, const QVariantMap &objects,
                         QObject *owner, QQmlContext **context);

  TabWidget *tabWidget() const;
  RepoView *addTab(const QString &path);
  RepoView *addTab(const git::Repository &repo);

  int count() const;
  RepoView *currentView() const;
  RepoView *view(int index) const;

  // Get the "active" main window.
  static MainWindow *activeWindow();
  static QList<MainWindow *> windows();

  // Restore previous open window state.
  // Returns true if any windows were opened.
  static bool restoreWindows();

  // Open a new window.
  static MainWindow *open(const QString &path, bool warnOnInvalid = true);
  static MainWindow *open(const git::Repository &repo = git::Repository());

  // Save window settings on close.
  static void setSaveWindowSettings(bool enabled);

signals:
  void sideBarVisibleChanged();
  void welcomeVisibleChanged();
  void menuBarVisibleChanged();

protected:
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  void updatePages();
  void updateTabNames();
  void updateInterface();
  void updateWindowTitle(int ahead = -1, int behind = -1);

  static void warnInvalidRepo(const QString &path);

  QStringList paths() const;
  QString windowGroup() const;

  TabWidget *mTabs;
  TabStrip *mTabStrip;
  ToolBar *mToolBar;
  SideBar *mSideBar;
  CommandPalette *mPalette;
  MenuBar *mMenuBar;
  QQuickWidget *mView;

  bool mFullPath = false;
  bool mIsSideBarVisible = true;

  bool mShown = false;
  bool mClosing = false;

  static bool sSaveWindowSettings;
};

#endif
