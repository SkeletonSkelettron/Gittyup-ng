//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "AccountDialog.h"
#include "ConfirmDialog.h"
#include "cred/CredentialHelper.h"
#include "host/Accounts.h"
#include <QUrl>

namespace {

// The hosts in the order of the list.
const QList<Account::Kind> kHosts = {Account::GitHub, Account::Gitea,
                                     Account::Bitbucket, Account::Beanstalk,
                                     Account::GitLab};

} // namespace

AccountDialog::AccountDialog(Account *account, QWidget *parent)
    : QmlDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Add Remote Account"));

  setKind(account ? account->kind() : Account::GitHub);
  if (account) {
    mUsername = account->username();
    mPassword = account->password();
    mUrl = account->url();
  }

  setContent("AccountDialog");
}

void AccountDialog::accept() {
  if (!isAcceptable())
    return;

  // Validate account.
  Account::Kind kind = this->kind();
  QString url = (mUrl != Account::defaultUrl(kind)) ? mUrl : QString();

  if (Account *account = Accounts::instance()->lookup(mUsername, kind)) {
    ConfirmDialog dialog(this);
    dialog.setTitle(tr("Replace?"));
    dialog.setText(tr("An account of this type already exists."));
    dialog.setInformativeText(
        tr("Would you like to replace the previous account?"));
    dialog.setAcceptText(tr("Replace"));
    if (dialog.exec() != QDialog::Accepted)
      return;

    Accounts::instance()->removeAccount(account);
  }

  Account *account = Accounts::instance()->createAccount(kind, mUsername, url);
  AccountProgress *progress = account->progress();
  connect(progress, &AccountProgress::finished, this, [this, account] {
    mBusy = false;
    emit changed();

    AccountError *error = account->error();
    if (error->isValid()) {
      ConfirmDialog::warning(this, tr("Connection Failed"), error->text(),
                             error->detailedText());
      Accounts::instance()->removeAccount(account);
      return;
    }

    // Store password.
    QUrl url;
    url.setScheme("https");
    url.setHost(account->host());

    CredentialHelper *helper = CredentialHelper::instance();
    helper->store(url.toString(), account->username(), mPassword);

    QDialog::accept();
  });

  // Start asynchronous connection.
  mBusy = true;
  emit changed();
  account->connect(mPassword);
}

void AccountDialog::setKind(Account::Kind kind) {
  setHostIndex(qMax(0, static_cast<int>(kHosts.indexOf(kind))));
}

Account::Kind AccountDialog::kind() const {
  return kHosts.value(mHostIndex, Account::GitHub);
}

QVariantList AccountDialog::hosts() const {
  QVariantList hosts;
  for (Account::Kind kind : kHosts)
    hosts.append(QVariantMap{{"text", Account::name(kind)},
                             {"icon", QString("account-%1").arg(kind)}});
  return hosts;
}

void AccountDialog::setHostIndex(int index) {
  if (index < 0 || index >= kHosts.size())
    return;

  bool defaultUrl = mUrl.isEmpty() || mUrl == Account::defaultUrl(kind());
  mHostIndex = index;

  // Follow the default URL of the host.
  if (defaultUrl)
    mUrl = Account::defaultUrl(kind());

  emit changed();
}

void AccountDialog::setUsername(const QString &username) {
  if (username == mUsername)
    return;

  mUsername = username;
  emit changed();
}

void AccountDialog::setPassword(const QString &password) {
  if (password == mPassword)
    return;

  mPassword = password;
  emit changed();
}

void AccountDialog::setUrl(const QString &url) {
  if (url == mUrl)
    return;

  mUrl = url;
  emit changed();
}

QString AccountDialog::helpText() const { return Account::helpText(kind()); }

bool AccountDialog::isAcceptable() const {
  return !mBusy && !mUsername.isEmpty() && !mPassword.isEmpty();
}
