//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "RefState.h"
#include "Reference.h"
#include "Repository.h"
#include "git2/branch.h"
#include "git2/buffer.h"
#include "git2/config.h"
#include "git2/errors.h"
#include "git2/refs.h"
#include "git2/repository.h"
#include <algorithm>

namespace git {

namespace {

const QString kHeads = "refs/heads/";
const QString kTags = "refs/tags/";

QString configString(git_config *config, const QString &key) {
  git_buf buf = GIT_BUF_INIT;
  if (git_config_get_string_buf(&buf, config, key.toUtf8()))
    return QString();

  QString result = QString::fromUtf8(buf.ptr, buf.size);
  git_buf_dispose(&buf);
  return result;
}

void setConfigString(git_config *config, const QString &key,
                     const QString &value) {
  if (value.isEmpty()) {
    git_config_delete_entry(config, key.toUtf8());
  } else {
    git_config_set_string(config, key.toUtf8(), value.toUtf8());
  }
}

QString lastError() {
  const git_error *error = git_error_last();
  return error && error->message ? QString::fromUtf8(error->message)
                                 : QString();
}

QString shortName(const QString &name) {
  if (name.startsWith(kHeads))
    return name.mid(kHeads.length());
  if (name.startsWith(kTags))
    return name.mid(kTags.length());
  return name;
}

} // namespace

RefState::RefState(const Repository &repo) {
  if (!repo.isValid())
    return;

  git_repository *raw = repo;
  mValid = true;

  git_reference *head = nullptr;
  if (!git_reference_lookup(&head, raw, "HEAD")) {
    if (git_reference_type(head) == GIT_REFERENCE_SYMBOLIC)
      mHead = git_reference_symbolic_target(head);
    git_reference_free(head);
  }

  git_oid id;
  if (!git_reference_name_to_id(&id, raw, "HEAD"))
    mHeadId = Id(id);

  git_reference_iterator *it = nullptr;
  if (!git_reference_iterator_new(&it, raw)) {
    git_reference *ref = nullptr;
    while (!git_reference_next(&ref, it)) {
      QString name = git_reference_name(ref);
      if (git_reference_type(ref) == GIT_REFERENCE_DIRECT &&
          (isBranch(name) || isTag(name)))
        mRefs.insert(name, Id(git_reference_target(ref)));
      git_reference_free(ref);
    }
    git_reference_iterator_free(it);
  }

  git_config *config = nullptr;
  if (!git_repository_config_snapshot(&config, raw)) {
    for (auto it = mRefs.cbegin(); it != mRefs.cend(); ++it) {
      if (!isBranch(it.key()))
        continue;

      QString name = shortName(it.key());
      QString key = QString("branch.%1.").arg(name);
      QString remote = configString(config, key + "remote");
      QString merge = configString(config, key + "merge");
      if (!remote.isEmpty() || !merge.isEmpty())
        mUpstreams.insert(name, {remote, merge});
    }
    git_config_free(config);
  }
}

void RefState::addTags(const RefState &state) {
  for (auto it = state.mRefs.cbegin(); it != state.mRefs.cend(); ++it) {
    if (isTag(it.key()) && !mRefs.contains(it.key()))
      mRefs.insert(it.key(), it.value());
  }
}

QStringList RefState::changes(const RefState &state) const {
  QStringList names;
  if (mHead != state.mHead || mHeadId != state.mHeadId)
    names.append("HEAD");

  for (auto it = mRefs.cbegin(); it != mRefs.cend(); ++it) {
    auto other = state.mRefs.constFind(it.key());
    if (other == state.mRefs.cend() || other.value() != it.value())
      names.append(it.key());
  }

  for (auto it = state.mRefs.cbegin(); it != state.mRefs.cend(); ++it) {
    if (!mRefs.contains(it.key()))
      names.append(it.key());
  }

  return names;
}

bool RefState::matches(const RefState &state, const QStringList &names) const {
  for (const QString &name : names) {
    if (name == "HEAD") {
      if (mHead != state.mHead || mHeadId != state.mHeadId)
        return false;
    } else if (mRefs.contains(name) != state.mRefs.contains(name) ||
               mRefs.value(name) != state.mRefs.value(name)) {
      return false;
    }
  }

  return true;
}

bool RefState::operator==(const RefState &rhs) const {
  return mHead == rhs.mHead && mHeadId == rhs.mHeadId && mRefs == rhs.mRefs &&
         mUpstreams == rhs.mUpstreams;
}

bool RefState::canRestore(const Repository &repo, const RefState &current,
                          const QStringList &names, QString *error) const {
  // HEAD can't point to a branch that another worktree checked out, and that
  // branch can't move: its index and its files would stay behind.
  QStringList branches;
  if (names.contains("HEAD") && !mHead.isEmpty() && mHead != current.mHead)
    branches.append(mHead);

  for (const QString &name : names) {
    auto it = mRefs.constFind(name);
    auto old = current.mRefs.constFind(name);
    if (isBranch(name) && it != mRefs.cend() && old != current.mRefs.cend() &&
        it.value() != old.value())
      branches.append(name);
  }

  for (const QString &name : branches) {
    Reference ref = repo.lookupRef(name);
    if (ref.isValid() && ref.isCheckedOutElsewhere()) {
      if (error)
        *error =
            tr("'%1' is checked out in another worktree.").arg(shortName(name));
      return false;
    }
  }

  return true;
}

bool RefState::restore(const Repository &repo, const RefState &current,
                       const QStringList &names, const QString &message,
                       QString *error) const {
  QString text;
  if (change(repo, current, names, message, text))
    return true;

  // Change back what was changed already, so that it can be tried again.
  QString ignored;
  current.change(repo, RefState(repo), names, message, ignored);

  if (error)
    *error = text;
  return false;
}

bool RefState::change(const Repository &repo, const RefState &current,
                      const QStringList &names, const QString &message,
                      QString &error) const {
  if (!canRestore(repo, current, names, &error))
    return false;

  git_repository *raw = repo;
  RepositoryNotifier *notifier = repo.notifier();
  QByteArray log = message.toUtf8();

  auto fail = [&error](const QString &text) {
    error = text;
    return false;
  };

  QStringList created;
  QStringList moved;
  QStringList removed;
  for (const QString &name : names) {
    auto it = mRefs.constFind(name);
    auto old = current.mRefs.constFind(name);
    bool has = (it != mRefs.cend());
    bool had = (old != current.mRefs.cend());
    if (has && !had) {
      created.append(name);
    } else if (has && old.value() != it.value()) {
      moved.append(name);
    } else if (had && !has) {
      removed.append(name);
    }
  }

  auto remove = [&](const QString &name) {
    Reference ref = repo.lookupRef(name);
    if (!ref.isValid())
      return true;

    emit notifier->referenceAboutToBeRemoved(ref);

    // libgit2 refuses to delete the branch of HEAD, which is unborn again
    // when the first commit is undone.
    bool unborn = (name == current.mHead && name == mHead);
    git_reference *handle = ref;
    int result = (isBranch(name) && !unborn) ? git_branch_delete(handle)
                                             : git_reference_delete(handle);
    if (result) {
      return fail(
          tr("Unable to remove '%1' - %2").arg(shortName(name), lastError()));
    }

    emit notifier->referenceRemoved(shortName(name));
    return true;
  };

  // A reference can't be created while another one has its name as a
  // directory, or the name of one of its directories, like 'a' and 'a/b'
  // after a rename. Remove those first. HEAD points to its branch below.
  bool restoresHead = names.contains("HEAD");
  QStringList blocking;
  for (const QString &name : removed) {
    bool blocks = std::any_of(
        created.cbegin(), created.cend(), [&name](const QString &other) {
          return other.startsWith(name + '/') || name.startsWith(other + '/');
        });
    if (!blocks)
      continue;

    if (restoresHead && name == current.mHead &&
        git_repository_set_head_detached(raw, current.mHeadId))
      return fail(tr("Unable to restore HEAD - %1").arg(lastError()));

    if (!remove(name))
      return false;

    blocking.append(name);
  }

  // Create and move references next so that HEAD can point to them.
  for (const QString &name : created + moved) {
    bool exists = current.mRefs.contains(name);
    if (!exists)
      emit notifier->referenceAboutToBeAdded(shortName(name));

    git_reference *ref = nullptr;
    if (git_reference_create(&ref, raw, name.toUtf8(), mRefs.value(name), 1,
                             log)) {
      return fail(
          tr("Unable to restore '%1' - %2").arg(shortName(name), lastError()));
    }

    git_reference_free(ref);

    Reference restored = repo.lookupRef(name);
    if (exists) {
      emit notifier->referenceUpdated(restored);
    } else {
      emit notifier->referenceAdded(restored);
    }
  }

  // Point HEAD to its branch or commit.
  if (restoresHead && (mHead != current.mHead || mHeadId != current.mHeadId)) {
    int result = 0;
    if (!mHead.isEmpty()) {
      if (mHead != current.mHead)
        result = git_repository_set_head(raw, mHead.toUtf8());
    } else if (mHeadId.isValid()) {
      result = git_repository_set_head_detached(raw, mHeadId);
    }

    if (result)
      return fail(tr("Unable to restore HEAD - %1").arg(lastError()));
  }

  // Remove the references that didn't exist.
  for (const QString &name : removed) {
    if (!blocking.contains(name) && !remove(name))
      return false;
  }

  // Branches that are created again get their upstreams back, which were
  // removed with them.
  git_config *config = nullptr;
  if (!git_repository_config(&config, raw)) {
    for (const QString &name : created) {
      if (!isBranch(name))
        continue;

      QString branch = shortName(name);
      QPair<QString, QString> upstream = mUpstreams.value(branch);
      QString key = QString("branch.%1.").arg(branch);
      if (configString(config, key + "remote") != upstream.first)
        setConfigString(config, key + "remote", upstream.first);
      if (configString(config, key + "merge") != upstream.second)
        setConfigString(config, key + "merge", upstream.second);
    }

    git_config_free(config);
  }

  emit notifier->referenceUpdated(repo.head());
  return true;
}

bool RefState::isBranch(const QString &name) { return name.startsWith(kHeads); }

bool RefState::isTag(const QString &name) { return name.startsWith(kTags); }

} // namespace git
