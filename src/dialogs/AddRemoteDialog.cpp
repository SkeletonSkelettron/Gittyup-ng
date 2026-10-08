//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "AddRemoteDialog.h"

AddRemoteDialog::AddRemoteDialog(const QString &name, QWidget *parent)
    : QmlDialog(parent), mName(name) {
  setWindowTitle(tr("Add Remote"));
  setContent("AddRemoteDialog");
}

void AddRemoteDialog::setName(const QString &name) {
  if (name == mName)
    return;

  mName = name;
  emit changed();
}

void AddRemoteDialog::setUrl(const QString &url) {
  if (url == mUrl)
    return;

  mUrl = url;
  emit changed();
}
