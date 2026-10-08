//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "InteractiveRebase.h"
#include "DetailView.h"
#include "RepoView.h"
#include "git/Reference.h"
#include "git/Signature.h"
#include "log/LogEntry.h"
#include <QLocale>

namespace {

class Callbacks : public git::Repository::CheckoutCallbacks {
public:
  QStringList conflicts() const { return mConflicts; }

  int flags() const override { return GIT_CHECKOUT_NOTIFY_CONFLICT; }

  bool notify(char status, const QString &path) override {
    if (status == '!')
      mConflicts.append(path);
    return true;
  }

private:
  QStringList mConflicts;
};

} // namespace

InteractiveRebase::InteractiveRebase(RepoView *view)
    : QAbstractListModel(view), mView(view) {}

bool InteractiveRebase::isModified() const {
  QList<git::Commit> original =
      git::Rewrite(mView->repo()).commits(mTip, mOnto);
  if (original.size() != mItems.size())
    return true;

  for (int i = 0; i < mItems.size(); ++i) {
    const Item &item = mItems.at(i);
    if (item.action != Pick || item.message != item.commit.message() ||
        item.commit.id() != original.at(original.size() - 1 - i).id())
      return true;
  }

  return false;
}

QString InteractiveRebase::problem() const {
  for (int i = 0; i < mItems.size(); ++i) {
    const Item &item = mItems.at(i);
    if (item.action == Squash && !canSquash(i))
      return tr("The oldest commit can't be squashed.");
    if (item.action == Reword && item.message.trimmed().isEmpty())
      return tr("A reworded commit needs a message.");
  }

  return QString();
}

bool InteractiveRebase::canOpen(const QString &branch,
                                const git::Commit &onto) const {
  git::Reference ref = mView->repo().lookupRef(branch);
  if (!ref.isValid() || !ref.isLocalBranch() || !onto.isValid())
    return false;

  git::Commit tip = ref.target();
  return tip.isValid() && tip.id() != onto.id() &&
         !git::Rewrite(mView->repo()).commits(tip, onto).isEmpty();
}

bool InteractiveRebase::open(const QString &branch, const git::Commit &onto,
                             const QString &ontoName) {
  if (!canOpen(branch, onto))
    return false;

  git::Reference ref = mView->repo().lookupRef(branch);
  mBranch = branch;
  mBranchName = ref.name();
  mTip = ref.target();
  mOnto = onto;
  mOntoName = ontoName;
  load();
  setActive(true);
  return true;
}

void InteractiveRebase::setAction(int row, int action) {
  if (row < 0 || row >= mItems.size() || action < Pick || action > Drop)
    return;

  mItems[row].action = static_cast<Action>(action);

  // Other squashes may have lost or found the commit to squash into.
  emit dataChanged(index(0), index(mItems.size() - 1),
                   {ActionRole, CanSquashRole});
  emit changed();
}

void InteractiveRebase::setMessage(int row, const QString &message) {
  if (row < 0 || row >= mItems.size() || mItems.at(row).message == message)
    return;

  mItems[row].message = message;
  emit dataChanged(index(row), index(row), {MessageRole, SummaryRole});
  emit changed();
}

void InteractiveRebase::move(int from, int to) {
  if (from < 0 || from >= mItems.size() || to < 0 || to >= mItems.size() ||
      from == to)
    return;

  beginMoveRows(QModelIndex(), from, from, QModelIndex(),
                to > from ? to + 1 : to);
  mItems.move(from, to);
  endMoveRows();

  emit dataChanged(index(0), index(mItems.size() - 1), {CanSquashRole});
  emit changed();
}

void InteractiveRebase::reset() {
  load();
  emit changed();
}

void InteractiveRebase::cancel() {
  beginResetModel();
  mItems.clear();
  endResetModel();
  setActive(false);
}

bool InteractiveRebase::start() {
  if (!mActive || !problem().isEmpty())
    return false;

  git::Repository repo = mView->repo();
  QString text = tr("%1 onto %2").arg(mBranchName, mOntoName);
  LogEntry *entry = mView->addLogEntry(text, tr("Interactive Rebase"));
  auto fail = [this, entry](const QString &error) {
    LogEntry *result = entry->addEntry(LogEntry::Error, error);
    mView->setLogVisible(true);
    return result;
  };

  if (repo.state() != GIT_REPOSITORY_STATE_NONE) {
    fail(tr("Finish or abort the current merge or rebase first."));
    return false;
  }

  git::Reference ref = repo.lookupRef(mBranch);
  if (!ref.isValid() || ref.target().id() != mTip.id()) {
    fail(tr("%1 has changed since the rebase was opened.").arg(mBranchName));
    return false;
  }

  // Apply the commits in memory, the oldest first.
  QList<git::Rewrite::Step> steps;
  for (int i = mItems.size() - 1; i >= 0; --i) {
    const Item &item = mItems.at(i);
    steps.append({item.commit, static_cast<git::Rewrite::Action>(item.action),
                  item.message});
  }

  DetailView *details = mView->detailView();
  git::Signature committer = repo.defaultSignature(
      nullptr, details->overrideUser(), details->overrideEmail());
  git::Rewrite rewrite(repo);
  git::Commit tip = rewrite.apply(mOnto, steps, committer);
  if (!tip.isValid()) {
    LogEntry *error = fail(rewrite.error());
    QStringList conflicts = rewrite.conflicts();
    for (const QString &path : conflicts)
      error->addEntry(LogEntry::File, path)->setStatus('!');
    if (!conflicts.isEmpty())
      error->addEntry(
          LogEntry::Hint,
          tr("Reorder or drop commits to avoid the conflict, or rebase "
             "without changing the commits to resolve it."));
    return false;
  }

  if (tip.id() != mTip.id()) {
    // Check out the result when the branch is checked out.
    if (ref.isHead()) {
      Callbacks callbacks;
      if (!repo.checkout(tip, &callbacks)) {
        LogEntry *error =
            fail(tr("Your local changes would be overwritten by the rebase."));
        for (const QString &path : callbacks.conflicts())
          error->addEntry(LogEntry::File, path)->setStatus('!');
        error->addEntry(LogEntry::Hint,
                        tr("Stash or discard your changes, then try again."));
        return false;
      }
    }

    QString msg = QString("rebase (interactive): %1 onto %2")
                      .arg(mBranchName, mOnto.shortId());
    if (!ref.setTarget(tip, msg).isValid()) {
      mView->error(entry, tr("rebase"), mBranchName);
      return false;
    }
  }

  entry->addEntry(tr("%1 is now at %2").arg(mBranchName, tip.link()));
  cancel();
  return true;
}

void InteractiveRebase::select(int row) {
  if (row >= 0 && row < mItems.size())
    mView->selectCommit(mItems.at(row).commit, QString());
}

int InteractiveRebase::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : mItems.size();
}

QVariant InteractiveRebase::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= mItems.size())
    return QVariant();

  const Item &item = mItems.at(index.row());
  switch (role) {
    case ShortIdRole:
      return item.commit.shortId();
    case SummaryRole:
      return item.message.section('\n', 0, 0).trimmed();
    case MessageRole:
      return item.message;
    case AuthorRole:
      return item.commit.author().name();
    case InitialsRole:
      return item.commit.author().initials();
    case DateRole:
      return QLocale().toString(item.commit.committer().date(),
                                QLocale::ShortFormat);
    case ActionRole:
      return item.action;
    case CanSquashRole:
      return canSquash(index.row());
  }

  return QVariant();
}

QHash<int, QByteArray> InteractiveRebase::roleNames() const {
  return {{ShortIdRole, "shortId"}, {SummaryRole, "summary"},
          {MessageRole, "message"}, {AuthorRole, "author"},
          {InitialsRole, "initials"}, {DateRole, "date"},
          {ActionRole, "action"},   {CanSquashRole, "canSquash"}};
}

bool InteractiveRebase::canSquash(int row) const {
  for (int i = row + 1; i < mItems.size(); ++i) {
    if (mItems.at(i).action != Drop)
      return true;
  }

  return false;
}

void InteractiveRebase::load() {
  QList<git::Commit> commits =
      git::Rewrite(mView->repo()).commits(mTip, mOnto);

  beginResetModel();
  mItems.clear();
  for (auto it = commits.crbegin(); it != commits.crend(); ++it)
    mItems.append({*it, Pick, it->message()});
  endResetModel();
}

void InteractiveRebase::setActive(bool active) {
  if (mActive == active)
    return;

  mActive = active;
  emit activeChanged();
  emit changed();
}
