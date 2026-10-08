//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "WelcomePage.h"
#include "MainWindow.h"
#include "RepoView.h"
#include "conf/RecentRepositories.h"
#include "conf/RecentRepository.h"
#include "dialogs/AccountDialog.h"
#include "dialogs/CloneDialog.h"
#include "host/Account.h"
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QUrl>

namespace {

const QString kSupportLink = "https://matrix.to/#/#Gittyup:matrix.org";

} // namespace

WelcomePage::WelcomePage(QWidget *widget) : QObject(widget), mWidget(widget) {
  RecentRepositories *repos = RecentRepositories::instance();
  connect(repos, &RecentRepositories::repositoryAdded, this,
          &WelcomePage::updateRecent);
  connect(repos, &RecentRepositories::repositoryRemoved, this,
          &WelcomePage::updateRecent);
  updateRecent();
}

QVariantList WelcomePage::accounts() const {
  QVariantList accounts;
  for (int i = 0; i < Account::NUM_KINDS; ++i) {
    Account::Kind kind = static_cast<Account::Kind>(i);
    accounts.append(QVariantMap{{"kind", i}, {"name", Account::name(kind)}});
  }

  return accounts;
}

void WelcomePage::setClosable(bool closable) {
  if (closable == mClosable)
    return;

  mClosable = closable;
  emit closableChanged();
}

void WelcomePage::openRepository() {
  // FIXME: Filter out non-git dirs.
  QFileDialog *dialog =
      new QFileDialog(mWidget, tr("Open Repository"), QDir::homePath());
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setFileMode(QFileDialog::Directory);
  dialog->setOption(QFileDialog::ShowDirsOnly);
  connect(dialog, &QFileDialog::fileSelected, this,
          [this](const QString &path) { open(path); });
  dialog->open();
}

void WelcomePage::cloneRepository() {
  CloneDialog *dialog = new CloneDialog(CloneDialog::Clone, mWidget);
  connect(dialog, &CloneDialog::accepted, this, [this, dialog] {
    open(dialog->path(), dialog->message(), dialog->messageTitle());
  });
  dialog->open();
}

void WelcomePage::initRepository() {
  CloneDialog *dialog = new CloneDialog(CloneDialog::Init, mWidget);
  connect(dialog, &CloneDialog::accepted, this, [this, dialog] {
    open(dialog->path(), dialog->message(), dialog->messageTitle());
  });
  dialog->open();
}

void WelcomePage::openRecent(const QString &path) { open(path); }

void WelcomePage::removeRecent(const QString &path) {
  RecentRepositories *repos = RecentRepositories::instance();
  for (int i = 0; i < repos->count(); ++i) {
    if (repos->repository(i)->gitpath() == path) {
      repos->remove(i);
      return;
    }
  }
}

void WelcomePage::addAccount(int kind) {
  AccountDialog *dialog = new AccountDialog(nullptr, mWidget);
  dialog->setKind(static_cast<Account::Kind>(kind));
  dialog->open();
}

void WelcomePage::openSupport() { QDesktopServices::openUrl(kSupportLink); }

void WelcomePage::close() { emit closeRequested(); }

void WelcomePage::updateRecent() {
  QVariantList recent;
  RecentRepositories *repos = RecentRepositories::instance();
  for (int i = 0; i < repos->count(); ++i) {
    RecentRepository *repo = repos->repository(i);
    QString path = repo->gitpath();
    recent.append(QVariantMap{{"name", repo->name()},
                              {"path", path},
                              {"display", QDir::toNativeSeparators(path)}});
  }

  mRecent = recent;
  emit recentChanged();
}

void WelcomePage::open(const QString &path, const QString &message,
                       const QString &title) {
  // Open the repository in a tab of this window.
  RepoView *view = nullptr;
  if (MainWindow *window = qobject_cast<MainWindow *>(mWidget->window())) {
    view = window->addTab(path);
  } else if (MainWindow *window = MainWindow::open(path)) {
    view = window->currentView();
  }

  if (view && !message.isEmpty())
    view->addLogEntry(message, title);
}
