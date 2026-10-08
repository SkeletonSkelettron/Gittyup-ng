//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include "git/Blob.h"
#include "git/Commit.h"
#include "git/Repository.h"

class FileEditor;
class QQuickWidget;

// A window to edit a file, or to look at a version of it, drawn by
// qrc:/qml/EditorPage.qml, which reads the window as 'editorWindow'.
class EditorWindow : public QMainWindow {
  Q_OBJECT

  // The view draws the menu bar unless the platform shows the menus.
  Q_PROPERTY(bool menuBarVisible READ isMenuBarVisible NOTIFY
                 menuBarVisibleChanged)
  Q_PROPERTY(QStringList menuTitles READ menuTitles CONSTANT)

public:
  EditorWindow(const git::Repository &repo = git::Repository(),
               QWidget *parent = nullptr);
  ~EditorWindow() override;

  FileEditor *editor() const { return mEditor; }

  bool isMenuBarVisible() const;
  QStringList menuTitles() const;
  // Show the menu of the menu bar at 'index' below a point of the view.
  Q_INVOKABLE void showMenu(int index, qreal x, qreal y);

  void updateWindowTitle();

  static EditorWindow *open(const QString &path,
                            const git::Blob &blob = git::Blob(),
                            const git::Commit &commit = git::Commit(),
                            const git::Repository &repo = git::Repository());

signals:
  void menuBarVisibleChanged();

protected:
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private:
  FileEditor *mEditor;
  QQuickWidget *mView;
};

#endif
