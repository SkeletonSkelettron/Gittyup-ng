//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "EditorWindow.h"
#include "FileEditor.h"
#include "MenuBar.h"
#include "conf/Settings.h"
#include "dialogs/ConfirmDialog.h"
#include "git/Reference.h"
#include "qml/QmlSupport.h"
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QQuickWidget>

EditorWindow::EditorWindow(const git::Repository &repo, QWidget *parent)
    : QMainWindow(parent), mEditor(new FileEditor(repo, this)) {
  setAttribute(Qt::WA_DeleteOnClose);
  resize(800, 800);

  connect(mEditor, &FileEditor::fileChanged, this,
          &EditorWindow::updateWindowTitle);
  connect(mEditor, &FileEditor::modifiedChanged, this,
          [this] { setWindowModified(mEditor->isModified()); });
  connect(mEditor, &FileEditor::saved, [repo] {
    if (!repo.isValid())
      return;

    // Notify window that the head branch is changed.
    emit repo.notifier()->referenceUpdated(repo.head());
  });

  // Connect menu bar actions. The view draws the menu bar unless it's
  // native, and the actions are added to the window for their shortcuts.
  if (MenuBar *menuBar = MenuBar::instance(this)) {
    connect(mEditor, &FileEditor::modifiedChanged, menuBar,
            &MenuBar::updateSave);
    menuBar->registerActions(this);
    if (!menuBar->isNativeMenuBar())
      menuBar->hide();
  }

  connect(Settings::instance(), &Settings::settingsChanged, this,
          &EditorWindow::menuBarVisibleChanged);

  mView = QmlSupport::createView(
      "EditorPage",
      {{"editor", QVariant::fromValue<QObject *>(mEditor)},
       {"editorWindow", QVariant::fromValue<QObject *>(this)}},
      this);
  QmlSupport::setDrawsPopups(mView, true);
  setCentralWidget(mView);
  mView->setFocus();
}

EditorWindow::~EditorWindow() {
  // The QML view references the editor, so it has to go first.
  delete mView;
}

bool EditorWindow::isMenuBarVisible() const {
  MenuBar *menuBar = qobject_cast<MenuBar *>(this->menuBar());
  return menuBar && !menuBar->isNativeMenuBar() &&
         !Settings::instance()->value(Setting::Id::HideMenuBar).toBool();
}

QStringList EditorWindow::menuTitles() const {
  QStringList titles;
  if (MenuBar *menuBar = qobject_cast<MenuBar *>(this->menuBar())) {
    for (QMenu *menu : menuBar->menus())
      titles.append(menu->title());
  }

  return titles;
}

void EditorWindow::showMenu(int index, qreal x, qreal y) {
  MenuBar *menuBar = qobject_cast<MenuBar *>(this->menuBar());
  QList<QMenu *> menus = menuBar ? menuBar->menus() : QList<QMenu *>();
  if (index >= 0 && index < menus.size())
    QmlSupport::execMenu(menus.at(index),
                         QmlSupport::host(mView)->mapToGlobal(x, y));
}

void EditorWindow::updateWindowTitle() {
  QString name = mEditor->name();
  if (name.isEmpty())
    name = tr("Untitled");

  QString revision = mEditor->revision();
  setWindowTitle(revision.isEmpty() ? QString("%1[*]").arg(name)
                                    : QString("%1: %2[*]").arg(name, revision));
}

EditorWindow *EditorWindow::open(const QString &path, const git::Blob &blob,
                                 const git::Commit &commit,
                                 const git::Repository &repo) {
  QDir dir = repo.isValid() ? repo.workdir() : QDir::current();
  QFileInfo file(QDir::isAbsolutePath(path) ? path : dir.filePath(path));
  if (!file.exists() || !file.isFile())
    return nullptr;

  EditorWindow *window = new EditorWindow(repo);

  // Try to load the content.
  if (!window->editor()->load(path, blob, commit)) {
    delete window;
    return nullptr;
  }

  // Show the window.
  window->show();
  return window;
}

void EditorWindow::showEvent(QShowEvent *event) {
  updateWindowTitle();
  QMainWindow::showEvent(event);
}

void EditorWindow::closeEvent(QCloseEvent *event) {
  // Prompt to save.
  if (mEditor->isModified()) {
    QString text =
        tr("'%1' has been modified. Do you want to save your changes?");
    QString name = mEditor->name().isEmpty() ? tr("Untitled") : mEditor->name();
    ConfirmDialog dialog(this);
    dialog.setTitle(tr("Save Changes?"));
    dialog.setText(text.arg(name));
    dialog.setWarning(true);
    dialog.setAcceptText(tr("Save"));
    dialog.addButton(tr("Don't Save"));

    int result = dialog.exec();
    if (result == QDialog::Rejected) {
      event->ignore();
      return;
    }

    // The alternative button discards the changes. Keep the window if the
    // file wasn't saved.
    if (result == QDialog::Accepted && !mEditor->save()) {
      event->ignore();
      return;
    }
  }

  QMainWindow::closeEvent(event);
}
