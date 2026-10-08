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

bool RefState::restore(const Repository &repo, const RefState &current,
                       const QString &message, QString *error) const {
  git_repository *raw = repo;
  RepositoryNotifier *notifier = repo.notifier();
  QByteArray log = message.toUtf8();

  auto fail = [error](const QString &text) {
    if (error)
      *error = text;
    return false;
  };

  // Create and move references first so that HEAD can point to them.
  for (auto it = mRefs.cbegin(); it != mRefs.cend(); ++it) {
    const QString &name = it.key();
    auto old = current.mRefs.constFind(name);
    bool exists = (old != current.mRefs.cend());
    if (exists && old.value() == it.value())
      continue;

    if (!exists)
      emit notifier->referenceAboutToBeAdded(shortName(name));

    git_reference *ref = nullptr;
    if (git_reference_create(&ref, raw, name.toUtf8(), it.value(), 1, log)) {
      return fail(tr("Unable to restore '%1' - %2")
                      .arg(shortName(name), lastError()));
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
  if (mHead != current.mHead || mHeadId != current.mHeadId) {
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
  for (auto it = current.mRefs.cbegin(); it != current.mRefs.cend(); ++it) {
    const QString &name = it.key();
    if (mRefs.contains(name))
      continue;

    Reference ref = repo.lookupRef(name);
    if (!ref.isValid())
      continue;

    emit notifier->referenceAboutToBeRemoved(ref);

    git_reference *handle = ref;
    int result = isBranch(name) ? git_branch_delete(handle)
                                : git_reference_delete(handle);
    if (result) {
      return fail(tr("Unable to remove '%1' - %2")
                      .arg(shortName(name), lastError()));
    }

    emit notifier->referenceRemoved(shortName(name));
  }

  // Restore the upstreams of the branches.
  git_config *config = nullptr;
  if (!git_repository_config(&config, raw)) {
    for (auto it = mRefs.cbegin(); it != mRefs.cend(); ++it) {
      if (!isBranch(it.key()))
        continue;

      QString name = shortName(it.key());
      QPair<QString, QString> upstream = mUpstreams.value(name);
      QString key = QString("branch.%1.").arg(name);
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
