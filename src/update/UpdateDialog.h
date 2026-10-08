//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include "dialogs/QmlDialog.h"

// Offer an update. qrc:/qml/UpdateDialog.qml draws it.
class UpdateDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString title READ title CONSTANT)
  Q_PROPERTY(QString text READ text CONSTANT)
  Q_PROPERTY(QString changelog READ changelog CONSTANT)
  Q_PROPERTY(bool installable READ isInstallable CONSTANT)
  Q_PROPERTY(bool installAutomatically READ installAutomatically WRITE
                 setInstallAutomatically NOTIFY installAutomaticallyChanged)

public:
  UpdateDialog(const QString &platform, const QString &version,
               const QString &changelog, const QString &link,
               QWidget *parent = nullptr);

  QString title() const;
  QString text() const { return mText; }
  QString changelog() const { return mChangelog; }

  // Whether Gittyup can install the update itself.
  bool isInstallable() const;

  bool installAutomatically() const;
  void setInstallAutomatically(bool install);

  Q_INVOKABLE void skip();
  Q_INVOKABLE void donate();

signals:
  void installAutomaticallyChanged();

private:
  void skipVersion();

  QString mVersion;
  QString mText;
  QString mChangelog;
};

#endif
