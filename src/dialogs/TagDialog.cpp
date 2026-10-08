//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TagDialog.h"
#include "git/TagRef.h"
#include <algorithm>

TagDialog::TagDialog(const git::Repository &repo, const QString &id,
                     const git::Remote &remote, QWidget *parent)
    : QmlDialog(parent), mRepo(repo), mRemote(remote), mTarget(id) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Create Tag"));

  // Newer versions first, since the next tag is usually the greatest.
  mExistingTags = repo.existingTags();
  std::sort(mExistingTags.begin(), mExistingTags.end(), std::greater<>());

  setContent("TagDialog");
}

void TagDialog::setName(const QString &name) {
  if (name == mName)
    return;

  mName = name;
  emit changed();
}

QString TagDialog::nameError() const {
  if (mName.isEmpty() || mForce)
    return QString();

  if (mRepo.lookupTag(mName).isValid())
    return tr("A tag with this name already exists. Force it to replace the "
              "existing tag.");

  return QString();
}

void TagDialog::setForce(bool force) {
  if (force == mForce)
    return;

  mForce = force;
  emit changed();
}

void TagDialog::setAnnotated(bool annotated) {
  if (annotated == mAnnotated)
    return;

  mAnnotated = annotated;
  emit changed();
}

void TagDialog::setMessage(const QString &message) {
  if (message == mMessage)
    return;

  mMessage = message;
  emit changed();
}

void TagDialog::setPush(bool push) {
  if (push == mPush)
    return;

  mPush = push;
  emit changed();
}

QString TagDialog::pushText() const {
  return mRemote.isValid() ? tr("Push to %1").arg(mRemote.name()) : QString();
}

bool TagDialog::isAcceptable() const {
  return !mName.isEmpty() && nameError().isEmpty() &&
         (!mAnnotated || !mMessage.isEmpty());
}

QStringList TagDialog::existingTags() const {
  return mExistingTags.filter(mName, Qt::CaseSensitive);
}

git::Remote TagDialog::remote() const {
  return mPush ? mRemote : git::Remote();
}
