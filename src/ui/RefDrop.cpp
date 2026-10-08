//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "RefDrop.h"
#include "InteractiveRebase.h"
#include "RepoView.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Remote.h"
#include "log/LogEntry.h"
#include "qml/QmlSupport.h"
#include <QMenu>

namespace {

const QString kRemotePrefix = "remote:";

} // namespace

RefDrop::RefDrop(RepoView *view) : QObject(view), mView(view) {}

void RefDrop::start(const QString &name, qreal x, qreal y) {
  git::Reference ref = mView->repo().lookupRef(name);
  if (!ref.isValid())
    return;

  mSource = name;
  mLabel = ref.name();
  mTarget.clear();
  mPos = QPointF(x, y);
  emit changed();
  emit moved();
}

void RefDrop::move(qreal x, qreal y) {
  mPos = QPointF(x, y);
  emit moved();
}

void RefDrop::finish() {
  if (mSource.isEmpty())
    return;

  // The target calls drop().
  emit released();

  QString source = mSource;
  QString target = mTarget;
  QPointF pos = mDropPos;
  cancel();

  if (!target.isEmpty())
    showMenu(source, target, pos);
}

void RefDrop::cancel() {
  mTarget.clear();
  if (mSource.isEmpty())
    return;

  mSource.clear();
  mLabel.clear();
  emit changed();
}

bool RefDrop::accepts(const QString &target) const {
  return !mSource.isEmpty() && !choices(mSource, target).isEmpty();
}

void RefDrop::drop(const QString &target, qreal x, qreal y) {
  mTarget = target;
  mDropPos = QPointF(x, y);
}

QList<RefDrop::Choice> RefDrop::choices(const QString &source,
                                        const QString &target) const {
  QList<Choice> result;
  git::Repository repo = mView->repo();
  git::Reference src = repo.lookupRef(source);
  if (!src.isValid() || source == target || src.isStash())
    return result;

  RepoView *view = mView;
  QString srcName = src.name();
  git::Commit srcCommit = src.target();

  // Push a local branch to a remote.
  if (target.startsWith(kRemotePrefix)) {
    git::Remote remote = repo.lookupRemote(target.mid(kRemotePrefix.length()));
    if (!src.isLocalBranch() || !remote.isValid())
      return result;

    result.append({tr("Push %1 to %2").arg(srcName, remote.name()),
                   [view, source, remote] {
                     git::Branch branch = view->repo().lookupRef(source);
                     view->push(remote, branch, QString(),
                                !branch.upstream().isValid());
                   }});
    return result;
  }

  git::Reference dst = repo.lookupRef(target);
  if (!dst.isValid() || dst.isTag() || dst.isStash() ||
      !(dst.isLocalBranch() || dst.isRemoteBranch()))
    return result;

  QString dstName = dst.name();
  git::Commit dstCommit = dst.target();
  if (!srcCommit.isValid() || !dstCommit.isValid())
    return result;

  bool same = (srcCommit.id() == dstCommit.id());
  git::Commit base = same ? srcCommit : repo.mergeBase(dstCommit, srcCommit);
  // The target is behind, and the source can't be merged or rebased.
  bool behind = !same && base.isValid() && base.id() == dstCommit.id();
  bool merged = same || (base.isValid() && base.id() == srcCommit.id());

  // Check out a branch first, unless it's already checked out.
  auto checkout = [view](const QString &name) {
    git::Reference ref = view->repo().lookupRef(name);
    if (!ref.isHead())
      view->checkout(ref);
    return view->repo().head().qualifiedName() == name;
  };

  if (dst.isRemoteBranch()) {
    if (!src.isLocalBranch())
      return result;

    // Push to the remote branch.
    git::Remote remote = git::Branch(dst).remote();
    QString branch = dstName.section('/', 1);
    if (remote.isValid() && !same) {
      result.append({tr("Push %1 to %2").arg(srcName, dstName),
                     [view, source, remote, branch] {
                       git::Reference ref = view->repo().lookupRef(source);
                       view->push(remote, ref, branch);
                     }});
    }

    if (!same && !behind) {
      result.append({tr("Rebase %1 onto %2").arg(srcName, dstName),
                     [view, source, target, checkout] {
                       if (checkout(source))
                         view->merge(RepoView::Rebase,
                                     view->repo().lookupRef(target));
                     }});
      addInteractive(result, source, dst);
    }

    return result;
  }

  if (behind) {
    result.append({tr("Fast-forward %1 to %2").arg(dstName, srcName),
                   [view, source, target, dstName, srcName] {
                     git::Repository repo = view->repo();
                     git::Reference ref = repo.lookupRef(target);
                     git::Reference upstream = repo.lookupRef(source);
                     if (ref.isHead()) {
                       view->merge(RepoView::FastForward, upstream);
                       return;
                     }

                     // Move a branch that isn't checked out.
                     LogEntry *entry =
                         view->addLogEntry(tr("%1 to %2").arg(dstName, srcName),
                                           tr("Fast-forward"));
                     QString msg = QString("fast-forward: %1").arg(srcName);
                     if (!ref.setTarget(upstream.target(), msg).isValid())
                       view->error(entry, tr("fast-forward"), dstName);
                   }});
  }

  if (!merged) {
    result.append({tr("Merge %1 into %2").arg(srcName, dstName),
                   [view, source, target, checkout] {
                     if (checkout(target))
                       view->merge(RepoView::Merge,
                                   view->repo().lookupRef(source));
                   }});
  }

  if (src.isLocalBranch() && !same && !behind) {
    result.append({tr("Rebase %1 onto %2").arg(srcName, dstName),
                   [view, source, target, checkout] {
                     if (checkout(source))
                       view->merge(RepoView::Rebase,
                                   view->repo().lookupRef(target));
                   }});
    addInteractive(result, source, dst);
  }

  return result;
}

void RefDrop::addInteractive(QList<Choice> &choices, const QString &source,
                             const git::Reference &target) const {
  InteractiveRebase *rebase = mView->interactiveRebase();
  git::Commit onto = target.target();
  if (!rebase->canOpen(source, onto))
    return;

  RepoView *view = mView;
  QString name = target.name();
  QString text = tr("Interactive Rebase %1 onto %2")
                     .arg(mView->repo().lookupRef(source).name(), name);
  choices.append({text, [view, source, onto, name] {
                    view->interactiveRebase()->open(source, onto, name);
                  }});
}

void RefDrop::showMenu(const QString &source, const QString &target,
                       const QPointF &pos) {
  QList<Choice> choices = this->choices(source, target);
  if (choices.isEmpty())
    return;

  QMenu menu;
  for (const Choice &choice : choices)
    menu.addAction(choice.text, choice.run);

  QmlSupport::execMenu(&menu, mView->mapFromPage(pos.x(), pos.y()));
}
