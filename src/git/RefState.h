//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef GIT_REFSTATE_H
#define GIT_REFSTATE_H

#include "Id.h"
#include <QCoreApplication>
#include <QMap>
#include <QPair>
#include <QStringList>

namespace git {

class Repository;

// The HEAD, local branches and tags of a repository at one time, and the
// upstreams of its branches. Actions are undone by restoring the state from
// before them, and redone by restoring the state after them.
class RefState {
  Q_DECLARE_TR_FUNCTIONS(RefState)

public:
  RefState() = default;
  // Read the current state of 'repo'.
  explicit RefState(const Repository &repo);

  bool isValid() const { return mValid; }

  // The qualified name of the branch that HEAD points to, or an empty
  // string when HEAD is detached.
  QString head() const { return mHead; }
  // The commit of HEAD, which is invalid when HEAD is unborn.
  Id headId() const { return mHeadId; }

  // The targets of 'refs/heads/*' and 'refs/tags/*'.
  const QMap<QString, Id> &refs() const { return mRefs; }
  // The remote and merge ref of each local branch with an upstream, by the
  // short name of the branch.
  const QMap<QString, QPair<QString, QString>> &upstreams() const {
    return mUpstreams;
  }

  // Add the tags of 'state' that this state doesn't have.
  void addTags(const RefState &state);

  // The names of the references whose targets differ from 'state', and
  // "HEAD" when HEAD differs.
  QStringList changes(const RefState &state) const;

  // Whether the references 'names' are the same in both states.
  bool matches(const RefState &state, const QStringList &names) const;

  bool operator==(const RefState &rhs) const;
  bool operator!=(const RefState &rhs) const { return !(*this == rhs); }

  // Change the references of 'repo' from 'current' to this state. The index
  // and the working directory don't change. The reflogs get 'message'.
  // Returns false and sets 'error' when something couldn't be changed.
  bool restore(const Repository &repo, const RefState &current,
               const QString &message, QString *error = nullptr) const;

  static bool isBranch(const QString &name);
  static bool isTag(const QString &name);

private:
  bool mValid = false;
  QString mHead;
  Id mHeadId;
  QMap<QString, Id> mRefs;
  QMap<QString, QPair<QString, QString>> mUpstreams;
};

} // namespace git

#endif
