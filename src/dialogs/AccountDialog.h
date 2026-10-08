//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef ACCOUNTDIALOG_H
#define ACCOUNTDIALOG_H

#include "QmlDialog.h"
#include "host/Account.h"

// Add a hosting service account. qrc:/qml/AccountDialog.qml draws it.
class AccountDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QVariantList hosts READ hosts CONSTANT)
  Q_PROPERTY(int hostIndex READ hostIndex WRITE setHostIndex NOTIFY changed)
  Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY changed)
  Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY changed)
  Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY changed)
  Q_PROPERTY(QString helpText READ helpText NOTIFY changed)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY changed)
  Q_PROPERTY(bool busy READ isBusy NOTIFY changed)

public:
  AccountDialog(Account *account, QWidget *parent = nullptr);

  void accept() override;

  void setKind(Account::Kind kind);
  Account::Kind kind() const;

  QVariantList hosts() const;
  int hostIndex() const { return mHostIndex; }
  void setHostIndex(int index);

  QString username() const { return mUsername; }
  void setUsername(const QString &username);

  QString password() const { return mPassword; }
  void setPassword(const QString &password);

  QString url() const { return mUrl; }
  void setUrl(const QString &url);

  QString helpText() const;
  bool isAcceptable() const;
  bool isBusy() const { return mBusy; }

signals:
  void changed();

private:
  int mHostIndex = 0;
  QString mUsername;
  QString mPassword;
  QString mUrl;
  bool mBusy = false;
};

#endif
