//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef REFSPANEL_H
#define REFSPANEL_H

#include "git/Reference.h"
#include "git/Repository.h"
#include <QAbstractListModel>
#include <QSet>

class PullRequestList;
class RepoView;

// Flat list of the references of a repository, grouped into collapsible
// sections the way GitKraken's left panel shows them.
class RefsModel : public QAbstractListModel {
  Q_OBJECT

public:
  // Keep in sync with RefsPanel.qml.
  enum Kind {
    Header,
    Branch,
    RemoteGroup,
    RemoteBranch,
    Tag,
    Stash,
    Submodule,
    Empty,
    PullRequest
  };

  enum Section { Local, Remote, Tags, Stashes, Submodules, PullRequests };

  enum Role {
    KindRole = Qt::UserRole,
    SectionRole,
    NameRole,
    DepthRole,
    HeadRole,
    CurrentRole,
    AheadRole,
    BehindRole,
    CountRole,
    ExpandedRole,
    ExpandableRole,
    // The branch is soloed in the graph.
    SoloRole,
    // The qualified name of a reference, or "remote:<name>" for a remote,
    // for dragging and dropping.
    RefNameRole,
    // The branch or remote is hidden in the graph.
    HiddenRole,
    // The number of a pull request.
    NumberRole
  };

  struct Item {
    Kind kind;
    Section section;
    QString name;
    int depth = 0;
    QString refName; // qualified, see ref()
    int index = -1; // stash index, or pull request number
    QString tip;
    int count = 0;
    bool head = false;
    int ahead = 0;
    int behind = 0;
    QString key; // collapse state key
  };

  RefsModel(const git::Repository &repo, QObject *parent = nullptr);

  const Item &item(int row) const { return mItems.at(row); }
  // The reference of an item as it is now, invalid if it's gone.
  git::Reference ref(const Item &item) const;
  git::Reference current() const { return mCurrent; }
  void setCurrent(const git::Reference &ref);

  void setFilter(const QString &filter);
  void toggle(int row);
  // Read the references again. They're read on another thread: there are
  // thousands in some repositories. The rows change when they're there.
  void update();

  // The qualified names of the soloed branches.
  void setSolo(const QStringList &solo);
  // The qualified names of the hidden branches, and remotes with a slash.
  void setHidden(const QStringList &hidden);

  // The open pull requests, when the host of the repository is supported.
  void setPullRequests(PullRequestList *pullRequests);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  // The items of the sections that update() reads, before filtering.
  struct Snapshot {
    QList<Item> local;
    QMap<QString, QList<Item>> remotes;
    QList<Item> tags;
    QList<Item> stashes;
    QList<Item> submodules;
  };

  static Snapshot snapshot(const git::Repository &repo);
  // Lay out the rows of the snapshot.
  void assemble();

  bool isExpanded(const QString &key) const;
  bool matches(const QString &name) const;
  void addHeader(Section section, const QString &name, int count);

  git::Repository mRepo;
  git::Reference mCurrent;
  QString mFilter;
  QSet<QString> mCollapsed;
  Snapshot mSnapshot;
  bool mLoaded = false;
  int mUpdates = 0;
  QList<Item> mItems;
  QStringList mSolo;
  QStringList mHidden;
  PullRequestList *mPullRequests = nullptr;
};

// Controller for the references panel. It replaces the reference drop-down
// above the commit list: the current reference drives the commit graph.
class RefsPanel : public QObject {
  Q_OBJECT

  Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)
  // Branches are soloed, and the others are hidden in the graph.
  Q_PROPERTY(bool soloActive READ isSoloActive NOTIFY soloChanged)

public:
  RefsPanel(const git::Repository &repo, RepoView *view);

  QAbstractItemModel *model() const { return mModel; }

  git::Reference currentReference() const;
  void select(const git::Reference &ref, bool suppress = false);

  Q_INVOKABLE void activate(int row);
  Q_INVOKABLE void open(int row);
  Q_INVOKABLE void toggle(int row);
  Q_INVOKABLE void setFilter(const QString &filter);
  Q_INVOKABLE void showContextMenu(int row, qreal x, qreal y);
  // The + button of a section header.
  Q_INVOKABLE void add(int section);

  bool isSoloActive() const { return mSoloActive; }
  void setSolo(const QStringList &solo);
  // Solo or unsolo the branch of a row.
  Q_INVOKABLE void toggleSolo(int row);
  // Hide or show the branch or remote of a row.
  Q_INVOKABLE void toggleHidden(int row);

signals:
  // The reference that the commit graph should show changed.
  void referenceChanged(const git::Reference &ref);
  // A reference was clicked; select its commit.
  void referenceSelected(const git::Reference &ref);
  // A stash was clicked; select it in the list of stashes.
  void stashSelected(int index);

  void soloChanged();

private:
  void setCurrent(const git::Reference &ref);
  void showMenu(RefsModel::Item item, qreal x, qreal y);
  void toggleHidden(const RefsModel::Item &item);

  RepoView *mView;
  git::Repository mRepo;
  RefsModel *mModel;
  bool mSoloActive = false;
  git::Reference mEmitted;
};

#endif
