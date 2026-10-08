//
//          Copyright (c) 2022, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "IgnoreDialog.h"

IgnoreDialog::IgnoreDialog(const QString &ignore, QWidget *parent)
    : QmlDialog(parent), mIgnore(ignore) {
  setWindowTitle(tr("Ignore"));
  setContent("IgnoreDialog");
}

void IgnoreDialog::setIgnoreText(const QString &text) {
  if (text == mIgnore)
    return;

  mIgnore = text;
  emit patternChanged();
}
