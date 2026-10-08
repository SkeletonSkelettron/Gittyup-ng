//
//          Copyright (c) 2022, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "AmendDialog.h"

namespace {

const QString kDateFormat = "yyyy-MM-dd HH:mm:ss";

} // namespace

AmendContributor::AmendContributor(const QString &title,
                                   const git::Signature &signature,
                                   QObject *parent)
    : QObject(parent), mTitle(title), mSignature(signature),
      mName(signature.name()), mEmail(signature.email()),
      mDateText(signature.date().toLocalTime().toString(kDateFormat)) {}

void AmendContributor::setName(const QString &name) {
  if (name == mName)
    return;

  mName = name;
  emit changed();
}

void AmendContributor::setEmail(const QString &email) {
  if (email == mEmail)
    return;

  mEmail = email;
  emit changed();
}

void AmendContributor::setDateType(ContributorInfo::SelectedDateTimeType type) {
  if (type == mDateType)
    return;

  mDateType = type;
  emit changed();
}

void AmendContributor::setDateTypeValue(int type) {
  setDateType(static_cast<ContributorInfo::SelectedDateTimeType>(type));
}

QDateTime AmendContributor::manualDate() const {
  return QDateTime::fromString(mDateText.trimmed(), kDateFormat);
}

void AmendContributor::setManualDate(const QDateTime &date) {
  setDateText(date.toString(kDateFormat));
}

void AmendContributor::setDateText(const QString &text) {
  if (text == mDateText)
    return;

  mDateText = text;
  emit changed();
}

bool AmendContributor::isDateValid() const {
  return mDateType != ContributorInfo::SelectedDateTimeType::Manual ||
         manualDate().isValid();
}

QString AmendContributor::originalDate() const {
  return QLocale().toString(mSignature.date().toLocalTime(),
                            QLocale::LongFormat);
}

ContributorInfo AmendContributor::info() const {
  ContributorInfo info;
  info.name = mName;
  info.email = mEmail;
  info.commitDateType = mDateType;
  info.commitDate = (mDateType == ContributorInfo::SelectedDateTimeType::Original)
                        ? mSignature.date().toLocalTime()
                        : manualDate();
  return info;
}

AmendDialog::AmendDialog(const git::Signature &author,
                         const git::Signature &committer,
                         const QString &commitMessage, QWidget *parent)
    : QmlDialog(parent),
      mAuthor(new AmendContributor(tr("Author"), author, this)),
      mCommitter(new AmendContributor(tr("Committer"), committer, this)),
      mMessage(commitMessage) {
  setWindowTitle(tr("Amend Commit"));

  connect(mAuthor, &AmendContributor::changed, this,
          &AmendDialog::acceptableChanged);
  connect(mCommitter, &AmendContributor::changed, this,
          &AmendDialog::acceptableChanged);

  setContent("AmendDialog");
}

AmendInfo AmendDialog::getInfo() const {
  AmendInfo info;
  info.authorInfo = mAuthor->info();
  info.committerInfo = mCommitter->info();
  info.commitMessage = mMessage;
  return info;
}

void AmendDialog::setCommitMessage(const QString &message) {
  if (message == mMessage)
    return;

  bool wasAcceptable = isAcceptable();
  mMessage = message;
  emit messageChanged();
  if (wasAcceptable != isAcceptable())
    emit acceptableChanged();
}

bool AmendDialog::isAcceptable() const {
  return !mMessage.trimmed().isEmpty() && !mAuthor->name().isEmpty() &&
         !mCommitter->name().isEmpty() && mAuthor->isDateValid() &&
         mCommitter->isDateValid();
}
