//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef REFERENCEITEMS_H
#define REFERENCEITEMS_H

#include "git/Reference.h"
#include <QVariantList>

namespace git {
class Commit;
class Repository;
} // namespace git

// References listed by the combo boxes of QML dialogs. Each item has a
// 'text', an 'icon' and the short id of its target as 'detail'.
class ReferenceItems {
public:
  enum Kind {
    DetachedHead = 0x1,
    LocalBranches = 0x2,
    RemoteBranches = 0x4,
    Tags = 0x8,
    AllRefs = DetachedHead | LocalBranches | RemoteBranches | Tags,
    // Leave out the branch that HEAD points to.
    ExcludeHead = 0x10
  };

  // 'none' is the text of a first item without a reference, if any.
  ReferenceItems(const git::Repository &repo, int kinds,
                 const QString &none = QString());

  QVariantList items() const { return mItems; }
  int count() const { return mItems.size(); }

  git::Reference reference(int index) const;
  int indexOf(const git::Reference &ref) const;

  // The best reference that points to the commit: a local branch, then a
  // tag, then a remote branch. -1 if there isn't any.
  int indexOf(const git::Commit &commit) const;

  // Insert an item without a reference at the top, like a commit.
  void prepend(const QVariantMap &item);

private:
  void add(const git::Reference &ref, const QString &icon);

  QList<git::Reference> mRefs;
  QVariantList mItems;
};

#endif
