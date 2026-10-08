//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "NewBranchDialog.h"
#include "git/Branch.h"
#include "git/Reference.h"
#include "ui/RepoView.h"

NewBranchDialog::NewBranchDialog(const git::Repository &repo,
                                 const git::Commit &commit, QWidget *parent)
    : QmlDialog(parent), mRepo(repo), mCommit(commit),
      mCheckoutVisible(qobject_cast<RepoView *>(parent)),
      mStartPoints(repo, ReferenceItems::AllRefs),
      mUpstreams(repo, ReferenceItems::RemoteBranches, tr("None")) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("New Branch"));

  mStartPoint = mStartPoints.indexOf(repo.head());
  if (mStartPoint < 0 && mStartPoints.count())
    mStartPoint = 0;

  setContent("NewBranchDialog");
}

void NewBranchDialog::setName(const QString &name) {
  if (name == mName)
    return;

  mName = name;
  emit nameChanged();
}

QString NewBranchDialog::nameError() const {
  if (mName.isEmpty())
    return QString();

  if (!git::Branch::isNameValid(mName))
    return tr("This isn't a valid branch name.");

  if (mRepo.lookupBranch(mName, GIT_BRANCH_LOCAL).isValid())
    return tr("A branch with this name already exists.");

  return QString();
}

bool NewBranchDialog::isAcceptable() const {
  return !mName.isEmpty() && nameError().isEmpty();
}

void NewBranchDialog::setCheckout(bool checkout) {
  if (checkout == mCheckout)
    return;

  mCheckout = checkout;
  emit checkoutChanged();
}

QString NewBranchDialog::commitText() const {
  if (!mCommit.isValid())
    return QString();

  return QString("%1  %2").arg(mCommit.shortId(), mCommit.summary());
}

void NewBranchDialog::setStartPoint(int index) {
  if (index == mStartPoint)
    return;

  mStartPoint = index;
  emit startPointChanged();
}

void NewBranchDialog::setUpstreamIndex(int index) {
  if (index == mUpstream)
    return;

  mUpstream = index;
  emit upstreamChanged();

  // Populate the name and the start point from the upstream.
  git::Reference ref = mUpstreams.reference(index);
  if (!ref.isValid())
    return;

  if (mName.isEmpty())
    setName(ref.name().section('/', -1));

  int start = mStartPoints.indexOf(ref);
  if (start >= 0)
    setStartPoint(start);
}

git::Commit NewBranchDialog::target() const {
  if (mCommit.isValid())
    return mCommit;

  git::Reference ref = mStartPoints.reference(mStartPoint);
  return ref.isValid() ? ref.target() : git::Commit();
}

git::Reference NewBranchDialog::upstream() const {
  return mUpstreams.reference(mUpstream);
}
