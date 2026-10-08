//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef COMMITLIST_H
#define COMMITLIST_H

#include "git/Id.h"
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QMap>
#include <QObject>
#include <QVariantList>

class Index;
class RepoView;

namespace git {
class Commit;
class Diff;
class Reference;
} // namespace git

// Controller for the commit graph. It owns the commit models and the
// selection; qrc:/qml/GraphView.qml draws them.
class CommitList : public QObject {
  Q_OBJECT

  Q_PROPERTY(QAbstractItemModel *model READ model NOTIFY modelChanged)
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(int selectionRevision READ selectionRevision NOTIFY
                 selectionRevisionChanged)
  Q_PROPERTY(int laneCount READ laneCount NOTIFY laneCountChanged)
  Q_PROPERTY(bool showAuthor READ showAuthor NOTIFY settingsChanged)
  Q_PROPERTY(bool showDate READ showDate NOTIFY settingsChanged)
  Q_PROPERTY(bool showId READ showId NOTIFY settingsChanged)
  Q_PROPERTY(bool compact READ compact NOTIFY settingsChanged)
  Q_PROPERTY(QString refsFilterName READ refsFilterName NOTIFY settingsChanged)
  Q_PROPERTY(QString sortName READ sortName NOTIFY settingsChanged)
  Q_PROPERTY(bool filtered READ isFiltered NOTIFY modelChanged)
  // The branches that are shown alone in the graph, like 'solo' in
  // GitKraken, by their qualified names.
  Q_PROPERTY(QStringList solo READ solo NOTIFY soloChanged)
  Q_PROPERTY(QString soloText READ soloText NOTIFY soloChanged)
  // The branches that aren't shown in the graph, like 'hide' in GitKraken,
  // by their qualified names. Names that end with a slash hide a remote.
  Q_PROPERTY(QStringList hidden READ hidden NOTIFY hiddenChanged)
  Q_PROPERTY(QString hiddenText READ hiddenText NOTIFY hiddenChanged)

public:
  enum Role {
    DiffRole = Qt::UserRole,
    CommitRole,
    GraphRole,
    GraphColorRole,

    // roles used by QML
    StatusRole,
    SummaryRole,
    AuthorRole,
    InitialsRole,
    DateRole,
    ShortIdRole,
    StarredRole,
    RefsRole,
    NodeColorRole,
    MergeRole,
    BusyRole,
    WipRole
  };

  enum class RefsFilter {
    AllRefs,
    SelectedRef,
    SelectedRefIgnoreMerge,
  };

  CommitList(Index *index, RepoView *view);

  // Role names shared by the commit models.
  static QHash<int, QByteArray> roleNames();

  QAbstractItemModel *model() const { return mCurrent; }
  QItemSelectionModel *selectionModel() const { return mSelection; }

  // Get the status diff item.
  git::Diff status() const;

  // Get the current selection.
  QString selectedRange() const;
  git::Diff selectedDiff() const;
  QList<git::Commit> selectedCommits() const;
  QModelIndexList selectedIndexes() const;

  // Cancel background status diff.
  Q_INVOKABLE void cancelStatus();

  void setReference(const git::Reference &ref);
  void setFilter(const QString &filter);
  void setPathspec(const QString &pathspec, bool index = false);
  void setCommits(const QList<git::Commit> &commits);

  void selectReference(const git::Reference &ref);
  void resetSelection(bool spontaneous = false);
  void selectFirstCommit(bool spontaneous = false);
  Q_INVOKABLE void selectCommitRelative(int offset);
  void selectRow(int row);
  bool selectRange(const QString &range, const QString &file = QString(),
                   bool spontaneous = false);
  void suppressResetWalker(bool suppress);
  bool isResetWalkerSuppressed();

  void resetSettings();
  void resetReference(const git::Reference &ref);

  // Whether a status check and/or walker/row rebuild is currently in
  // flight. See the loadingChanged() signal for a way to wait on this
  // instead of polling it.
  bool isLoading() const { return mLoading; }

  int selectionRevision() const { return mSelectionRevision; }
  int laneCount() const { return mLaneCount; }
  bool showAuthor() const;
  bool showDate() const;
  bool showId() const;
  bool compact() const;
  QString refsFilterName() const;
  QString sortName() const;
  bool isFiltered() const { return mCurrent != mModel; }

  // QML interface
  Q_INVOKABLE bool isSelected(int row) const;
  Q_INVOKABLE void click(int row, int modifiers);
  Q_INVOKABLE void toggleStar(int row);
  Q_INVOKABLE void fetchMore();
  Q_INVOKABLE void showContextMenu(int row, qreal x, qreal y);
  Q_INVOKABLE void showRefsFilterMenu(qreal x, qreal y);
  Q_INVOKABLE void showSortMenu(qreal x, qreal y);
  Q_INVOKABLE void showSettingsMenu(qreal x, qreal y);

  QStringList solo() const { return mSolo; }
  // The soloed branch, or how many branches are soloed.
  QString soloText() const;
  bool isSoloed(const QString &name) const;
  Q_INVOKABLE void setSoloed(const QString &name, bool soloed);
  Q_INVOKABLE void unsoloAll();

  QStringList hidden() const { return mHidden; }
  // The hidden branch or remote, or how many are hidden.
  QString hiddenText() const;
  // Whether the branch is hidden, also when its remote is.
  bool isHidden(const QString &name) const;
  // The checked out branch can't be hidden.
  bool canHide(const QString &name) const;
  Q_INVOKABLE void setHidden(const QString &name, bool hidden);
  Q_INVOKABLE void showAll();

signals:
  void statusChanged(bool dirty);
  void diffSelected(const git::Diff diff, const QString &file = QString(),
                    bool spontaneous = false);

  // Emitted just before a (potentially slow) diff is being computation. This
  // can be used to clear GUI and enable loading indicators whilst waiting
  void diffLoading();

  // Emitted whenever isLoading() changes.
  void loadingChanged(bool loading);

  void modelChanged();
  void selectionRevisionChanged();
  void laneCountChanged();
  void settingsChanged();
  void soloChanged();
  void hiddenChanged();

  // Ask the view to scroll so that the row is visible.
  void scrollRequested(int row);

private:
  void setModel(QAbstractItemModel *model);
  void storeSelection();
  void restoreSelection();
  void updateModel();
  void updateRefs();
  void updateLaneCount();
  void setLoading(bool loading);
  void setConfigValue(const QString &key, const QVariant &value);
  void setSolo(const QStringList &solo);
  void setHidden(const QStringList &hidden);

  QModelIndexList sortedIndexes() const;

  QModelIndex findCommit(const git::Commit &commit);
  void selectIndexes(const QItemSelection &selection,
                     const QString &file = QString(), bool spontaneous = false);

  void notifySelectionChanged();
  void dispatchSelectedDiff(const QString &file, bool spontaneous);

  RepoView *mView;
  Index *mIndex;
  QString mFilter;

  QString mFile;
  bool mSpontaneous = true;

  QAbstractListModel *mList;
  QAbstractListModel *mModel;
  QAbstractItemModel *mCurrent = nullptr;
  QItemSelectionModel *mSelection = nullptr;

  // Reference badges for each commit.
  QMap<git::Id, QVariantList> mRefs;

  QStringList mSolo;
  QStringList mHidden;

  bool mRestoreSelection{true};

  QString mSelectedRange;

  // Whether the current selection is just the automatic fallback rather
  // than a deliberate user pick
  bool mSelectionIsDefault{false};

  // Whether the loading indicator should be shown
  bool mLoading{false};

  int mSelectionRevision = 0;
  int mLaneCount = 0;

  // Incremented on every selection-driven diff request. This is a hack used to
  // discard diffs that arrive before the last one
  // (Yes, we should have a proper cancel pathway here)
  int mDiffRequest = 0;
};

#endif
