//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef ANNOTATEDCOMMIT_H
#define ANNOTATEDCOMMIT_H

#include <QSharedPointer>
#include <QString>

struct git_annotated_commit;
struct git_repository;

namespace git {

class Commit;
class Repository;

class AnnotatedCommit {
public:
  AnnotatedCommit();

  bool isValid() const { return !d.isNull(); }

  Commit commit() const;

  int analysis() const;

  // The message of a merge of this commit, like libgit2 writes MERGE_MSG.
  QString mergeMessage() const;

private:
  AnnotatedCommit(git_annotated_commit *commit, git_repository *repo,
                  const QString &url = QString());
  operator git_annotated_commit *() const;

  git_repository *repo;
  QSharedPointer<git_annotated_commit> d;
  QString url; // of the remote, for a commit from FETCH_HEAD

  friend class Commit;
  friend class Branch;
  friend class Reference;
  friend class Repository;
};

} // namespace git

#endif
