//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef GIT_REWRITE_H
#define GIT_REWRITE_H

#include "Commit.h"
#include "Repository.h"
#include "Signature.h"
#include <QCoreApplication>
#include <QStringList>

namespace git {

// Commits rewritten in memory like an interactive rebase: each commit is
// picked onto the new base, reworded, squashed into the commit before it or
// dropped. Nothing changes in the repository until a branch is moved to the
// result.
class Rewrite {
  Q_DECLARE_TR_FUNCTIONS(Rewrite)

public:
  enum Action { Pick, Reword, Squash, Drop };

  struct Step {
    Commit commit;
    Action action = Pick;
    // The new message of a reworded commit.
    QString message;
  };

  Rewrite(const Repository &repo);

  // The commits of 'tip' that 'onto' doesn't have, the oldest first and
  // without merges, like the ones that 'git rebase -i' shows.
  QList<Commit> commits(const Commit &tip, const Commit &onto) const;

  // Apply the steps onto 'onto', the oldest first. Returns the new tip, or
  // an invalid commit when a step fails.
  Commit apply(const Commit &onto, const QList<Step> &steps,
               const Signature &committer);

  QString error() const { return mError; }
  // The commit that couldn't be applied, and the files that conflicted.
  Commit failed() const { return mFailed; }
  QStringList conflicts() const { return mConflicts; }

private:
  Commit fail(const QString &error, const Commit &commit = Commit());

  Repository mRepo;
  QString mError;
  Commit mFailed;
  QStringList mConflicts;
};

} // namespace git

#endif
