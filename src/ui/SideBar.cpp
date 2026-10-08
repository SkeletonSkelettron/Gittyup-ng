//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "SideBar.h"
#include "dialogs/ConfirmDialog.h"
#include "MainWindow.h"
#include "RepoView.h"
#include "TabWidget.h"
#include "conf/RecentRepositories.h"
#include "conf/RecentRepository.h"
#include "conf/Settings.h"
#include "dialogs/AccountDialog.h"
#include "dialogs/CloneDialog.h"
#include "host/Accounts.h"
#include "qml/QmlSupport.h"
#include <QAbstractItemModel>
#include <QFileDialog>
#include <QMenu>
#include <QPushButton>
#include <QQuickWidget>
#include <QSettings>
#include <QStyle>
#include <QTabBar>

namespace {

const QString kRemoteExpandedGroup = "remote/expanded";
const QString kSectionCollapsedGroup = "sidebar/collapsed";

enum Role {
  PathRole = Qt::UserRole,
  TabRole,
  RecentRole,
  AccountRole,
  AccountKindRole,
  RepositoryRole,

  // roles used by QML
  KindRole,
  IconRole,
  CurrentRole,
  CountRole,
  RemovableRole
};

// Keep in sync with SideBar.qml.
enum ItemKind {
  KindHeader,
  KindOpen,
  KindRecent,
  KindAccount,
  KindAddAccount,
  KindRemoteRepo,
  KindError,
  KindProgress,
  KindEmpty
};

// Does this index correspond to one of the open repositories?
bool isRepoIndex(const QModelIndex &index) {
  QModelIndex parent = index.parent();
  return (parent.isValid() && !parent.parent().isValid() && parent.row() == 0);
}

class RepoModel : public QAbstractItemModel {
  Q_OBJECT

public:
  enum RootRow { Repo, Recent, Remote };

  RepoModel(TabWidget *tabs, QObject *parent = nullptr)
      : QAbstractItemModel(parent), mTabs(tabs), mCloudIcon(":/cloud.png"),
        mErrorIcon(tabs->style()->standardIcon(QStyle::SP_MessageBoxCritical)) {
    // Tabs are also removed without a notice, like when the window is
    // destroyed.
    connect(tabs, &TabWidget::tabAboutToBeInserted, this,
            &RepoModel::beginTabsReset);
    connect(tabs, &TabWidget::tabAboutToBeRemoved, this,
            &RepoModel::beginTabsReset);
    connect(tabs, QOverload<>::of(&TabWidget::tabInserted), this,
            &RepoModel::endTabsReset);
    connect(tabs, QOverload<>::of(&TabWidget::tabRemoved), this,
            &RepoModel::endTabsReset);
    connect(tabs->tabBar(), &QTabBar::tabMoved, [this] {
      beginResetModel();
      endResetModel();
    });
    // Queued because adding a tab changes the current tab in the middle of
    // a model reset.
    connect(
        tabs, &TabWidget::currentChanged, this,
        [this] {
          QModelIndex parent = index(Repo, 0);
          int rows = rowCount(parent);
          if (rows > 0)
            emit dataChanged(index(0, 0, parent), index(rows - 1, 0, parent),
                             {CurrentRole});
        },
        Qt::QueuedConnection);

    RecentRepositories *repos = RecentRepositories::instance();
    connect(repos, &RecentRepositories::repositoryAboutToBeAdded, this,
            &RepoModel::beginResetModel);
    connect(repos, &RecentRepositories::repositoryAboutToBeRemoved, this,
            &RepoModel::beginResetModel);
    connect(repos, &RecentRepositories::repositoryAdded, this,
            &RepoModel::endResetModel);
    connect(repos, &RecentRepositories::repositoryRemoved, this,
            &RepoModel::endResetModel);

    Accounts *accounts = Accounts::instance();
    connect(accounts, &Accounts::accountAboutToBeAdded, this,
            &RepoModel::beginResetModel);
    connect(accounts, &Accounts::accountAdded, this, &RepoModel::endResetModel);
    connect(accounts, &Accounts::accountAboutToBeRemoved, this,
            &RepoModel::beginResetModel);
    connect(accounts, &Accounts::accountRemoved, this,
            &RepoModel::endResetModel);

    connect(accounts, &Accounts::progress, this, [this](int accountIndex) {
      QModelIndex idx = index(0, 0, index(accountIndex, 0, index(Remote, 0)));
      emit dataChanged(idx, idx, {Qt::DisplayRole});
    });
    connect(accounts, &Accounts::started, this, [this](int accountIndex) {
      beginResetModel();
      endResetModel();
    });
    connect(accounts, &Accounts::finished, this, [this](int accountIndex) {
      beginResetModel();
      endResetModel();
    });

    connect(accounts, &Accounts::repositoryAboutToBeAdded, this,
            &RepoModel::beginResetModel);
    connect(accounts, &Accounts::repositoryAdded, this,
            &RepoModel::endResetModel);
    connect(accounts, &Accounts::repositoryPathChanged, this,
            [this](int accountIndex, int repoIndex) {
              QModelIndex idx =
                  index(repoIndex, 0, index(accountIndex, 0, index(Remote, 0)));
              emit dataChanged(idx, idx, {Qt::DisplayRole});
            });
  }

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override {
    if (!parent.isValid())
      return createIndex(row, column);

    QObject *ptr = static_cast<QObject *>(parent.internalPointer());
    if (Account *account = qobject_cast<Account *>(ptr)) {
      // repository
      if (account->repositoryCount())
        return createIndex(row, column, account->repository(row));

      // error
      AccountError *error = account->error();
      if (error->isValid())
        return createIndex(row, column, error);

      // progress
      return createIndex(row, column, account->progress());
    }

    Accounts *accounts = Accounts::instance();
    RecentRepositories *recent = RecentRepositories::instance();
    switch (parent.row()) {
      case Repo:
        if (!mTabs->count())
          return createIndex(row, column, mTabs);
        return createIndex(row, column, mTabs->widget(row));

      case Recent:
        if (!recent->count())
          return createIndex(row, column, recent);
        return createIndex(row, column, recent->repository(row));

      case Remote:
        if (!accounts->count())
          return createIndex(row, column, accounts);
        return createIndex(row, column, accounts->account(row));
    }

    return QModelIndex();
  }

  QModelIndex parent(const QModelIndex &index) const override {
    QObject *ptr = static_cast<QObject *>(index.internalPointer());
    if (!ptr)
      return QModelIndex();

    if (qobject_cast<RepoView *>(ptr) || qobject_cast<TabWidget *>(ptr))
      return createIndex(Repo, 0);

    if (qobject_cast<RecentRepository *>(ptr) ||
        qobject_cast<RecentRepositories *>(ptr))
      return createIndex(Recent, 0);

    if (qobject_cast<Account *>(ptr) || qobject_cast<Accounts *>(ptr))
      return createIndex(Remote, 0);

    Account *account = nullptr;
    if (Repository *repo = qobject_cast<Repository *>(ptr)) {
      account = repo->account();
    } else if (AccountError *error = qobject_cast<AccountError *>(ptr)) {
      account = error->account();
    } else if (AccountProgress *ap = qobject_cast<AccountProgress *>(ptr)) {
      account = ap->account();
    }

    if (account)
      return createIndex(Accounts::instance()->indexOf(account), 0, account);

    return QModelIndex();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    if (!parent.isValid())
      return 3;

    QObject *ptr = static_cast<QObject *>(parent.internalPointer());
    if (Account *account = qobject_cast<Account *>(ptr)) {
      int count = account->repositoryCount();
      if (count > 0)
        return count;

      AccountError *error = account->error();
      AccountProgress *progress = account->progress();
      return (error->isValid() || progress->isValid()) ? 1 : 0;
    }

    if (parent.parent().isValid())
      return 0;

    switch (parent.row()) {
      case Repo:
        return mTabs->count() ? mTabs->count() : 1;

      case Recent: {
        RecentRepositories *recent = RecentRepositories::instance();
        return recent->count() ? recent->count() : 1;
      }

      case Remote: {
        Accounts *accounts = Accounts::instance();
        return accounts->count() ? accounts->count() : Account::NUM_KINDS;
      }

      default:
        return 0;
    }
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const override {
    return 1;
  }

  QHash<int, QByteArray> roleNames() const override {
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(KindRole, "kind");
    roles.insert(IconRole, "iconName");
    roles.insert(CurrentRole, "isCurrent");
    roles.insert(CountRole, "count");
    roles.insert(RemovableRole, "removable");
    return roles;
  }

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    switch (role) {
      case KindRole:
        return kind(index);
      case IconRole:
        return iconName(index);
      case CurrentRole:
        return isRepoIndex(index) && index.row() == mTabs->currentIndex();
      case CountRole:
        return count(index);
      case RemovableRole:
        return isRemovable(index);
      case Qt::ToolTipRole:
        // Always a string so QML can bind to it.
        return legacyData(index, role).toString();
      default:
        return legacyData(index, role);
    }
  }

  ItemKind kind(const QModelIndex &index) const {
    QModelIndex parent = index.parent();
    if (!parent.isValid())
      return KindHeader;

    QObject *ptr = static_cast<QObject *>(index.internalPointer());
    if (qobject_cast<Repository *>(ptr))
      return KindRemoteRepo;
    if (qobject_cast<AccountError *>(ptr))
      return KindError;
    if (qobject_cast<AccountProgress *>(ptr))
      return KindProgress;

    switch (parent.row()) {
      case Repo:
        return mTabs->count() ? KindOpen : KindEmpty;
      case Recent:
        return RecentRepositories::instance()->count() ? KindRecent : KindEmpty;
      case Remote:
        return Accounts::instance()->count() ? KindAccount : KindAddAccount;
      default:
        return KindEmpty;
    }
  }

  QString iconName(const QModelIndex &index) const {
    switch (kind(index)) {
      case KindOpen:
        return "repo";
      case KindRecent:
        return "clock";
      case KindAccount:
      case KindAddAccount:
        return QString("account-%1")
            .arg(static_cast<int>(
                index.data(AccountKindRole).value<Account::Kind>()));
      case KindRemoteRepo:
        return index.data(PathRole).toString().isEmpty() ? "cloud" : "repo";
      case KindError:
        return "alert";
      case KindProgress:
        return "spinner";
      default:
        return QString();
    }
  }

  int count(const QModelIndex &index) const {
    if (index.parent().isValid())
      return 0;

    switch (index.row()) {
      case Repo:
        return mTabs->count();
      case Recent:
        return RecentRepositories::instance()->count();
      case Remote:
        return Accounts::instance()->count();
      default:
        return 0;
    }
  }

  bool isRemovable(const QModelIndex &index) const {
    switch (kind(index)) {
      case KindOpen:
      case KindRecent:
      case KindAccount:
        return true;
      case KindRemoteRepo:
        return !index.data(PathRole).toString().isEmpty();
      default:
        return false;
    }
  }

  QVariant legacyData(const QModelIndex &index, int role) const {
    QObject *ptr = static_cast<QObject *>(index.internalPointer());
    if (Repository *repo = qobject_cast<Repository *>(ptr)) {
      switch (role) {
        case Qt::DisplayRole:
          return mShowFullName ? repo->fullName() : repo->name();

        case Qt::DecorationRole: {
          Account *account = repo->account();
          QString path = account->repositoryPath(account->indexOf(repo));
          return path.isEmpty() ? mCloudIcon : QIcon();
        }

        case PathRole: {
          Account *account = repo->account();
          return account->repositoryPath(account->indexOf(repo));
        }

        case RepositoryRole:
          return QVariant::fromValue(repo);

        default:
          return QVariant();
      }
    }

    if (AccountError *error = qobject_cast<AccountError *>(ptr)) {
      switch (role) {
        case Qt::DisplayRole:
          return error->text();
        case Qt::DecorationRole:
          return mErrorIcon;
        case Qt::ToolTipRole:
          return error->detailedText();
        default:
          return QVariant();
      }
    }

    if (AccountProgress *progress = qobject_cast<AccountProgress *>(ptr)) {
      switch (role) {
        case Qt::DisplayRole:
          return tr("Connecting");

        case Qt::DecorationRole:
          return progress->value();

        case Qt::FontRole: {
          QFont font = static_cast<QWidget *>(QObject::parent())->font();
          font.setItalic(true);
          return font;
        }

        default:
          return QVariant();
      }
    }

    int row = index.row();
    QModelIndex parent = index.parent();
    RecentRepositories *repos = RecentRepositories::instance();
    switch (role) {
      case Qt::DisplayRole: {
        if (!parent.isValid()) {
          switch (row) {
            case Repo:
              return tr("open");
            case Recent:
              return tr("recent");
            case Remote:
              return tr("remote");
            default:
              return QVariant();
          }
        }

        switch (parent.row()) {
          case Repo:
            if (mTabs->count()) {
              if (!mShowFullPath)
                return mTabs->tabText(row);

              RepoView *view = static_cast<RepoView *>(mTabs->widget(row));
              return view->repo().dir(false).path();
            }

            return tr("none");

          case Recent: {
            RecentRepositories *recent = RecentRepositories::instance();
            if (recent->count()) {
              RecentRepository *repo = repos->repository(row);
              return mShowFullPath ? repo->gitpath() : repo->name();
            }

            return tr("none");
          }

          case Remote: {
            Accounts *accounts = Accounts::instance();
            if (accounts->count())
              return accounts->account(row)->username();
            return Account::name(static_cast<Account::Kind>(row));
          }

          default:
            return QVariant();
        }
      }

      case Qt::DecorationRole: {
        if (!parent.isValid())
          return QVariant();

        switch (parent.row()) {
          case Remote: {
            Accounts *accounts = Accounts::instance();
            if (accounts->count())
              return Account::icon(accounts->account(row)->kind());
            return Account::icon(static_cast<Account::Kind>(row));
          }

          default:
            return QVariant();
        }
      }

      case Qt::FontRole: {
        if (!parent.isValid()) {
          QFont font;
          font.setCapitalization(QFont::SmallCaps);
          font.setBold(true);
          return font;
        }

        switch (parent.row()) {
          case Repo: {
            if (mTabs->count())
              return QVariant();

            QFont font;
            font.setItalic(true);
            return font;
          }

          case Recent: {
            RecentRepositories *recent = RecentRepositories::instance();
            if (recent->count())
              return QVariant();

            QFont font;
            font.setItalic(true);
            return font;
          }

          default:
            return QVariant();
        }
      }

      case Qt::ForegroundRole:
        return QPalette().brush(!parent.isValid() ? QPalette::BrightText
                                                  : QPalette::Text);

      case Qt::SizeHintRole:
        return QSize(0, QFontMetrics(QFont()).lineSpacing() + 8);

      case PathRole: {
        if (!parent.isValid())
          return QVariant();

        switch (parent.row()) {
          case Repo: {
            if (!mTabs->count())
              return QVariant();
            QWidget *widget = mTabs->widget(row);
            RepoView *view = static_cast<RepoView *>(widget);
            return view->repo().dir(false).path();
          }

          case Recent:
            if (!repos->count())
              return QVariant();
            return repos->repository(row)->gitpath();

          default:
            return QVariant();
        }
      }

      case TabRole:
        return QVariant::fromValue(qobject_cast<RepoView *>(ptr));

      case RecentRole:
        return QVariant::fromValue(qobject_cast<RecentRepository *>(ptr));

      case AccountRole:
        return QVariant::fromValue(qobject_cast<Account *>(ptr));

      case AccountKindRole: {
        if (!parent.isValid())
          return QVariant();

        switch (parent.row()) {
          case Remote:
            if (Account *account = qobject_cast<Account *>(ptr))
              return account->kind();
            return static_cast<Account::Kind>(row);

          default:
            return QVariant();
        }
      }

      case Qt::ToolTipRole: {
        if (!parent.isValid() ||
            mShowFullPath) // makes no sense to show tooltip when path is
                           // already shown completely
          return "";

        switch (parent.row()) {
          case Repo:
            if (mTabs->count()) {
              RepoView *view = static_cast<RepoView *>(mTabs->widget(row));
              return view->repo().dir(false).path();
            }

            return "";

          case Recent: {
            RecentRepositories *recent = RecentRepositories::instance();
            if (recent->count()) {
              RecentRepository *repo = repos->repository(row);
              return repo->gitpath();
            }

            return "";
          }

          default:
            return QVariant();
        }
      }
      default:
        return QVariant();
    }
  }

  Qt::ItemFlags flags(const QModelIndex &index) const override {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.parent().isValid())
      flags &= ~Qt::ItemIsSelectable;
    return flags;
  }

  QModelIndex currentIndex() const {
    return index(mTabs->currentIndex(), 0, index(Repo, 0));
  }

  void setShowFullPath(bool enabled) {
    beginResetModel();
    mShowFullPath = enabled;
    endResetModel();
  }

  void setShowFullName(bool enabled) {
    beginResetModel();
    mShowFullName = enabled;
    endResetModel();
  }

  void beginTabsReset() {
    if (mTabsResetting)
      return;

    mTabsResetting = true;
    beginResetModel();
  }

  void endTabsReset() {
    if (!mTabsResetting)
      beginResetModel();

    mTabsResetting = false;
    endResetModel();
  }

private:
  bool mTabsResetting = false;
  TabWidget *mTabs;
  bool mShowFullPath = false;
  bool mShowFullName = false;

  QIcon mCloudIcon;
  QIcon mErrorIcon;
};

bool isRemoteIndex(const QModelIndex &index) {
  QModelIndex parent = index.parent();
  return (parent.isValid() && !parent.parent().isValid() &&
          parent.row() == RepoModel::Remote);
}

bool autoHideSideBar() {
  return Settings::instance()
      ->value(Setting::Id::AutoHideRepoSiderbar, true)
      .toBool();
}

} // namespace

SideBar::SideBar(TabWidget *tabs, MainWindow *mainWindow)
    : QObject(mainWindow), mTabs(tabs), mMainWindow(mainWindow) {
  RepoModel *model = new RepoModel(tabs, this);
  mModel = model;

  // add menu
  mAddMenu = new QMenu(mMainWindow);

  QAction *clone = mAddMenu->addAction(tr("Clone Repository"));
  connect(clone, &QAction::triggered, [this] {
    CloneDialog *dialog = new CloneDialog(CloneDialog::Clone, mMainWindow);
    connect(dialog, &CloneDialog::accepted, [dialog] {
      if (MainWindow *window = MainWindow::open(dialog->path()))
        window->currentView()->addLogEntry(dialog->message(),
                                           dialog->messageTitle());
    });
    dialog->open();
  });

  QAction *openAction = mAddMenu->addAction(tr("Open Existing Repository"));
  connect(openAction, &QAction::triggered, [this] {
    // FIXME: Filter out non-git dirs.
    QFileDialog *dialog =
        new QFileDialog(mMainWindow, tr("Open Repository"), QDir::homePath());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly);
    connect(dialog, &QFileDialog::fileSelected,
            [](const QString &path) { MainWindow::open(path); });
    dialog->open();
  });

  QAction *init = mAddMenu->addAction(tr("Initialize New Repository"));
  connect(init, &QAction::triggered, [this] {
    CloneDialog *dialog = new CloneDialog(CloneDialog::Init, mMainWindow);
    connect(dialog, &CloneDialog::accepted, [dialog] {
      if (MainWindow *window = MainWindow::open(dialog->path()))
        window->currentView()->addLogEntry(dialog->message(),
                                           dialog->messageTitle());
    });
    dialog->open();
  });

  mAddMenu->addSeparator();

  for (int i = 0; i < Account::NUM_KINDS; ++i) {
    Account::Kind kind = static_cast<Account::Kind>(i);
    QString text = tr("Add %1 Account").arg(Account::name(kind));
    QAction *add = mAddMenu->addAction(text);
    connect(add, &QAction::triggered, [this, kind] {
      AccountDialog *dialog = new AccountDialog(nullptr, mMainWindow);
      dialog->setKind(kind);
      dialog->open();
    });
  }

  // options menu
  QSettings settings;
  mOptionsMenu = new QMenu(mMainWindow);

  QAction *clear = mOptionsMenu->addAction(tr("Clear All Recent"));
  connect(clear, &QAction::triggered,
          [] { RecentRepositories::instance()->clear(); });

  QAction *showFullPath = mOptionsMenu->addAction(tr("Show Full Path"));
  bool recentChecked = settings.value("start/recent/fullpath").toBool();
  showFullPath->setCheckable(true);
  showFullPath->setChecked(recentChecked);
  model->setShowFullPath(recentChecked);
  connect(showFullPath, &QAction::triggered, [model](bool checked) {
    QSettings().setValue("start/recent/fullpath", checked);
    model->setShowFullPath(checked);
  });

  QAction *filter = mOptionsMenu->addAction(tr("Filter Non-existent Paths"));
  filter->setCheckable(true);
  filter->setChecked(settings.value("recent/filter", true).toBool());
  connect(filter, &QAction::triggered,
          [](bool checked) { QSettings().setValue("recent/filter", checked); });

  mOptionsMenu->addSeparator();

  QAction *refresh = mOptionsMenu->addAction(tr("Refresh Remote Accounts"));
  connect(refresh, &QAction::triggered, [] {
    Accounts *accounts = Accounts::instance();
    for (int i = 0; i < accounts->count(); ++i)
      accounts->account(i)->connect();
  });

  QAction *showFullName = mOptionsMenu->addAction(tr("Show Full Name"));
  bool remoteChecked = settings.value("start/remote/fullname").toBool();
  showFullName->setCheckable(true);
  showFullName->setChecked(remoteChecked);
  model->setShowFullName(remoteChecked);
  connect(showFullName, &QAction::triggered, [model](bool checked) {
    QSettings().setValue("start/remote/fullname", checked);
    model->setShowFullName(checked);
  });

}

SideBar::~SideBar() {}

void SideBar::activate(const QModelIndex &index) {
  if (isRepoIndex(index))
    mTabs->setCurrentIndex(index.row());
}

void SideBar::open(const QModelIndex &index) {
  if (isRepoIndex(index)) {
    mTabs->setCurrentIndex(index.row());
    hideAfterOpen();
    return;
  }

  // Open existing path.
  QString path = index.data(PathRole).toString();
  if (!path.isEmpty()) {
    MainWindow::open(path);
    hideAfterOpen();
    return;
  }

  // Add remote account.
  QVariant accountKindVariant = index.data(AccountKindRole);
  if (accountKindVariant.isValid()) {
    Account *account = index.data(AccountRole).value<Account *>();
    AccountDialog *dialog = new AccountDialog(account, mMainWindow);
    dialog->setKind(accountKindVariant.value<Account::Kind>());
    dialog->open();
    return;
  }

  // Clone remote repository.
  QVariant repoVariant = index.data(RepositoryRole);
  if (repoVariant.isValid()) {
    Repository *repo = repoVariant.value<Repository *>();
    CloneDialog *dialog = new CloneDialog(CloneDialog::Clone, mMainWindow, repo);
    connect(dialog, &CloneDialog::accepted, [this, repo, dialog] {
      // Set local path.
      Account *account = repo->account();
      account->setRepositoryPath(account->indexOf(repo), dialog->path());

      // Open the repo.
      hideAfterOpen();
      MainWindow::open(dialog->path());
    });

    dialog->open();
  }
}

void SideBar::remove(const QModelIndex &index) {
  if (RepoView *tab = index.data(TabRole).value<RepoView *>()) {
    tab->close();

  } else if (index.data(RecentRole).value<RecentRepository *>()) {
    RecentRepositories::instance()->remove(index.row());

  } else if (auto account = index.data(AccountRole).value<Account *>()) {
    promptToRemoveAccount(account);

  } else if (auto repo = index.data(RepositoryRole).value<Repository *>()) {
    QString fmt = tr("<p>Are you sure you want to remove the remote repository "
                     "association for %1?</p><p>The local clone itself will "
                     "not be affected.</p>");

    ConfirmDialog *dialog = new ConfirmDialog(mMainWindow);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setTitle(tr("Remove Repository Association?"));
    dialog->setText(fmt.arg(repo->fullName()));
    dialog->setAcceptText(tr("Remove"));
    dialog->setDanger(true);

    int row = index.row();
    connect(dialog, &QDialog::accepted, [row, repo] {
      repo->account()->setRepositoryPath(row, QString());
    });

    dialog->open();
  }
}

void SideBar::showContextMenu(const QModelIndex &index, qreal x, qreal y) {
  QMenu *menu = new QMenu(mMainWindow);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  if (RepoView *view = index.data(TabRole).value<RepoView *>()) {
    menu->addAction(tr("Close"), view, &RepoView::close);
  } else if (index.data(RecentRole).value<RecentRepository *>()) {
    QPersistentModelIndex persistent(index);
    menu->addAction(tr("Open"), [this, persistent] {
      if (persistent.isValid())
        open(persistent);
    });
    menu->addAction(tr("Remove from Recent"), [this, persistent] {
      if (persistent.isValid())
        remove(persistent);
    });
  } else if (Account *account = index.data(AccountRole).value<Account *>()) {
    menu->addAction(tr("Remove"),
                    [this, account] { promptToRemoveAccount(account); });

    if (account->isAuthorizeSupported())
      menu->addAction(tr("Authorize"), account, &Account::authorize);
  }

  if (menu->isEmpty()) {
    delete menu;
    return;
  }

  QmlSupport::host(mView)->popup(menu, x, y);
}

void SideBar::showAddMenu(qreal x, qreal y) {
  QmlSupport::host(mView)->popup(mAddMenu, x, y);
}

void SideBar::showOptionsMenu(qreal x, qreal y) {
  QmlSupport::host(mView)->popup(mOptionsMenu, x, y);
}

bool SideBar::isExpanded(const QModelIndex &index) const {
  if (!index.parent().isValid()) {
    QSettings settings;
    settings.beginGroup(kSectionCollapsedGroup);
    return !settings.value(QString::number(index.row()), false).toBool();
  }

  if (isRemoteIndex(index)) {
    QSettings settings;
    settings.beginGroup(kRemoteExpandedGroup);
    return settings.value(index.data(Qt::DisplayRole).toString(), true)
        .toBool();
  }

  return true;
}

void SideBar::setExpanded(const QModelIndex &index, bool expanded) {
  QSettings settings;
  if (!index.parent().isValid()) {
    settings.beginGroup(kSectionCollapsedGroup);
    settings.setValue(QString::number(index.row()), !expanded);
  } else if (isRemoteIndex(index)) {
    settings.beginGroup(kRemoteExpandedGroup);
    settings.setValue(index.data(Qt::DisplayRole).toString(), expanded);
  }
}

void SideBar::hideAfterOpen() {
  if (autoHideSideBar())
    mMainWindow->setSideBarVisible(false);
}

void SideBar::promptToRemoveAccount(Account *account) {
  QString fmt =
      tr("<p>Are you sure you want to remove the %1 account for '%2'?</p>"
         "<p>Only the account association will be removed. Remote "
         "configurations and local clones will not be affected.</p>");

  ConfirmDialog *dialog = new ConfirmDialog(mMainWindow);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setTitle(tr("Remove Account?"));
  dialog->setText(fmt.arg(Account::name(account->kind()), account->username()));
  dialog->setAcceptText(tr("Remove"));
  dialog->setDanger(true);

  connect(dialog, &QDialog::accepted,
          [account] { Accounts::instance()->removeAccount(account); });

  dialog->open();
}

#include "SideBar.moc"
