//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "UpToDateDialog.h"
#include <QCoreApplication>

UpToDateDialog::UpToDateDialog(QWidget *parent) : ConfirmDialog(parent) {
  QString name = QCoreApplication::applicationName();
  QString version = QCoreApplication::applicationVersion();
  setTitle(tr("Already Up-to-date"));
  setText(tr("%1 is already up-to-date. You have version %2.")
              .arg(name, version));
  setCancelVisible(false);
}
