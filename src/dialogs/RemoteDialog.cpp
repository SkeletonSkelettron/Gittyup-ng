//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RemoteDialog.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include "git/Remote.h"
#include "ui/RepoView.h"
#include "ui/qml/QmlSupport.h"
#include <QMenu>

namespace {

// The flags of the pull actions, in the order of RemoteDialog::actions().
const QList<int> kPullActions = {
    RepoView::Merge, RepoView::Rebase,
    RepoView::Merge | RepoView::NoFastForward,
    RepoView::Merge | RepoView::FastForward};

} // namespace

RemoteDialog::RemoteDialog(Kind kind, RepoView *parent)
    : QmlDialog(parent), mKind(kind), mRepo(parent->repo()),
      mRefs(mRepo, kind == Push
                       ? ReferenceItems::LocalBranches | ReferenceItems::Tags
                       : 0) {
  setAttribute(Qt::WA_DeleteOnClose);

  switch (kind) {
    case Fetch:
      setWindowTitle(tr("Fetch"));
      break;
    case Pull:
      setWindowTitle(tr("Pull"));
      break;
    case Push:
      setWindowTitle(tr("Push"));
      break;
  }

  git::Remote remote = mRepo.defaultRemote();
  if (!remote.isValid() && !mRepo.remotes().isEmpty())
    remote = mRepo.remotes().first();
  mRemote = remote.isValid() ? remote.name() : QString();

  if (kind == Push) {
    setRefIndex(mRefs.indexOf(mRepo.head()));
  } else {
    bool autoPrune =
        Settings::instance()->value(Setting::Id::PruneAfterFetch).toBool();
    mPrune = mRepo.appConfig().value<bool>("autoprune.enable", autoPrune);
  }

  connect(this, &QDialog::accepted, this, &RemoteDialog::run);

  setContent("RemoteDialog");
}

void RemoteDialog::setRemote(const QString &remote) {
  if (remote == mRemote)
    return;

  mRemote = remote;
  emit changed();
}

void RemoteDialog::showRemoteMenu(qreal x, qreal y) {
  QMenu menu;
  for (const git::Remote &remote : mRepo.remotes()) {
    QString name = remote.name();
    QAction *action = menu.addAction(QString("%1  —  %2").arg(name, remote.url()),
                                     this, [this, name] { setRemote(name); });
    action->setCheckable(true);
    action->setChecked(name == mRemote);
  }

  menu.exec(QmlSupport::host(view())->mapToGlobal(x, y));
}

void RemoteDialog::setTags(bool tags) {
  if (tags == mTags)
    return;

  mTags = tags;
  emit changed();
}

void RemoteDialog::setPrune(bool prune) {
  if (prune == mPrune)
    return;

  mPrune = prune;
  emit changed();
}

QStringList RemoteDialog::actions() const {
  return {tr("Merge"), tr("Rebase"), tr("Merge (No Fast-forward)"),
          tr("Merge (Fast-forward Only)")};
}

void RemoteDialog::setAction(int action) {
  if (action == mAction || action < 0 || action >= kPullActions.size())
    return;

  mAction = action;
  emit changed();
}

void RemoteDialog::setRefIndex(int index) {
  if (index == mRefIndex)
    return;

  mRefIndex = index;

  // Push to the branch that the local branch merges from.
  git::Reference ref = mRefs.reference(index);
  QString value;
  if (ref.isValid()) {
    QString key = QString("branch.%1.merge").arg(ref.name());
    value = mRepo.gitConfig().value<QString>(key);
  }

  mRemoteRef = value;
  emit changed();
}

void RemoteDialog::setRemoteRef(const QString &ref) {
  if (ref == mRemoteRef)
    return;

  mRemoteRef = ref;
  emit changed();
}

void RemoteDialog::setSetUpstream(bool setUpstream) {
  if (setUpstream == mSetUpstream)
    return;

  mSetUpstream = setUpstream;
  emit changed();
}

void RemoteDialog::setForce(bool force) {
  if (force == mForce)
    return;

  mForce = force;
  emit changed();
}

void RemoteDialog::run() {
  RepoView *view = RepoView::parentView(this);
  QString name = mRemote.trimmed();
  git::Remote remote;
  for (const git::Remote &candidate : mRepo.remotes()) {
    if (candidate.name() == name)
      remote = candidate;
  }

  // Use a URL or an unknown name as an anonymous remote.
  if (!remote.isValid())
    remote = mRepo.anonymousRemote(name);

  switch (mKind) {
    case Fetch:
      view->fetch(remote, mTags, true, nullptr, nullptr, mPrune);
      break;

    case Pull: {
      RepoView::MergeFlags flags(kPullActions.value(mAction, RepoView::Merge));
      view->pull(flags, remote, mTags, mPrune);
      break;
    }

    case Push:
      view->push(remote, mRefs.reference(mRefIndex), mRemoteRef, mSetUpstream,
                 mForce, mTags);
      break;
  }
}
