//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef INTERACTIVEREBASE_H
#define INTERACTIVEREBASE_H

#include "git/Commit.h"
#include "git/Rewrite.h"
#include <QAbstractListModel>

class RepoView;

// The commits of a branch rebased interactively like in GitKraken: each is
// picked, reworded, squashed into the commit below it or dropped, and the
// commits can be reordered. The rows show the newest commit first.
// qrc:/qml/RebasePanel.qml draws it as 'interactiveRebase'.
class InteractiveRebase : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
  Q_PROPERTY(QString branch READ branch NOTIFY activeChanged)
  Q_PROPERTY(QString onto READ onto NOTIFY activeChanged)
  Q_PROPERTY(bool modified READ isModified NOTIFY changed)
  // Why the rebase can't start, or an empty string.
  Q_PROPERTY(QString problem READ problem NOTIFY changed)

public:
  enum Action {
    Pick = git::Rewrite::Pick,
    Reword = git::Rewrite::Reword,
    Squash = git::Rewrite::Squash,
    Drop = git::Rewrite::Drop
  };

  enum Role {
    ShortIdRole = Qt::UserRole,
    SummaryRole,
    MessageRole,
    AuthorRole,
    InitialsRole,
    DateRole,
    ActionRole,
    // A commit below that isn't dropped, to squash into.
    CanSquashRole
  };

  InteractiveRebase(RepoView *view);

  bool isActive() const { return mActive; }
  QString branch() const { return mBranchName; }
  QString onto() const { return mOntoName; }
  bool isModified() const;
  QString problem() const;

  // Edit the rebase of the local branch 'branch' onto 'onto'. Returns false
  // when there are no commits to rebase.
  bool open(const QString &branch, const git::Commit &onto,
            const QString &ontoName);
  // Whether 'branch' has commits to rebase onto 'onto'.
  bool canOpen(const QString &branch, const git::Commit &onto) const;

  Q_INVOKABLE void setAction(int row, int action);
  Q_INVOKABLE void setMessage(int row, const QString &message);
  Q_INVOKABLE void move(int from, int to);
  // Undo the changes to the actions, messages and order.
  Q_INVOKABLE void reset();
  Q_INVOKABLE void cancel();
  // Rebase the branch. The editor closes when it's done.
  Q_INVOKABLE bool start();
  // Select the commit of a row in the graph.
  Q_INVOKABLE void select(int row);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

signals:
  void activeChanged();
  void changed();

private:
  struct Item {
    git::Commit commit;
    Action action = Pick;
    QString message;
  };

  bool canSquash(int row) const;
  void load();
  void setActive(bool active);

  RepoView *mView;
  bool mActive = false;
  QString mBranch;
  QString mBranchName;
  git::Commit mTip;
  git::Commit mOnto;
  QString mOntoName;
  QList<Item> mItems;
};

#endif
