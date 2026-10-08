//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ToolBar.h"
#include "History.h"
#include "MainWindow.h"
#include "RepoView.h"
#include "SearchField.h"
#include "UndoHistory.h"
#include "dialogs/PullRequestDialog.h"
#include "dialogs/SettingsDialog.h"
#include "git/Branch.h"
#include "qml/QmlSupport.h"
#include "ui/HotkeyManager.h"
#include <QMenu>
#include <QQuickWidget>
#include <QRegularExpression>
#include <QShortcut>

namespace {

const QString kStarredQuery = "is:starred";

static Hotkey terminalHotkey = HotkeyManager::registerHotkey(
    nullptr, "tools/terminal", "Tools/Open Terminal");

static Hotkey fileManagerHotkey = HotkeyManager::registerHotkey(
    nullptr, "tools/fileManager", "Tools/Open File Manager");

} // namespace

ToolBar::ToolBar(MainWindow *parent) : QObject(parent), mWindow(parent) {
  Q_ASSERT(parent);

  mPullRequestAvailable = !qgetenv("GITTYUP_OAUTH").isEmpty();

  // Menus are native so they aren't clipped to the QML view.
  mPrevMenu = new QMenu(parent);
  connect(mPrevMenu, &QMenu::triggered, [this](QAction *action) {
    currentView()->history()->setIndex(action->data().toInt());
  });

  mNextMenu = new QMenu(parent);
  connect(mNextMenu, &QMenu::triggered, [this](QAction *action) {
    currentView()->history()->setIndex(action->data().toInt());
  });

  mPullMenu = new QMenu(parent);
  QAction *mergeAction = mPullMenu->addAction(tr("Merge"));
  connect(mergeAction, &QAction::triggered,
          [this] { currentView()->pull(RepoView::Merge); });

  QAction *rebaseAction = mPullMenu->addAction(tr("Rebase"));
  connect(rebaseAction, &QAction::triggered,
          [this] { currentView()->pull(RepoView::Rebase); });

  mSettingsMenu = new QMenu(parent);
  mRepoConfigAction = mSettingsMenu->addAction(tr("Repository settings"));
  connect(mRepoConfigAction, &QAction::triggered,
          [this] { currentView()->configureSettings(); });

  QAction *appConfigAction =
      mSettingsMenu->addAction(tr("Application settings"));
  connect(appConfigAction, &QAction::triggered,
          [] { SettingsDialog::openSharedInstance(); });

  mSearchField = new SearchField(this);

  connect(mSearchField, &SearchField::textChanged, [this](const QString &text) {
    QStringList terms = text.split(QRegularExpression("\\s+"));
    bool starred = terms.contains(kStarredQuery);
    if (starred != mState.starred) {
      mState.starred = starred;
      emit stateChanged();
    }
  });

  QShortcut *shortcut = new QShortcut(parent);
  terminalHotkey.use(shortcut);
  connect(shortcut, &QShortcut::activated, this, &ToolBar::openTerminal);

  shortcut = new QShortcut(parent);
  fileManagerHotkey.use(shortcut);
  connect(shortcut, &QShortcut::activated, this, &ToolBar::openFileManager);
}

ToolBar::~ToolBar() {}

void ToolBar::toggleSideBar() {
  MainWindow *window = static_cast<MainWindow *>(parent());
  window->setSideBarVisible(!window->isSideBarVisible());
}

void ToolBar::prev() {
  if (RepoView *view = currentView())
    view->history()->prev();
}

void ToolBar::next() {
  if (RepoView *view = currentView())
    view->history()->next();
}

void ToolBar::showHistoryMenu(bool next, qreal x, qreal y) {
  RepoView *view = currentView();
  if (!view)
    return;

  QMenu *menu = next ? mNextMenu : mPrevMenu;
  if (next) {
    view->history()->updateNextMenu(menu);
  } else {
    view->history()->updatePrevMenu(menu);
  }

  if (!menu->isEmpty())
    QmlSupport::host(mView)->popup(menu, x, y);
}

void ToolBar::undo() {
  if (RepoView *view = currentView())
    view->undoHistory()->undo();
}

void ToolBar::redo() {
  if (RepoView *view = currentView())
    view->undoHistory()->redo();
}

void ToolBar::fetch() {
  if (RepoView *view = currentView())
    view->fetch();
}

void ToolBar::pull() {
  if (RepoView *view = currentView())
    view->pull();
}

void ToolBar::showPullMenu(qreal x, qreal y) {
  if (mState.canPull)
    QmlSupport::host(mView)->popup(mPullMenu, x, y);
}

void ToolBar::push() {
  if (RepoView *view = currentView())
    view->push();
}

void ToolBar::checkout() {
  if (RepoView *view = currentView())
    view->promptToCheckout();
}

void ToolBar::stash() {
  if (RepoView *view = currentView())
    view->promptToStash();
}

void ToolBar::popStash() {
  if (RepoView *view = currentView())
    view->popStash();
}

void ToolBar::refresh() {
  if (RepoView *view = currentView())
    view->refresh();
}

void ToolBar::createPullRequest() {
  if (RepoView *view = currentView()) {
    PullRequestDialog *dialog = new PullRequestDialog(view);
    dialog->open();
  }
}

void ToolBar::openTerminal() {
  if (RepoView *view = currentView())
    view->openTerminal();
}

void ToolBar::openFileManager() {
  if (RepoView *view = currentView())
    view->openFileManager();
}

void ToolBar::toggleLog() {
  if (RepoView *view = currentView())
    view->setLogVisible(!view->isLogVisible());
}

void ToolBar::setViewMode(int mode) {
  if (RepoView *view = currentView())
    view->setViewMode(static_cast<RepoView::ViewMode>(mode));
}

void ToolBar::setStarred(bool starred) {
  QStringList terms = mSearchField->text().split(QRegularExpression("\\s+"),
                                                 Qt::SkipEmptyParts);
  if (starred) {
    if (!terms.contains(kStarredQuery))
      terms.append(kStarredQuery);
  } else {
    terms.removeAll(kStarredQuery);
  }

  mSearchField->setText(terms.join(' '));
}

void ToolBar::showSettingsMenu(qreal x, qreal y) {
  QmlSupport::host(mView)->popup(mSettingsMenu, x, y);
}

void ToolBar::updateButtons(int ahead, int behind) {
  RepoView *view = currentView();
  mState.hasView = view;
  mState.canCheckout = view && !view->repo().isBare();

  mState.repoName.clear();
  mState.repoPath.clear();
  mState.branchName.clear();
  if (view) {
    git::Repository repo = view->repo();
    QDir dir = repo.dir(false);
    mState.repoName = dir.dirName();
    mState.repoPath = dir.path();

    git::Reference head = repo.head();
    mState.branchName = head.isValid() ? head.name() : repo.unbornHeadName();
  }

  // Each of these emits stateChanged.
  updateRemote(ahead, behind);
  updateHistory();
  updateUndo();
  updateStash();
  updateView();
  updateSearch();
}

void ToolBar::updateRemote(int ahead, int behind) {
  RepoView *view = currentView();
  mState.ahead = ahead;
  mState.behind = behind;
  mState.canPull = view && !view->repo().isBare();
  emit stateChanged();
}

void ToolBar::updateHistory() {
  RepoView *view = currentView();
  History *history = view ? view->history() : nullptr;
  mState.canPrev = history && history->hasPrev();
  mState.canNext = history && history->hasNext();
  emit stateChanged();
}

void ToolBar::updateUndo() {
  RepoView *view = currentView();
  UndoHistory *history = view ? view->undoHistory() : nullptr;
  mState.canUndo = history && history->canUndo();
  mState.canRedo = history && history->canRedo();
  mState.undoText = history ? history->undoText() : QString();
  mState.redoText = history ? history->redoText() : QString();
  emit stateChanged();
}

void ToolBar::updateStash() {
  RepoView *view = currentView();
  mState.canStash = view && view->isWorkingDirectoryDirty();
  mState.canPop = view && view->repo().stashRef().isValid();
  emit stateChanged();
}

void ToolBar::updateView() {
  RepoView *view = currentView();
  MainWindow *window = static_cast<MainWindow *>(parent());
  mState.sidebarVisible = window->isSideBarVisible();
  mState.logVisible = view && view->isLogVisible();
  if (view)
    mState.viewMode = view->viewMode();
  mRepoConfigAction->setEnabled(view);
  emit stateChanged();
}

void ToolBar::updateSearch() { mSearchField->setEnabled(currentView()); }

RepoView *ToolBar::currentView() const {
  return mWindow->currentView();
}
