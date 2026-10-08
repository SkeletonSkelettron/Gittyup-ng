//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "MainWindow.h"
#include "CommandPalette.h"
#include "dialogs/ConfirmDialog.h"
#include "MenuBar.h"
#include "RepoView.h"
#include "SideBar.h"
#include "WelcomePage.h"
#include "SearchField.h"
#include "TabStrip.h"
#include "TabWidget.h"
#include "ToolBar.h"
#include "qml/QmlSupport.h"
#include "conf/RecentRepositories.h"
#include "conf/Settings.h"
#include "git/Repository.h"
#include "git/Config.h"
#include "git/Submodule.h"
#include "qmap.h"
#include <QApplication>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QCryptographicHash>
#include <QMenu>
#include <QMimeData>
#include <QSettings>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWidget>
#include "util/Debug.h"

namespace {

const int kDefaultWidth = 1200;
const int kDefaultHeight = 800;
const int kMinimumWidth = 720;
const int kMinimumHeight = 480;

const QString kPathKey = "path";
const QString kIndexKey = "index";
const QString kStateKey = "state";
const QString kActiveKey = "active";
const QString kSidebarKey = "sidebar";
const QString kGeometryKey = "geometry";
const QString kWindowsGroup = "windows";
const QString kLastGeometryKey = "lastGeometry";

class TabName {
public:
  TabName(const QString &path) : mPath(path) {}

  QString name() const { return mPath.section('/', -mSections); }

  void increment() { ++mSections; }
  int sections() const { return mSections; }
  void setSections(int sections) { mSections = sections; }

private:
  QString mPath;
  int mSections = 1;
};

} // namespace

bool MainWindow::sSaveWindowSettings = false;

MainWindow::MainWindow(const git::Repository &repo, QWidget *parent,
                       Qt::WindowFlags flags)
    : QMainWindow(parent, flags) {
  setAttribute(Qt::WA_DeleteOnClose);
  setUnifiedTitleAndToolBarOnMac(true);
  setAcceptDrops(true);

  // Create new menu bar for this window if there isn't a shared one.
  mMenuBar = MenuBar::instance(this);
  mMenuBar->registerActions(this);

  // Update title and refresh when settings change.
  mFullPath =
      Settings::instance()->value(Setting::Id::ShowFullRepoPath).toBool();
  connect(Settings::instance(), &Settings::settingsChanged, this,
          [this](bool refresh) {
            Settings *settings = Settings::instance();

            // The view draws the menu bar unless it's native.
            bool menuBarHidden =
                settings->value(Setting::Id::HideMenuBar).toBool();
            if (mMenuBar->isNativeMenuBar()) {
              if (mMenuBar->isHidden() != menuBarHidden)
                mMenuBar->setHidden(menuBarHidden);
            } else {
              emit menuBarVisibleChanged();
            }

            bool fullPath =
                settings->value(Setting::Id::ShowFullRepoPath).toBool();
            if (mFullPath != fullPath) {
              mFullPath = fullPath;
              updateWindowTitle();
            }

            if (refresh) {
              for (int i = 0; i < count(); ++i)
                view(i)->refresh();
            }
          });

  // The tabs keep the repositories. The view draws them.
  mTabs = new TabWidget(this);
  mTabs->hide();
  connect(mTabs, &TabWidget::currentChanged, [this](int index) {
    updatePages();
    updateInterface();
    MenuBar::instance(this)->update();
  });
  connect(mTabs, &TabWidget::welcomeChanged, this, &MainWindow::updatePages);

  connect(mTabs, QOverload<>::of(&TabWidget::tabInserted), this,
          &MainWindow::updateTabNames);
  connect(mTabs, QOverload<>::of(&TabWidget::tabRemoved), this,
          &MainWindow::updateTabNames);

  mTabStrip = new TabStrip(this);
  mTabStrip->setTabWidget(mTabs);
  mToolBar = new ToolBar(this);
  mSideBar = new SideBar(mTabs, this);
  mPalette = new CommandPalette(this);

  // The actions of the menu bar are also added to the window, so their
  // shortcuts work while the view draws the menu bar.
  if (!mMenuBar->isNativeMenuBar())
    mMenuBar->hide();

  // Draw everything in one view, with the sidebar as it was.
  mIsSideBarVisible = QSettings().value(kSidebarKey, true).toBool();
  mView = QmlSupport::createView(
      "MainPage",
      {{"mainWindow", QVariant::fromValue<QObject *>(this)},
       {"tabStrip", QVariant::fromValue<QObject *>(mTabStrip)},
       {"toolbar", QVariant::fromValue<QObject *>(mToolBar)},
       {"search", QVariant::fromValue<QObject *>(mToolBar->searchField())},
       {"sidebar", QVariant::fromValue<QObject *>(mSideBar)},
       {"welcome", QVariant::fromValue<QObject *>(mTabs->welcomePage())},
       {"commandPalette", QVariant::fromValue<QObject *>(mPalette)}},
      this);
  mView->setMinimumSize(kMinimumWidth, kMinimumHeight);
  QmlSupport::setDrawsPopups(mView, true);
  setCentralWidget(mView);
  mTabStrip->setView(mView);
  mToolBar->setView(mView);
  mSideBar->setView(mView);

  if (repo)
    addTab(repo);

  // Restore the last known size and position, falling back to a default.
  QByteArray lastGeometry = QSettings().value(kLastGeometryKey).toByteArray();
  if (!lastGeometry.isEmpty()) {
    restoreGeometry(lastGeometry);
  } else {
    resize(kDefaultWidth, kDefaultHeight);

    QRect desktop = QGuiApplication::primaryScreen()->availableGeometry();
    int x = (desktop.width() / 2) - (kDefaultWidth / 2);
    int y = (desktop.height() / 2) - (kDefaultHeight / 2);
    move(x, y);

    // Position with respect to existing windows.
    if (MainWindow *win = activeWindow())
      move(win->x() + 24, win->y() + 24);
  }

  // Set initial state of interface.
  updateInterface();
}

MainWindow::~MainWindow() {
  // The pages of the repositories are in the view, which references the
  // objects of the window, so they go first and the view next.
  delete mTabs;
  mTabs = nullptr;
  delete mView;
  mView = nullptr;
}

bool MainWindow::isSideBarVisible() const { return mIsSideBarVisible; }

void MainWindow::setSideBarVisible(bool visible) {
  if (visible == mIsSideBarVisible)
    return;

  // The sidebar slides in or out.
  mIsSideBarVisible = visible;
  emit sideBarVisibleChanged();
  mToolBar->updateView();

  // Remember in settings.
  QSettings().setValue(kSidebarKey, visible);
}

TabWidget *MainWindow::tabWidget() const { return mTabs; }

bool MainWindow::isWelcomeVisible() const {
  return mTabs && mTabs->isWelcomeVisible();
}

bool MainWindow::isMenuBarVisible() const {
  return !mMenuBar->isNativeMenuBar() &&
         !Settings::instance()->value(Setting::Id::HideMenuBar).toBool();
}

QStringList MainWindow::menuTitles() const {
  QStringList titles;
  for (QMenu *menu : mMenuBar->menus())
    titles.append(menu->title());
  return titles;
}

void MainWindow::showMenu(int index, qreal x, qreal y) {
  QList<QMenu *> menus = mMenuBar->menus();
  if (index >= 0 && index < menus.size())
    QmlSupport::execMenu(menus.at(index), mapFromScene(x, y));
}

QPoint MainWindow::mapFromScene(qreal x, qreal y) const {
  return QmlSupport::host(mView)->mapToGlobal(x, y);
}

QQuickItem *MainWindow::createPage(const QString &name,
                                   const QVariantMap &objects, QObject *owner,
                                   QQmlContext **context) {
  QQuickItem *pages = mView->rootObject()->findChild<QQuickItem *>("pages");
  Q_ASSERT(pages);

  QQmlComponent component(mView->engine(),
                          QUrl(QString("qrc:/qml/%1.qml").arg(name)));
  QQmlContext *pageContext = new QQmlContext(mView->rootContext(), owner);
  for (auto it = objects.cbegin(); it != objects.cend(); ++it)
    pageContext->setContextProperty(it.key(), it.value());

  QQuickItem *page = qobject_cast<QQuickItem *>(component.beginCreate(pageContext));
  if (!page) {
    for (const QQmlError &error : component.errors())
      qWarning("%s", qPrintable(error.toString()));
    delete pageContext;
    *context = nullptr;
    return nullptr;
  }

  page->setParent(owner);
  page->setParentItem(pages);
  page->setVisible(false);
  QQmlProperty(page, "anchors.fill").write(QVariant::fromValue(pages));
  component.completeCreate();

  *context = pageContext;
  return page;
}

void MainWindow::updatePages() {
  // Only the page of the current tab is shown, unless the welcome page is.
  int current = mTabs->currentIndex();
  bool welcome = mTabs->isWelcomeVisible();
  for (int i = 0; i < count(); ++i)
    view(i)->setPageVisible(!welcome && i == current);

  emit welcomeVisibleChanged();
}

RepoView *MainWindow::addTab(const QString &path) {
  if (path.isEmpty())
    return nullptr;

  TabWidget *tabs = tabWidget();
  for (int i = 0; i < tabs->count(); i++) {
    RepoView *view = static_cast<RepoView *>(tabs->widget(i));
    if (path == view->repo().dir(false).path()) {
      tabs->setCurrentIndex(i);
      return view;
    }
  }

  git::Repository repo = git::Repository::open(path, true);
  if (!repo.isValid()) {
    warnInvalidRepo(path);
    return nullptr;
  }

  return addTab(repo);
}

RepoView *MainWindow::addTab(const git::Repository &repo) {
  // Update recent repository settings.
  QDir dir = repo.dir(false);
  RecentRepositories::instance()->add(dir.path());

  TabWidget *tabs = tabWidget();
  for (int i = 0; i < tabs->count(); i++) {
    RepoView *view = static_cast<RepoView *>(tabs->widget(i));
    if (dir.path() == view->repo().dir(false).path()) {
      tabs->setCurrentIndex(i);
      return view;
    }
  }

  RepoView *view = new RepoView(repo, this);
  view->detailSplitterMaximize(mMenuBar->isMaximized());
  git::RepositoryNotifier *notifier = repo.notifier();
  connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
          &MainWindow::updateInterface);
  connect(notifier, &git::RepositoryNotifier::stateChanged, this,
          [this] { updateWindowTitle(); });

  emit tabs->tabAboutToBeInserted();
  tabs->setCurrentIndex(tabs->addTab(view, dir.dirName()));

  Settings *settings = Settings::instance();
  bool enable =
      settings->value(Setting::Id::UpdateSubmodulesAfterPullAndClone).toBool();
  if (repo.appConfig().value<bool>("autoupdate.enable", enable)) {
    // update submodules
    view->updateSubmodules(repo.submodules(), true, true, false, nullptr);
  }

  // Start status diff.
  view->refresh(false);
  return view;
}

// The tabs are gone while the window is destroyed.
int MainWindow::count() const { return mTabs ? mTabs->count() : 0; }

RepoView *MainWindow::currentView() const {
  return mTabs ? static_cast<RepoView *>(mTabs->currentWidget()) : nullptr;
}

RepoView *MainWindow::view(int index) const {
  return mTabs ? static_cast<RepoView *>(mTabs->widget(index)) : nullptr;
}

MainWindow *MainWindow::activeWindow() {
  QWidget *win = QApplication::activeWindow();
  if (MainWindow *mainWin = qobject_cast<MainWindow *>(win))
    return mainWin;

  QList<MainWindow *> mainWins = windows();
  return !mainWins.isEmpty() ? mainWins.first() : nullptr;
}

QList<MainWindow *> MainWindow::windows() {
  QList<MainWindow *> mainWins;
  for (QWidget *win : QApplication::topLevelWidgets()) {
    if (MainWindow *mainWin = qobject_cast<MainWindow *>(win))
      mainWins.append(mainWin);
  }

  return mainWins;
}

bool MainWindow::restoreWindows() {
  QList<MainWindow *> windows;

  // Open windows.
  QSettings settings;
  settings.beginGroup(kWindowsGroup);
  for (const QString &group : settings.childGroups()) {
    settings.beginGroup(group);
    int index = settings.value(kIndexKey).toInt();
    bool active = settings.value(kActiveKey).toBool();
    QStringList paths = settings.value(kPathKey).toStringList();
    QByteArray state = settings.value(kStateKey).toByteArray();
    QByteArray geometry = settings.value(kGeometryKey).toByteArray();
    settings.endGroup();

    // This shouldn't ever happen.
    if (paths.isEmpty())
      continue;

    // Open a window new for the first valid repo.
    MainWindow *window = open(paths.takeFirst());
    while (!window && !paths.isEmpty())
      window = open(paths.takeFirst());

    if (!window)
      continue;

    // Add the remainder as tabs.
    for (const QString &path : paths)
      window->addTab(path);

    // Select saved index.
    window->tabWidget()->setCurrentIndex(index);

    // Restore state and geometry.
    window->restoreState(state);
    window->restoreGeometry(geometry);

    // Order active window first.
    windows.insert(active ? 0 : windows.size(), window);
  }
  settings.endGroup();

  // Remove all window settings.
  settings.remove(kWindowsGroup);

  // Activate the top window.
  if (!windows.isEmpty()) {
    MainWindow *window = windows.first();
    window->raise();
    window->activateWindow();
  }

  return !windows.isEmpty();
}

MainWindow *MainWindow::open(const QString &path, bool warnOnInvalid) {
  DebugRefresh("Open project: " << path);
  if (path.isEmpty())
    return nullptr;

  git::Repository repo = git::Repository::open(path, true);
  if (!repo.isValid()) {
    if (warnOnInvalid)
      warnInvalidRepo(path);
    return nullptr;
  }

  if (Settings::instance()->value(Setting::Id::OpenAllReposInTabs).toBool()) {
    if (MainWindow *win = activeWindow()) {
      win->addTab(repo);
      return win;
    }
  }

  return open(repo);
}

MainWindow *MainWindow::open(const git::Repository &repo) {
  // Update recent repository settings.
  if (repo.isValid())
    RecentRepositories::instance()->add(repo.dir(false).path());

  // Create the window.
  MainWindow *window = new MainWindow(repo);

  const bool showMaximized =
      Settings::instance()->value(Setting::Id::ShowMaximized).toBool();

  if (showMaximized) {
    window->showMaximized();
  } else {
    window->show();
  }

  return window;
}

void MainWindow::setSaveWindowSettings(bool enabled) {
  sSaveWindowSettings = enabled;
}

void MainWindow::showEvent(QShowEvent *event) {
  QMainWindow::showEvent(event);

  if (mShown)
    return;

  mShown = true;
  updateInterface();
}

void MainWindow::closeEvent(QCloseEvent *event) {
  // FIXME: Attempt to close windows before writing settings?

  // Remember size and position for the next new window, independent of
  // full session restore.
  QSettings().setValue(kLastGeometryKey, saveGeometry());

  if (sSaveWindowSettings) {
    // Store window state.
    // FIXME: Qt doesn't impose a predictable order on top-level windows.
    // Instead order the active window first and leave others undefined.
    QSettings settings;
    settings.beginGroup(kWindowsGroup);
    settings.beginGroup(windowGroup());
    settings.setValue(kPathKey, paths());
    settings.setValue(kIndexKey, tabWidget()->currentIndex());
    settings.setValue(kActiveKey, this == activeWindow());
    settings.setValue(kStateKey, saveState());
    settings.setValue(kGeometryKey, saveGeometry());
    settings.endGroup();
    settings.endGroup();
  }

  for (int i = 0; i < count(); ++i) {
    if (!view(i)->close()) {
      event->ignore();
      return;
    }
  }

  mClosing = true;
  QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  if (!event->mimeData()->hasFormat("text/uri-list"))
    return;

  for (const QUrl &url : event->mimeData()->urls()) {
    if (!url.isLocalFile())
      return;

    QDir dir(url.toLocalFile());
    if (!dir.exists())
      return;

    if (!git::Repository::open(dir.path(), true).isValid())
      return;
  }

  event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
  for (const QUrl &url : event->mimeData()->urls())
    addTab(url.toLocalFile());
}

void MainWindow::warnInvalidRepo(const QString &path) {
  // Say why, like that the repository belongs to another user.
  QString reason;
  if (git::Repository::lastErrorKind() != 0)
    reason = git::Repository::lastError();

  QString title = tr("Invalid Git Repository");
  QString text = tr("%1 does not contain a valid git repository.").arg(path);
  if (!reason.isEmpty())
    text += "\n\n" + reason;
  if (reason.contains("not owned by current user"))
    text += "\n\n" + tr("Start %1 as the user that owns the repository, "
                        "or trust it with 'git config --global --add "
                        "safe.directory <path>'.")
                         .arg(QCoreApplication::applicationName());
  ConfirmDialog::warning(activeWindow(), title, text);
}

void MainWindow::updateTabNames() {
  TabWidget *tabs = tabWidget();
  QHash<QString, QList<int>> names;
  QList<TabName> fullNames;

  for (int i = 0; i < count(); ++i) {
    TabName name(view(i)->repo().dir(false).path());
    names[name.name()].append(i);
    fullNames.append(name);
  }

  QHash<QString, QList<int>>::key_iterator first;
  while ((first = names.keyBegin()) != names.keyEnd()) {
    auto key = *first;
    auto ids = names.take(key);

    if (ids.count() == 1) {
      tabs->setTabText(ids.first(), key);
    } else {
      for (auto id : ids) {
        auto &name = fullNames[id];
        name.increment();
        names[name.name()].append(id);
      }
    }
  }
}

void MainWindow::updateInterface() {
  // Avoid updating during close.
  if (mClosing)
    return;

  int ahead = 0;
  int behind = 0;
  if (RepoView *view = currentView()) {
    if (git::Branch head = view->repo().head()) {
      if (git::Branch upstream = head.upstream()) {
        ahead = head.difference(upstream);
        behind = upstream.difference(head);
      }
    }
  }

  updateWindowTitle(ahead, behind);
  mToolBar->updateButtons(ahead, behind);
}

void MainWindow::updateWindowTitle(int ahead, int behind) {
  RepoView *view = currentView();
  if (!view) {
    setWindowTitle(QCoreApplication::applicationName() + BUILD_DESCRIPTION);
    return;
  }

  git::Repository repo = view->repo();
  QDir dir = repo.dir(false);
  git::Reference head = repo.head();
  QString path = mFullPath ? dir.path() : dir.dirName();
  QString name = head.isValid() ? head.name() : repo.unbornHeadName();
  QString title = tr("%1 - %2").arg(path, name);

  // Add remote tracking information.
  if (git::Branch branch = head) {
    if (git::Branch upstream = branch.upstream()) {
      if (ahead < 0)
        ahead = branch.difference(upstream);
      if (behind < 0)
        behind = upstream.difference(branch);

      QStringList parts;
      if (ahead > 0)
        parts.append(tr("ahead: %1").arg(ahead));
      if (behind > 0)
        parts.append(tr("behind: %1").arg(behind));

      QString status = parts.isEmpty() ? tr("up-to-date") : parts.join(", ");
      QString remote = tr("%1 (%2)").arg(status, upstream.name());
      title = tr("%1 - %2").arg(title, remote);
    }
  }

  // Add state.
  QString state;
  switch (repo.state()) {
    case GIT_REPOSITORY_STATE_MERGE:
      state = tr("MERGING");
      break;

    case GIT_REPOSITORY_STATE_REVERT:
    case GIT_REPOSITORY_STATE_REVERT_SEQUENCE:
      state = tr("REVERTING");
      break;

    case GIT_REPOSITORY_STATE_CHERRYPICK:
    case GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE:
      state = tr("CHERRY-PICKING");
      break;

    case GIT_REPOSITORY_STATE_BISECT:
      break; // FIXME?

    case GIT_REPOSITORY_STATE_REBASE:
    case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
    case GIT_REPOSITORY_STATE_REBASE_MERGE:
      state = tr("REBASING");
      break;

    case GIT_REPOSITORY_STATE_APPLY_MAILBOX:
    case GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE:
      break; // FIXME?
  }

  if (!state.isEmpty())
    title = tr("%1 (%2)").arg(title, state);

  setWindowTitle(QString("%1%2").arg(title, BUILD_DESCRIPTION));
}

QStringList MainWindow::paths() const {
  QStringList paths;
  for (int i = 0; i < count(); ++i)
    paths.append(view(i)->repo().dir(false).path());
  return paths;
}

QString MainWindow::windowGroup() const {
  QByteArray group = paths().join(';').toUtf8();
  QByteArray hash = QCryptographicHash::hash(group, QCryptographicHash::Md5);
  return QString::fromUtf8(hash.toHex());
}
