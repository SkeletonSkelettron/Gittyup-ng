//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef ADDREMOTEDIALOG_H
#define ADDREMOTEDIALOG_H

#include "QmlDialog.h"

// Add a remote. qrc:/qml/AddRemoteDialog.qml draws it.
class AddRemoteDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
  Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY changed)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY changed)

public:
  AddRemoteDialog(const QString &name = QString(), QWidget *parent = nullptr);

  QString name() const { return mName; }
  void setName(const QString &name);

  QString url() const { return mUrl; }
  void setUrl(const QString &url);

  bool isAcceptable() const { return !mName.isEmpty() && !mUrl.isEmpty(); }

signals:
  void changed();

private:
  QString mName;
  QString mUrl;
};

#endif
