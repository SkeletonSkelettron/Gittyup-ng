//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QModelIndex>
#include <QObject>

class Account;
class MainWindow;
class QAbstractItemModel;
class QMenu;
class QQuickWidget;
class TabWidget;

// The repository sidebar. The list is drawn by qrc:/qml/SideBar.qml as
// 'sidebar' in the view of the main window.
class SideBar : public QObject {
  Q_OBJECT

  Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)

public:
  SideBar(TabWidget *tabs, MainWindow *mainWindow);
  ~SideBar() override;

  // The view that draws the sidebar, for the positions of menus.
  void setView(QQuickWidget *view) { mView = view; }

  QAbstractItemModel *model() const { return mModel; }

  // Single click.
  Q_INVOKABLE void activate(const QModelIndex &index);
  // Double click.
  Q_INVOKABLE void open(const QModelIndex &index);
  Q_INVOKABLE void remove(const QModelIndex &index);
  Q_INVOKABLE void showContextMenu(const QModelIndex &index, qreal x, qreal y);
  Q_INVOKABLE void showAddMenu(qreal x, qreal y);
  Q_INVOKABLE void showOptionsMenu(qreal x, qreal y);

  // Persist the expansion state of sections and remote accounts.
  Q_INVOKABLE bool isExpanded(const QModelIndex &index) const;
  Q_INVOKABLE void setExpanded(const QModelIndex &index, bool expanded);

private:
  void hideAfterOpen();
  void promptToRemoveAccount(Account *account);

  TabWidget *mTabs;
  MainWindow *mMainWindow;
  QAbstractItemModel *mModel;
  QQuickWidget *mView = nullptr;
  QMenu *mAddMenu;
  QMenu *mOptionsMenu;
};

#endif
