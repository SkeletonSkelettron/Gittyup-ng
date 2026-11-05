//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REVWALK_H
#define REVWALK_H

#include "Id.h"
#include <QList>
#include <QSharedPointer>

struct git_revwalk;

namespace git {

class Commit;
class Reference;

class RevWalk {
public:
  RevWalk();

  bool isValid() const { return !d.isNull(); }

  bool hide(const Commit &commit);
  bool hide(const Reference &ref);

  bool push(const Commit &commit);
  bool push(const Reference &ref);

  // Walk only the newest commits, at most count of them, if it's positive.
  // A sorted walk reads the whole history before the first commit otherwise.
  // Call it after the last push.
  void limit(int count);

  // Return the next commit that matches the given pathspec.
  Commit next(const QString &pathspec = QString()) const;

protected:
  RevWalk(git_revwalk *walker);

  QSharedPointer<git_revwalk> d;

  // The pushed commits and whether only the first parents are walked, for
  // limit().
  QList<Id> mTips;
  bool mFirstParent = false;

  friend class Commit;
  friend class Reference;
  friend class Repository;
};

} // namespace git

#endif
