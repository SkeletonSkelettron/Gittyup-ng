//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "ConfirmDialog.h"

ConfirmDialog::ConfirmDialog(QWidget *parent)
    : QmlDialog(parent), mAcceptText(tr("OK")) {
  setContent("ConfirmDialog");
}

void ConfirmDialog::setTitle(const QString &title) {
  mTitle = title;
  setWindowTitle(title);
  emit changed();
}

void ConfirmDialog::setText(const QString &text) {
  mText = text;
  emit changed();
}

void ConfirmDialog::setInformativeText(const QString &text) {
  mInformativeText = text;
  emit changed();
}

void ConfirmDialog::setDetailedText(const QString &text) {
  mDetailedText = text;
  emit changed();
}

void ConfirmDialog::setAcceptText(const QString &text) {
  mAcceptText = text;
  emit changed();
}

void ConfirmDialog::setCheckText(const QString &text) {
  mCheckText = text;
  emit changed();
}

void ConfirmDialog::setChecked(bool checked) {
  if (checked == mChecked)
    return;

  mChecked = checked;
  emit changed();
}

void ConfirmDialog::setDanger(bool danger) {
  mDanger = danger;
  emit changed();
}

void ConfirmDialog::setWarning(bool warning) {
  mWarning = warning;
  emit changed();
}

void ConfirmDialog::setCancelVisible(bool visible) {
  mCancelVisible = visible;
  emit changed();
}

int ConfirmDialog::addButton(const QString &text) {
  mButtons.append(text);
  emit changed();
  return mButtons.size() - 1;
}

void ConfirmDialog::clickButton(int index) {
  if (index < 0 || index >= mButtons.size())
    return;

  mClicked = index;
  emit buttonClicked(index);
  done(QDialog::Accepted + 1 + index);
}

ConfirmDialog *ConfirmDialog::information(QWidget *parent,
                                          const QString &title,
                                          const QString &text,
                                          const QString &detailedText) {
  ConfirmDialog *dialog = new ConfirmDialog(parent);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setTitle(title);
  dialog->setText(text);
  dialog->setDetailedText(detailedText);
  dialog->setCancelVisible(false);
  return dialog;
}

void ConfirmDialog::warning(QWidget *parent, const QString &title,
                            const QString &text,
                            const QString &informativeText) {
  ConfirmDialog dialog(parent);
  dialog.setTitle(title);
  dialog.setText(text);
  dialog.setInformativeText(informativeText);
  dialog.setWarning(true);
  dialog.setCancelVisible(false);
  dialog.exec();
}
