//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Rewrite.h"
#include "Signing.h"
#include "Tree.h"
#include "git2/cherrypick.h"
#include "git2/commit.h"
#include "git2/errors.h"
#include "git2/index.h"
#include "git2/revwalk.h"
#include "git2/tree.h"

namespace git {

namespace {

QString lastError() {
  const git_error *error = git_error_last();
  return error && error->message ? QString::fromUtf8(error->message)
                                 : QString();
}

// The message of a squashed commit, like 'git rebase -i' combines them.
QString combine(const QString &first, const QString &second) {
  return first.trimmed() + "\n\n" + second.trimmed() + "\n";
}

} // namespace

Rewrite::Rewrite(const Repository &repo) : mRepo(repo) {}

QList<Commit> Rewrite::commits(const Commit &tip, const Commit &onto) const {
  QList<Commit> result;
  git_revwalk *walk = nullptr;
  if (git_revwalk_new(&walk, mRepo))
    return result;

  git_revwalk_sorting(walk, GIT_SORT_TOPOLOGICAL | GIT_SORT_REVERSE);
  git_revwalk_push(walk, git_commit_id(tip));
  if (onto.isValid())
    git_revwalk_hide(walk, git_commit_id(onto));

  git_oid id;
  while (!git_revwalk_next(&id, walk)) {
    Commit commit = mRepo.lookupCommit(Id(id));
    if (commit.isValid() && !commit.isMerge())
      result.append(commit);
  }

  git_revwalk_free(walk);
  return result;
}

Commit Rewrite::apply(const Commit &onto, const QList<Step> &steps,
                      const Signature &committer) {
  mError.clear();
  mFailed = Commit();
  mConflicts.clear();

  git_repository *repo = mRepo;
  Commit current = onto;
  // Whether a commit of this rewrite comes before, to squash into it.
  bool previous = false;

  for (const Step &step : steps) {
    if (step.action == Drop)
      continue;

    if (step.action == Squash && !previous)
      return fail(tr("There is no commit before %1 to squash it into.")
                      .arg(step.commit.shortId()),
                  step.commit);

    // Keep a picked commit that's already on top.
    QList<Commit> parents = step.commit.parents();
    if (step.action == Pick && parents.size() == 1 &&
        parents.first().id() == current.id()) {
      current = step.commit;
      previous = true;
      continue;
    }

    // Apply the changes of the commit onto the current one.
    git_index *index = nullptr;
    git_merge_options options = GIT_MERGE_OPTIONS_INIT;
    if (git_cherrypick_commit(&index, repo, step.commit, current, 0,
                              &options))
      return fail(lastError(), step.commit);

    if (git_index_has_conflicts(index)) {
      git_index_conflict_iterator *it = nullptr;
      if (!git_index_conflict_iterator_new(&it, index)) {
        const git_index_entry *ancestor, *ours, *theirs;
        while (!git_index_conflict_next(&ancestor, &ours, &theirs, it)) {
          const git_index_entry *entry =
              ours ? ours : theirs ? theirs : ancestor;
          if (entry && !mConflicts.contains(entry->path))
            mConflicts.append(QString::fromUtf8(entry->path));
        }
        git_index_conflict_iterator_free(it);
      }

      git_index_free(index);
      return fail(tr("%1 conflicts with the commits before it.")
                      .arg(step.commit.shortId()),
                  step.commit);
    }

    git_oid treeId;
    int error = git_index_write_tree_to(&treeId, index, repo);
    git_index_free(index);
    if (error)
      return fail(lastError(), step.commit);

    Commit base;
    QString message;
    Signature author;
    if (step.action == Squash) {
      // Replace the commit before with both.
      message = combine(current.message(), step.commit.message());
      author = current.author();
      QList<Commit> previousParents = current.parents();
      base = previousParents.value(0);
    } else {
      // Drop commits whose changes are already there.
      if (git_oid_equal(&treeId, git_commit_tree_id(current)))
        continue;

      message = (step.action == Reword) ? step.message : step.commit.message();
      author = step.commit.author();
      base = current;
    }

    git_tree *tree = nullptr;
    if (git_tree_lookup(&tree, repo, &treeId))
      return fail(lastError(), step.commit);

    QVector<const git_commit *> parentList;
    if (base.isValid())
      parentList.append(base);

    git_oid id;
    QByteArray raw = message.toUtf8();
    error = Signing::createCommit(&id, repo, nullptr, author, committer,
                                  raw.constData(), tree, parentList.size(),
                                  parentList.data());
    git_tree_free(tree);
    if (error)
      return fail(lastError(), step.commit);

    current = mRepo.lookupCommit(Id(id));
    previous = true;
  }

  return current;
}

Commit Rewrite::fail(const QString &error, const Commit &commit) {
  mError = error;
  mFailed = commit;
  return Commit();
}

} // namespace git
