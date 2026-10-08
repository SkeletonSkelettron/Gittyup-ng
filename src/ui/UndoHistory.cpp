//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "UndoHistory.h"
#include "DetailView.h"
#include "RepoView.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include "log/LogEntry.h"
#include <QTextDocumentFragment>

namespace {

// Actions are forgotten after this many.
const int kMaxActions = 100;

QString shortName(const QString &name) {
  return name.section('/', 2);
}

QString shortId(const git::Id &id) { return id.toString().left(7); }

QString plainText(const QString &html) {
  return QTextDocumentFragment::fromHtml(html).toPlainText().simplified();
}

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

UndoHistory::UndoHistory(RepoView *view) : QObject(view), mView(view) {
  mState = git::RefState(view->repo());

  // Record changes when the event loop is back after an action.
  mTimer.setSingleShot(true);
  connect(&mTimer, &QTimer::timeout, this, &UndoHistory::check);

  auto schedule = [this] { mTimer.start(0); };
  git::RepositoryNotifier *notifier = view->repo().notifier();
  connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
          schedule);
  connect(notifier, &git::RepositoryNotifier::referenceAdded, this, schedule);
  connect(notifier, &git::RepositoryNotifier::referenceRemoved, this,
          schedule);
  connect(notifier, &git::RepositoryNotifier::stateChanged, this, schedule);
}

void UndoHistory::begin(LogEntry *entry) {
  // Changes that weren't recorded yet belong to the previous action.
  check();

  mEntry = entry;
  mMode = Checkout;
}

void UndoHistory::setMode(Mode mode) { mMode = mode; }

bool UndoHistory::canUndo() const { return !mUndo.isEmpty(); }

bool UndoHistory::canRedo() const { return !mRedo.isEmpty(); }

QString UndoHistory::undoText() const {
  return mUndo.isEmpty() ? QString() : mUndo.last().text;
}

QString UndoHistory::redoText() const {
  return mRedo.isEmpty() ? QString() : mRedo.last().text;
}

bool UndoHistory::undo() {
  check();
  if (mUndo.isEmpty())
    return false;

  Action action = mUndo.last();
  if (!apply(action, true))
    return false;

  mUndo.removeLast();
  mRedo.append(action);
  emit changed();
  return true;
}

bool UndoHistory::redo() {
  check();
  if (mRedo.isEmpty())
    return false;

  Action action = mRedo.last();
  if (!apply(action, false))
    return false;

  mRedo.removeLast();
  mUndo.append(action);
  emit changed();
  return true;
}

void UndoHistory::check() {
  mTimer.stop();
  if (mApplying)
    return;

  // Wait until merges, rebases, cherry-picks and reverts are finished.
  git::Repository repo = mView->repo();
  if (repo.state() != GIT_REPOSITORY_STATE_NONE)
    return;

  git::RefState state(repo);
  if (!mState.isValid() || state == mState) {
    mState = state;
    return;
  }

  Action action;
  action.before = mState;
  action.after = state;
  mState = state;

  // Tags are only undone when they are created by hand, not by fetching.
  if (!mEntry || mEntry->title() != RepoView::tr("Tag"))
    action.before.addTags(action.after);

  if (action.before.changes(action.after).isEmpty())
    return;

  action.text = describe(action.before, action.after);
  action.mode = mMode;

  // Undoing a commit leaves its changes and its message to commit again.
  if (mMode == Soft) {
    git::Commit commit = repo.lookupCommit(action.after.headId());
    if (commit.isValid() && commit.parents().value(0).id() ==
                                action.before.headId())
      action.message = commit.message();
  }

  mEntry = nullptr;
  mMode = Checkout;

  mUndo.append(action);
  while (mUndo.size() > kMaxActions)
    mUndo.removeFirst();
  mRedo.clear();

  emit changed();
}

bool UndoHistory::apply(const Action &action, bool undo) {
  git::Repository repo = mView->repo();
  const git::RefState &from = undo ? action.after : action.before;
  const git::RefState &to = undo ? action.before : action.after;

  mApplying = true;
  QString title = undo ? tr("Undo") : tr("Redo");
  LogEntry *entry = mView->addLogEntry(action.text, title);

  auto finish = [this](bool result) {
    mApplying = false;
    mEntry = nullptr;
    mMode = Checkout;
    mState = git::RefState(mView->repo());
    return result;
  };

  auto fail = [this, entry, &finish](const QString &text) {
    entry->addEntry(LogEntry::Error, text);
    mView->setLogVisible(true);
    return finish(false);
  };

  if (repo.state() != GIT_REPOSITORY_STATE_NONE)
    return fail(tr("Finish or abort the current merge, rebase, cherry-pick "
                   "or revert first."));

  git::RefState current(repo);
  if (!current.matches(from, action.before.changes(action.after)))
    return fail(tr("The branches have changed since."));

  // Move the working directory to the commit of HEAD.
  git::Commit commit;
  if (from.headId() != to.headId() && to.headId().isValid()) {
    commit = repo.lookupCommit(to.headId());
    if (!commit.isValid())
      return fail(tr("The commit %1 doesn't exist.").arg(shortId(to.headId())));

    if (action.mode == Checkout) {
      Callbacks callbacks;
      if (!repo.checkout(commit, &callbacks)) {
        QStringList conflicts = callbacks.conflicts();
        if (conflicts.isEmpty())
          return fail(git::Repository::lastError());

        LogEntry *error = entry->addEntry(
            LogEntry::Error, tr("Your local changes would be overwritten."));
        for (const QString &path : conflicts)
          error->addEntry(LogEntry::File, path)->setStatus('!');
        error->addEntry(LogEntry::Hint,
                        tr("Stash or discard your changes, then try again."));
        mView->setLogVisible(true);
        return finish(false);
      }
    }
  }

  QString message = QString("%1: %2").arg(title.toLower(), action.text);
  QString error;
  if (!to.restore(repo, current, message, &error))
    return fail(error);

  if (action.mode == Mixed && commit.isValid())
    commit.reset(GIT_RESET_MIXED);

  // Offer the message of an undone commit to commit it again.
  if (!action.message.isEmpty()) {
    DetailView *details = mView->detailView();
    QString text = details->commitMessage();
    if (undo && text.trimmed().isEmpty()) {
      details->setCommitMessage(action.message.trimmed());
    } else if (!undo && text.trimmed() == action.message.trimmed()) {
      details->setCommitMessage(QString());
    }
  }

  mView->refresh(false);
  return finish(true);
}

QString UndoHistory::describe(const git::RefState &before,
                              const git::RefState &after) const {
  QStringList added;
  QStringList removed;
  QStringList moved;
  for (const QString &name : before.changes(after)) {
    if (name == "HEAD")
      continue;

    bool had = before.refs().contains(name);
    bool has = after.refs().contains(name);
    if (had && has) {
      moved.append(name);
    } else if (has) {
      added.append(name);
    } else {
      removed.append(name);
    }
  }

  // Actions that move branches are described by their log entry.
  bool headMoved =
      before.head() == after.head() && before.headId() != after.headId();
  if ((headMoved || !moved.isEmpty()) && mEntry) {
    QString text = plainText(mEntry->text());
    return text.isEmpty() ? mEntry->title()
                          : tr("%1 %2").arg(mEntry->title(), text);
  }

  if (before.head() != after.head() ||
      (after.head().isEmpty() && before.headId() != after.headId())) {
    QString name = after.head().isEmpty() ? shortId(after.headId())
                                          : shortName(after.head());
    return tr("Checkout %1").arg(name);
  }

  if (added.size() == 1 && removed.size() == 1 &&
      git::RefState::isBranch(added.first()) &&
      git::RefState::isBranch(removed.first()) &&
      after.refs().value(added.first()) ==
          before.refs().value(removed.first())) {
    return tr("Rename Branch %1 to %2")
        .arg(shortName(removed.first()), shortName(added.first()));
  }

  if (!added.isEmpty()) {
    QString name = added.first();
    return git::RefState::isBranch(name)
               ? tr("Create Branch %1").arg(shortName(name))
               : tr("Create Tag %1").arg(shortName(name));
  }

  if (!removed.isEmpty()) {
    QString name = removed.first();
    return git::RefState::isBranch(name)
               ? tr("Delete Branch %1").arg(shortName(name))
               : tr("Delete Tag %1").arg(shortName(name));
  }

  QString name = moved.isEmpty() ? after.head() : moved.first();
  return tr("Update %1").arg(shortName(name));
}
