//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CheckoutDialog.h"
#include "git/Repository.h"

CheckoutDialog::CheckoutDialog(const git::Repository &repo,
                               const git::Reference &ref, QWidget *parent)
    : QmlDialog(parent), mRefs(repo, ReferenceItems::AllRefs) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Checkout"));

  mIndex = mRefs.indexOf(ref);
  if (mIndex < 0)
    mIndex = mRefs.indexOf(repo.head());
  if (mIndex < 0 && mRefs.count())
    mIndex = 0;

  setContent("CheckoutDialog");
}

git::Reference CheckoutDialog::reference() const {
  return mRefs.reference(mIndex);
}

void CheckoutDialog::setDetach(bool detach) {
  if (!isDetachEnabled() || detach == mDetach)
    return;

  mDetach = detach;
  emit changed();
}

void CheckoutDialog::setRefIndex(int index) {
  if (index == mIndex)
    return;

  mIndex = index;
  emit changed();
}

bool CheckoutDialog::isDetachEnabled() const {
  git::Reference ref = reference();
  return ref.isValid() && ref.isLocalBranch();
}

bool CheckoutDialog::isDetachChecked() const {
  return !isDetachEnabled() || mDetach;
}

bool CheckoutDialog::isAcceptable() const {
  git::Reference ref = reference();
  if (!ref.isValid())
    return false;

  return !ref.isHead() || (ref.isLocalBranch() && isDetachChecked());
}
