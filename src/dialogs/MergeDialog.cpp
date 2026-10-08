//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "MergeDialog.h"
#include "conf/Settings.h"

namespace {

const int kRefKinds = ReferenceItems::LocalBranches |
                      ReferenceItems::RemoteBranches | ReferenceItems::Tags |
                      ReferenceItems::ExcludeHead;

// The flags of the actions, in the order of MergeDialog::actions().
const QList<int> kActions = {
    RepoView::Merge, RepoView::Rebase, RepoView::Squash,
    RepoView::Merge | RepoView::NoFastForward,
    RepoView::Merge | RepoView::FastForward};

} // namespace

MergeDialog::MergeDialog(RepoView::MergeFlags flags,
                         const git::Repository &repo, QWidget *parent)
    : QmlDialog(parent), mRepo(repo), mRefs(repo, kRefKinds) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Merge"));

  mAction = qMax(0, static_cast<int>(kActions.indexOf(static_cast<int>(flags))));
  mIndex = mRefs.count() ? 0 : -1;

  setContent("MergeDialog");
}

git::Commit MergeDialog::target() const {
  git::Reference ref = reference();
  return ref.isValid() ? ref.target() : mCommit;
}

git::Reference MergeDialog::reference() const {
  return mRefs.reference(mIndex);
}

RepoView::MergeFlags MergeDialog::actionFlags() const {
  return static_cast<RepoView::MergeFlags>(kActions.value(mAction, RepoView::Merge));
}

RepoView::MergeFlags MergeDialog::flags() const {
  RepoView::MergeFlags flags = actionFlags();
  if (noCommit())
    flags |= RepoView::NoCommit;
  return flags;
}

void MergeDialog::setCommit(const git::Commit &commit) {
  // Prefer a reference that points to the commit.
  int index = mRefs.indexOf(commit);
  if (index >= 0) {
    setRefIndex(index);
    return;
  }

  // Merge a commit that no reference points to.
  if (mCommit.isValid() || !commit.isValid())
    return;

  mCommit = commit;
  mRefs.prepend(QVariantMap{
      {"text", QString("%1  %2").arg(commit.shortId(), commit.summary())},
      {"icon", "commit"}});
  mIndex = 0;
  emit changed();
}

void MergeDialog::setReference(const git::Reference &ref) {
  int index = mRefs.indexOf(ref);
  if (index >= 0)
    setRefIndex(index);
}

void MergeDialog::setRefIndex(int index) {
  if (index == mIndex)
    return;

  mIndex = index;
  emit changed();
}

QStringList MergeDialog::actions() const {
  return {tr("Merge"), tr("Rebase"), tr("Squash"),
          tr("Merge (No Fast-forward)"), tr("Merge (Fast-forward Only)")};
}

void MergeDialog::setAction(int action) {
  if (action == mAction || action < 0 || action >= kActions.size())
    return;

  mAction = action;
  setWindowTitle(buttonText());
  emit changed();
}

bool MergeDialog::noCommit() const {
  return !Settings::instance()
              ->value(Setting::Id::CommitMergeImmediately)
              .toBool();
}

void MergeDialog::setNoCommit(bool noCommit) {
  Settings::instance()->setValue(Setting::Id::CommitMergeImmediately,
                                 !noCommit);
  emit changed();
}

bool MergeDialog::isNoCommitVisible() const {
  RepoView::MergeFlags flags = actionFlags();
  return (flags & RepoView::Merge) && !(flags & RepoView::FastForward);
}

QString MergeDialog::labelText() const {
  QString fmt;
  RepoView::MergeFlags flags = actionFlags();
  if (flags & RepoView::Merge)
    fmt = tr("Choose a reference to merge into '%1'.");
  else if (flags & RepoView::Rebase)
    fmt = tr("Choose a reference to rebase '%1' on.");
  else
    fmt = tr("Choose a reference to squash into '%1'.");

  git::Reference head = mRepo.head();
  return fmt.arg(head.isValid() ? head.name(false) : QString());
}

QString MergeDialog::buttonText() const {
  RepoView::MergeFlags flags = actionFlags();
  if (flags & RepoView::Merge)
    return tr("Merge");

  return (flags & RepoView::Rebase) ? tr("Rebase") : tr("Squash");
}
