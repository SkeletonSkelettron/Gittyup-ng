//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CommitList.h"
#include "InteractiveRebase.h"
#include "qml/QmlSupport.h"
#include "ConfigKeys.h"
#include "Debug.h"
#include "RepoView.h"
#include "app/Application.h"
#include "conf/Settings.h"
#include "dialogs/MergeDialog.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "git/Patch.h"
#include "git/RevWalk.h"
#include "git/Signature.h"
#include "git/TagRef.h"
#include "git/Tree.h"
#include "index/Index.h"
#include "ui/HotkeyManager.h"
#include <QAbstractListModel>
#include <QMenu>
#include <QShortcut>
#include <QtConcurrent>

namespace {

// The branches that are soloed, separated by spaces.
const QString kSoloKey = "solo.refs";
// The branches that are hidden, separated by spaces. Names that end with a
// slash hide the branches of a remote.
const QString kHiddenKey = "hide.refs";

// Whether 'name' is one of the hidden branches or on a hidden remote.
bool matchesHidden(const QStringList &hidden, const QString &name) {
  for (const QString &entry : hidden) {
    if (entry == name || (entry.endsWith('/') && name.startsWith(entry)))
      return true;
  }
  return false;
}

// FIXME: Factor out into theme?
const QColor kTaintedColor = Qt::gray;

const QString kPathspecFmt = "pathspec:%1";

// Use fixed short id size in compact mode.
// FIXME: Use 'core.abbrev' config instead?
const int kShortIdSize = 7;

enum GraphSegment {
  Dot,
  Top,
  Middle,
  Bottom,
  Cross,
  LeftIn,
  LeftOut,
  RightIn,
  RightOut
};

class DiffCallbacks : public git::Diff::Callbacks {
public:
  void setCanceled(bool canceled) { mCanceled = canceled; }

  bool progress(const QString &oldPath, const QString &newPath) override {
    return !mCanceled;
  }

private:
  bool mCanceled = false;
};

// Data for the QML roles of a commit row.
QVariant commitData(const git::Commit &commit, int role,
                    const QMap<git::Id, QVariantList> *refs) {
  switch (role) {
    case CommitList::Role::SummaryRole:
      return commit.summary(git::Commit::SubstituteEmoji);

    case CommitList::Role::AuthorRole:
      return commit.author().name();

    case CommitList::Role::InitialsRole: {
      QStringList parts =
          commit.author().name().split(' ', Qt::SkipEmptyParts);
      QString initials;
      if (!parts.isEmpty())
        initials += parts.first().left(1);
      if (parts.size() > 1)
        initials += parts.last().left(1);
      return initials.toUpper();
    }

    case CommitList::Role::DateRole: {
      QDateTime date = commit.committer().date().toLocalTime();
      return (date.date() == QDate::currentDate())
                 ? QLocale().toString(date.time(), QLocale::ShortFormat)
                 : QLocale().toString(date.date(), QLocale::ShortFormat);
    }

    case CommitList::Role::ShortIdRole:
      return commit.id().toString().left(kShortIdSize);

    case CommitList::Role::StarredRole:
      return commit.isStarred();

    case CommitList::Role::RefsRole:
      return refs ? refs->value(commit.id()) : QVariantList();

    case CommitList::Role::MergeRole:
      return commit.isMerge();

    default:
      return QVariant();
  }
}

/*!
 * \brief The CommitModel class
 * Model showing all commits as timeline
 */
class CommitModel : public QAbstractListModel {
  Q_OBJECT

public:
  CommitModel(const git::Repository &repo,
              const QMap<git::Id, QVariantList> *refs,
              QObject *parent = nullptr)
      : QAbstractListModel(parent), mRepo(repo), mRefs(refs) {
    // Connect progress timer.
    connect(&mTimer, &QTimer::timeout, [this] {
      ++mProgress;
      QModelIndex idx = index(0, 0);
      emit dataChanged(idx, idx, {Qt::DisplayRole, CommitList::BusyRole});
    });

    // Connect watcher to signal when the status diff finishes.
    connect(&mStatus, &QFutureWatcher<git::Diff>::finished, [this] {
      mTimer.stop();
      dispatchResetWalker(true);
    });

    // Apply the result of an asynchronous walker reset on the GUI thread.
    connect(&mReset, &QFutureWatcher<ResetResult>::finished, [this] {
      ResetResult result = mReset.result();
      bool emitStatusFinished = result.emitStatusFinished;
      applyResetResult(std::move(result));
      if (emitStatusFinished)
        emit statusFinished(!mRows.isEmpty() &&
                            !mRows.first().commit.isValid());
    });

    resetSettings();
  }

  ~CommitModel() {
    // Ensure that mStatus is stopped since it captures `this` and potentially
    // might crash after the destructor is finished
    cancelStatus();

    // ..and the same applies to mReset too
    if (mReset.isRunning())
      mReset.waitForFinished();
  }

  git::Reference reference() const { return mRef; }

  git::Diff status() const {
    if (!mStatus.isFinished())
      return git::Diff();

    QFuture<git::Diff> future = mStatus.future();
    if (!future.resultCount())
      return git::Diff();

    return future.result();
  }

  void startStatus() {
    // Cancel existing status diff.
    cancelStatus();

    // Reload the index before starting the status thread. Allowing
    // it to reload on the thread frequently corrupts the index.
    git::Index index = mRepo.index();
    index.read();

    // The thread reads the index of its own: checking out, staging and other
    // changes of the index can't wait for it.
    git::Index own = index.reopen();
    if (!own.isValid())
      own = index;

    // Check for uncommitted changes asynchronously.
    emit loadingChanged(true);
    mProgress = 0;
    mTimer.start(50);
    mStatus.setFuture(QtConcurrent::run([this, index, own] {
      bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();
      git::Diff diff = mRepo.status(own, &mStatusCallbacks, ignoreWhitespace);

      // Stage with the index of the repository, which isn't read here.
      if (diff.isValid())
        diff.setIndex(index);
      return diff;
    }));
  }

  void cancelStatus() {
    if (!mStatus.isRunning())
      return;

    mStatusCallbacks.setCanceled(true);
    mStatus.waitForFinished();
    mStatus.setFuture(QFuture<git::Diff>());
    mStatusCallbacks.setCanceled(false);
  }

  void setPathspec(const QString &pathspec) {
    if (mPathspec == pathspec)
      return;

    mPathspec = pathspec;
    resetWalker();
  }

  // Walk only these branches and their upstream branches.
  void setSolo(const QStringList &solo) {
    if (solo == mSolo)
      return;

    mSolo = solo;
    resetWalker();
  }

  // Don't walk these branches, unless they are soloed.
  void setHidden(const QStringList &hidden) {
    if (hidden == mHidden)
      return;

    mHidden = hidden;
    resetWalker();
  }

  void suppressResetWalker(bool suppress) { mSuppressResetWalker = suppress; }

  bool isResetWalkerSuppressed() { return mSuppressResetWalker; }

  void setReference(const git::Reference &ref) {
    mRef = ref;
    if (!mSuppressResetWalker) {
      resetWalker();
    }
  }

  void resetReference(const git::Reference &ref) {
    // Reset selected ref to updated ref.
    if (ref.isValid() && mRef.isValid() &&
        ref.qualifiedName() == mRef.qualifiedName())
      mRef = ref;

    // Status is invalid after HEAD changes.
    if (!ref.isValid() || ref.isHead())
      startStatus();
    else if (!mSuppressResetWalker) {
      // reset walker will be done when status finished
      resetWalker();
    }
  }

  // Rebuild the walker and the first page of rows. The expensive part
  // (building the revwalk over all refs and computing the graph for the
  // first page of commits) runs on a background thread; see
  // dispatchResetWalker().
  void resetWalker() { dispatchResetWalker(false); }

  void resetSettings(bool walk = false) {
    git::Config config = mRepo.appConfig();
    mRefsFilter = static_cast<CommitList::RefsFilter>(config.value<int>(
        ConfigKeys::kRefsKey, (int)CommitList::RefsFilter::AllRefs));
    mSortDate = config.value<bool>(ConfigKeys::kSortKey, true);
    mShowCleanStatus = config.value<bool>(ConfigKeys::kStatusKey, true);
    mGraphVisible = config.value<bool>(ConfigKeys::kGraphKey, true);

    if (walk)
      resetWalker();
  }

  bool canFetchMore(const QModelIndex &parent) const {
    return mWalker.isValid();
  }

  void fetchMore(const QModelIndex &parent) {
    FetchResult fetched =
        fetchRows(mWalker, mParents, mRows, mPathspec, mGraphVisible,
                  mSolo.isEmpty() ? mRefsFilter
                                  : CommitList::RefsFilter::AllRefs);

    // Update the model.
    if (!fetched.rows.isEmpty()) {
      int first = mRows.size();
      int last = first + fetched.rows.size() - 1;
      beginInsertRows(QModelIndex(), first, last);
      mRows.append(fetched.rows);
      endInsertRows();
    }

    // Invalidate walker.
    if (fetched.exhausted)
      mWalker = git::RevWalk();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const {
    return mRows.size();
  }

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
    if (index.row() >= mRows.size())
      return QVariant();
    const Row &row = mRows.at(index.row());
    bool status = !row.commit.isValid();
    switch (role) {
      case Qt::DisplayRole:
        if (!status)
          return QVariant();

        return mStatus.isFinished() ? tr("Uncommitted changes")
                                    : tr("Checking for uncommitted changes");

      case Qt::TextAlignmentRole:
        if (!status)
          return QVariant();

        return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);

      case Qt::DecorationRole:
        if (!status)
          return QVariant();

        return mStatus.isFinished() ? QVariant() : mProgress;

      case CommitList::Role::DiffRole: {
        if (status)
          return QVariant::fromValue(this->status());

        bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();
        git::Diff diff = row.commit.diff(git::Commit(), -1, ignoreWhitespace);
        diff.findSimilar();
        return QVariant::fromValue(diff);
      }

      case CommitList::Role::CommitRole:
        return status ? QVariant() : QVariant::fromValue(row.commit);

      case CommitList::Role::GraphRole: {
        QVariantList columns;
        for (const Column &column : row.columns) {
          QVariantList segments;
          for (const Segment &segment : column)
            segments.append(segment.segment);
          columns.append(QVariant(segments));
        }

        return columns;
      }

      case CommitList::Role::GraphColorRole: {
        QVariantList columns;
        for (const Column &column : row.columns) {
          QVariantList segments;
          for (const Segment &segment : column)
            segments.append(segment.color);
          columns.append(QVariant(segments));
        }

        return columns;
      }

      case CommitList::Role::StatusRole:
        return status;

      case CommitList::Role::BusyRole:
        return status && !mStatus.isFinished();

      case CommitList::Role::WipRole: {
        QVariantMap counts;
        git::Diff diff = status ? this->status() : git::Diff();
        if (!diff.isValid())
          return counts;

        int added = 0, modified = 0, deleted = 0;
        for (int i = 0; i < diff.count(); ++i) {
          switch (diff.status(i)) {
            case GIT_DELTA_ADDED:
            case GIT_DELTA_UNTRACKED:
              ++added;
              break;
            case GIT_DELTA_DELETED:
              ++deleted;
              break;
            default:
              ++modified;
              break;
          }
        }

        counts.insert("added", added);
        counts.insert("modified", modified);
        counts.insert("deleted", deleted);
        return counts;
      }

      case CommitList::Role::NodeColorRole: {
        for (const Column &column : row.columns) {
          for (const Segment &segment : column) {
            if (segment.segment == Dot && segment.color.isValid())
              return segment.color;
          }
        }

        return QVariant();
      }

      default:
        return status ? QVariant() : commitData(row.commit, role, mRefs);
    }
  }

  QHash<int, QByteArray> roleNames() const override {
    return CommitList::roleNames();
  }

  // The widest graph row that has been loaded.
  int laneCount() const {
    int count = 0;
    for (const Row &row : mRows)
      count = qMax(count, static_cast<int>(row.columns.size()));
    return count;
  }

signals:
  void statusFinished(bool visible);
  void loadingChanged(bool loading);

private:
  struct Parent {
    Parent(const git::Commit &commit, const QColor &color, bool tainted = false)
        : commit(commit), color(color), tainted(tainted) {}

    QColor taintedColor(const git::Commit &commit = git::Commit()) const {
      return (tainted && this->commit != commit) ? kTaintedColor : color;
    }

    git::Commit commit;
    QColor color;
    bool tainted;
  };

  struct Segment {
    Segment(GraphSegment segment, QColor color)
        : segment(segment), color(color) {}

    GraphSegment segment;
    QColor color;
  };

  using Column = QList<Segment>;

  struct Row {
    Row(const git::Commit &commit, const QVector<Column> &columns)
        : commit(commit), columns(columns) {}

    git::Commit commit;
    QVector<Column> columns;
  };

  // Everything the background thread needs to rebuild the walker and the
  // first page of rows. Captured by value at dispatch time so the
  // computation can run on another thread without touching model state.
  struct ResetContext {
    git::Reference ref;
    QString pathspec;
    bool graphVisible;
    bool sortDate;
    CommitList::RefsFilter refsFilter;
    QStringList solo;
    QStringList hidden;
    bool showCleanStatus;
    git::Repository repo;
    git::Diff statusDiff;
    bool statusCheckFinished;

    // Carried straight through to ResetResult; see its field for why.
    bool emitStatusFinished;
  };

  struct ResetResult {
    QList<Parent> parents;
    QList<Row> rows;
    git::RevWalk walker;

    // Whether this particular reset was triggered by the status check
    // finishing, and should therefore emit statusFinished() once applied.
    bool emitStatusFinished = false;
  };

  struct FetchResult {
    QList<Row> rows;
    bool exhausted = false;
  };

  static int indexOf(const QList<Parent> &parents, const git::Commit &commit) {
    int count = parents.size();
    for (int i = 0; i < count; ++i) {
      if (parents.at(i).commit == commit)
        return i;
    }

    return -1;
  }

  static bool contains(const git::Commit &commit,
                       const QList<Row> &existingRows,
                       const QList<Row> &newRows) {
    for (const Row &row : existingRows) {
      if (row.commit == commit)
        return true;
    }

    for (const Row &row : newRows) {
      if (row.commit == commit)
        return true;
    }

    return false;
  }

  // The commit and parents parameters represent the current row.
  // The nextParents parameter represents the next row after this one.
  static QVector<Column> columns(const git::Commit &commit,
                                 const QList<Parent> &parents,
                                 const QList<Parent> &nextParents, bool root) {
    int count = parents.size();
    QVector<Column> columns(count);

    // Add incoming paths.
    int incoming = root ? count - 1 : count;
    for (int i = 0; i < incoming; ++i)
      columns[i] << Segment(Top, parents.at(i).taintedColor());

    // Add outgoing paths.
    for (int i = 0; i < count; ++i) {
      // Get the successors of this column.
      QList<git::Commit> successors;
      const Parent &parent = parents.at(i);
      if (parent.commit == commit) {
        successors = parent.commit.parents();
      } else {
        successors.append(parent.commit);
      }

      // Add a path to each successor.
      for (const git::Commit &successor : successors) {
        // Find index of parent in next row.
        int index = indexOf(nextParents, successor);
        if (index < 0)
          continue;

        // Handle multiple commits that share the same parent.
        bool single = (successors.size() == 1);
        const QColor &color =
            single ? parent.taintedColor(commit) : nextParents.at(index).color;

        if (index < i) {
          // out to the left
          columns[index] << Segment(RightIn, color);
          for (int j = index + 1; j < i; ++j)
            columns[j] << Segment(Cross, color);
          columns[i] << Segment(LeftOut, color);

        } else if (index > i) {
          // out to the right
          columns[i] << Segment(RightOut, color);
          for (int j = i + 1; j < index; ++j)
            columns[j] << Segment(Cross, color);
          if (index == columns.size())
            columns.append(Column());
          columns[index] << Segment(LeftIn, color);

        } else { // index == i
          // out the bottom
          columns[index] << Segment(Bottom, color);
        }
      }
    }

    // Add middle section last.
    for (int i = 0; i < count; ++i) {
      const Parent &parent = parents.at(i);
      // The node belongs to the commit, so it always gets the lane color.
      bool dot = (parent.commit == commit);
      columns[i] << Segment(dot ? Dot : Middle,
                            dot ? parent.color : parent.taintedColor());
    }

    return columns;
  }

  static QColor nextColor(const QList<Parent> &parents) {
    // Get the first unused (or least used) color.
    QMap<QString, int> counts;
    for (const Parent &parent : parents)
      counts[parent.color.name()]++;

    int count = 0;
    QList<QColor> colors = Application::theme()->branchTopologyEdges();
    forever {
      for (const QColor &color : colors) {
        if (counts.value(color.name()) == count)
          return color;
      }

      ++count;
    }

    Q_UNREACHABLE();
    return QColor();
  }

  // Walk at most one page of commits, updating parents in place and
  // returning the new rows. Operates purely on its arguments (no access to
  // 'this' state) so it can run on a background thread as well as
  // synchronously from fetchMore().
  static FetchResult fetchRows(git::RevWalk &walker, QList<Parent> &parents,
                               const QList<Row> &existingRows,
                               const QString &pathspec, bool graphVisible,
                               CommitList::RefsFilter refsFilter) {
    FetchResult result;
    int i = 0;
    git::Commit commit = walker.next(pathspec);
    while (commit.isValid()) {
      // Add root commits.
      bool root = false;
      if (indexOf(parents, commit) < 0) {
        root = true;
        parents.append(Parent(commit, nextColor(parents)));
      }

      // Calculate graph columns.
      // Remember current row.
      QList<Parent> rowParents = parents;

      // Replace commit with its parents.
      QList<git::Commit> replacements;
      for (const git::Commit &parent : commit.parents()) {
        // FIXME: Mark commits that point to existing parent?
        if (indexOf(parents, parent) < 0 &&
            !contains(parent, existingRows, result.rows))
          replacements.append(parent);
        if (refsFilter == CommitList::RefsFilter::SelectedRefIgnoreMerge) {
          break;
        }
      }

      // Set parents for next row.
      int index = indexOf(parents, commit);
      if (index >= 0) {
        Parent parent = parents.takeAt(index);
        if (!replacements.isEmpty()) {
          git::Commit replacement = replacements.takeFirst();
          parents.insert(index, Parent(replacement, parent.color));
          for (const git::Commit &replacement : replacements)
            parents.append(Parent(replacement, nextColor(parents)));
        }
      }

      // Add graph row.
      QVector<Column> row;
      if (graphVisible && pathspec.isEmpty())
        row = columns(commit, rowParents, parents, root);

      result.rows.append(Row(commit, row));
      DebugRefresh("Append commit: " << commit.shortId());

      // Bail out.
      if (i++ >= 64)
        break;

      commit = walker.next(pathspec);
    }

    result.exhausted = !commit.isValid();
    return result;
  }

  // Build the walker and the first page of rows. Safe to run off the GUI
  // thread: it only touches the context passed in and returns a fresh
  // result rather than mutating model state directly.
  static ResetResult computeReset(const ResetContext &ctx) {
    ResetResult result;

    // Soloed branches are shown alone, with their upstream branches.
    QList<git::Reference> solo;
    for (const QString &name : ctx.solo) {
      git::Reference ref = ctx.repo.lookupRef(name);
      if (!ref.isValid())
        continue;

      solo.append(ref);
      if (ref.isLocalBranch()) {
        if (git::Branch upstream = git::Branch(ref).upstream())
          solo.append(upstream);
      }
    }

    // The uncommitted changes are shown on top of HEAD, when its branch is
    // soloed.
    git::Reference statusRef = ctx.ref;
    bool head = (!ctx.ref.isValid() || ctx.ref.isHead());
    if (!solo.isEmpty()) {
      git::Reference repoHead = ctx.repo.head();
      statusRef = git::Reference();
      head = false;
      for (const git::Reference &ref : solo) {
        if (repoHead.isValid() &&
            ref.qualifiedName() == repoHead.qualifiedName()) {
          statusRef = repoHead;
          head = true;
        }
      }
    }

    // Update status row.
    bool valid = (!ctx.statusCheckFinished || ctx.statusDiff.isValid());
    if (ctx.showCleanStatus && head && valid && ctx.pathspec.isEmpty()) {
      QVector<Column> row;
      if (ctx.graphVisible && statusRef.isValid() && ctx.statusCheckFinished) {
        row.append({Segment(Bottom, kTaintedColor), Segment(Dot, QColor())});
        result.parents.append(
            Parent(statusRef.target(), nextColor(result.parents), true));
      }
      result.rows.append(Row(git::Commit(), row)); // Uncommitted changes
    }

    int sort = GIT_SORT_NONE;
    if (ctx.graphVisible) {
      sort |= GIT_SORT_TOPOLOGICAL;
      if (ctx.sortDate)
        sort |= GIT_SORT_TIME;
    } else if (!ctx.sortDate) {
      sort |= GIT_SORT_TOPOLOGICAL;
    }

    // Begin walking commits.
    if (!solo.isEmpty()) {
      result.walker = solo.first().walker(sort);
      for (int i = 1; i < solo.size(); ++i)
        result.walker.push(solo.at(i));

      if (head) {
        // Add merge head.
        if (git::Reference mergeHead = ctx.repo.lookupRef("MERGE_HEAD"))
          result.walker.push(mergeHead);
      }

    } else if (ctx.ref.isValid()) {
      result.walker = ctx.ref.walker(
          sort,
          ctx.refsFilter == CommitList::RefsFilter::SelectedRefIgnoreMerge);
      if (ctx.ref.isLocalBranch()) {
        // Add the upstream branch.
        git::Branch upstream = git::Branch(ctx.ref).upstream();
        if (upstream.isValid() &&
            !matchesHidden(ctx.hidden, upstream.qualifiedName()))
          result.walker.push(upstream);
      }

      if (ctx.ref.isHead()) {
        // Add merge head.
        if (git::Reference mergeHead = ctx.repo.lookupRef("MERGE_HEAD"))
          result.walker.push(mergeHead);
      }

      // Hidden branches aren't walked, like in GitKraken.
      if (ctx.refsFilter == CommitList::RefsFilter::AllRefs) {
        for (const git::Reference &ref : ctx.repo.refs()) {
          if (!ref.isStash() && !matchesHidden(ctx.hidden, ref.qualifiedName()))
            result.walker.push(ref);
        }
      }
    }

    if (result.walker.isValid()) {
      // Soloed branches show all their parents.
      FetchResult fetched = fetchRows(
          result.walker, result.parents, result.rows, ctx.pathspec,
          ctx.graphVisible,
          solo.isEmpty() ? ctx.refsFilter : CommitList::RefsFilter::AllRefs);
      result.rows.append(fetched.rows);
      if (fetched.exhausted)
        result.walker = git::RevWalk();
    }

    result.emitStatusFinished = ctx.emitStatusFinished;
    return result;
  }

  // Kick off an asynchronous walker reset. The GUI thread keeps showing the
  // previous rows (behind a loading indicator, see CommitList::setLoading)
  // until the background computation finishes and applyResetResult() swaps
  // the new data in.
  void dispatchResetWalker(bool emitStatusFinishedAfter) {
    ResetContext ctx{mRef,
                     mPathspec,
                     mGraphVisible,
                     mSortDate,
                     mRefsFilter,
                     mSolo,
                     mHidden,
                     mShowCleanStatus,
                     mRepo,
                     status(),
                     mStatus.isFinished(),
                     emitStatusFinishedAfter};

    emit loadingChanged(true);
    mReset.setFuture(QtConcurrent::run([ctx] { return computeReset(ctx); }));
  }

  // Apply a completed background reset on the GUI thread.
  void applyResetResult(ResetResult &&result) {
    beginResetModel();
    mParents = std::move(result.parents);
    mRows = std::move(result.rows);
    mWalker = std::move(result.walker);
    DebugRefresh("");
    endResetModel();
    emit loadingChanged(false);
  }

  QTimer mTimer;
  int mProgress = 0;

  DiffCallbacks mStatusCallbacks;
  QFutureWatcher<git::Diff> mStatus;

  QFutureWatcher<ResetResult> mReset;

  QString mPathspec;
  git::Reference mRef;
  git::RevWalk mWalker;
  git::Repository mRepo;
  const QMap<git::Id, QVariantList> *mRefs;

  QList<Row> mRows;
  QList<Parent> mParents;

  // walker settings
  bool mSuppressResetWalker{false};
  CommitList::RefsFilter mRefsFilter{CommitList::RefsFilter::AllRefs};
  QStringList mSolo;
  QStringList mHidden;
  bool mSortDate = true;
  bool mShowCleanStatus = true;
  bool mGraphVisible = true;
};

/*!
 * \brief The ListModel class
 * Used to show a list of commits. This is used when a filter is used
 */
class ListModel : public QAbstractListModel {
public:
  ListModel(const QMap<git::Id, QVariantList> *refs, QObject *parent = nullptr)
      : QAbstractListModel(parent), mRefs(refs) {}

  void setList(const QList<git::Commit> &commits) {
    beginResetModel();
    mCommits = commits;
    endResetModel();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    return mCommits.size();
  }

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    switch (role) {
      case CommitList::Role::DiffRole: {
        git::Commit commit = mCommits.at(index.row());
        bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();
        git::Diff diff = commit.diff(git::Commit(), -1, ignoreWhitespace);
        diff.findSimilar();
        return QVariant::fromValue(diff);
      }

      case CommitList::Role::CommitRole:
        return QVariant::fromValue(mCommits.at(index.row()));

      case CommitList::Role::StatusRole:
      case CommitList::Role::BusyRole:
        return false;

      default:
        return commitData(mCommits.at(index.row()), role, mRefs);
    }
  }

  QHash<int, QByteArray> roleNames() const override {
    return CommitList::roleNames();
  }

private:
  QList<git::Commit> mCommits;
  const QMap<git::Id, QVariantList> *mRefs;
};

class SelectionModel : public QItemSelectionModel {
public:
  SelectionModel(QAbstractItemModel *model) : QItemSelectionModel(model) {}

  void select(const QItemSelection &selection,
              QItemSelectionModel::SelectionFlags command) {
    if ((command == QItemSelectionModel::Select ||
         command == QItemSelectionModel::SelectCurrent ||
         command == (QItemSelectionModel::Current |
                     QItemSelectionModel::ClearAndSelect)) &&
        (selectedIndexes().size() >= 2 || selection.indexes().size() > 1))
      return;

    QItemSelectionModel::select(selection, command);
  }
};

} // namespace

static Hotkey selectCommitDownHotKey = HotkeyManager::registerHotkey(
    "j", "commitList/selectCommitDown", "CommitList/Select Next Commit Down");

static Hotkey selectCommitUpHotKey = HotkeyManager::registerHotkey(
    "k", "commitList/selectCommitUp", "CommitList/Select Next Commit Up");

CommitList::CommitList(Index *index, RepoView *view)
    : QObject(view), mView(view), mIndex(index) {
  git::Repository repo = index->repo();
  mList = new ListModel(&mRefs, this);
  mModel = new CommitModel(repo, &mRefs, this);

  // Restore the soloed branches that still exist.
  QString solo = repo.appConfig().value<QString>(kSoloKey, QString());
  for (const QString &name : solo.split(' ', Qt::SkipEmptyParts)) {
    if (repo.lookupRef(name).isValid())
      mSolo.append(name);
  }
  static_cast<CommitModel *>(mModel)->setSolo(mSolo);

  // Restore the hidden branches.
  QString hidden = repo.appConfig().value<QString>(kHiddenKey, QString());
  mHidden = hidden.split(' ', Qt::SkipEmptyParts);
  static_cast<CommitModel *>(mModel)->setHidden(mHidden);

  setModel(mModel);
  updateRefs();

  connect(mModel, &QAbstractItemModel::modelAboutToBeReset, this,
          &CommitList::storeSelection);
  connect(mModel, &QAbstractItemModel::modelReset, this,
          &CommitList::restoreSelection);
  connect(mList, &QAbstractItemModel::modelAboutToBeReset, this,
          &CommitList::storeSelection);
  connect(mList, &QAbstractItemModel::modelReset, this,
          &CommitList::restoreSelection);

  connect(mModel, &QAbstractItemModel::modelReset, this,
          &CommitList::updateLaneCount);
  connect(mModel, &QAbstractItemModel::rowsInserted, this,
          &CommitList::updateLaneCount);

  CommitModel *model = static_cast<CommitModel *>(mModel);
  connect(model, &CommitModel::statusFinished, this, [this](bool visible) {
    mRestoreSelection = true; // Reset to default

    // Select the first commit if the selection was cleared.
    if (selectedIndexes().isEmpty())
      selectFirstCommit();

    // Notify main window.
    emit statusChanged(visible);
  });

  connect(model, &CommitModel::loadingChanged, this, &CommitList::setLoading);

  git::RepositoryNotifier *notifier = repo.notifier();
  connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
          [this](const git::Reference &ref, bool restoreSelection) {
            mRestoreSelection = restoreSelection;
            resetReference(ref);
          });
  connect(notifier, &git::RepositoryNotifier::workdirChanged, this, [this] {
    resetReference(static_cast<const CommitModel *>(mModel)->reference());
  });

  connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
          &CommitList::updateRefs);
  connect(notifier, &git::RepositoryNotifier::referenceAdded, this,
          &CommitList::updateRefs);
  connect(notifier, &git::RepositoryNotifier::referenceRemoved, this,
          &CommitList::updateRefs);

  // Stop soloing and hiding branches that are gone.
  connect(notifier, &git::RepositoryNotifier::referenceRemoved, this, [this] {
    QStringList solo;
    git::Repository repo = mView->repo();
    for (const QString &name : mSolo) {
      if (repo.lookupRef(name).isValid())
        solo.append(name);
    }
    setSolo(solo);

    QStringList hidden;
    for (const QString &name : mHidden) {
      if (name.endsWith('/') || repo.lookupRef(name).isValid())
        hidden.append(name);
    }
    setHidden(hidden);
  });

  QShortcut *shortcut = new QShortcut(view);
  selectCommitDownHotKey.use(shortcut);
  connect(shortcut, &QShortcut::activated, this,
          [this] { selectCommitRelative(1); });

  shortcut = new QShortcut(view);
  selectCommitUpHotKey.use(shortcut);
  connect(shortcut, &QShortcut::activated, this,
          [this] { selectCommitRelative(-1); });

  // Settings that change how rows are drawn.
  connect(Settings::instance(), &Settings::settingsChanged, this,
          &CommitList::settingsChanged);
}

QHash<int, QByteArray> CommitList::roleNames() {
  return {{Qt::DisplayRole, "display"},
          {GraphRole, "graph"},
          {GraphColorRole, "graphColors"},
          {StatusRole, "isStatus"},
          {SummaryRole, "summary"},
          {AuthorRole, "author"},
          {InitialsRole, "initials"},
          {DateRole, "date"},
          {ShortIdRole, "shortId"},
          {StarredRole, "starred"},
          {RefsRole, "refs"},
          {NodeColorRole, "nodeColor"},
          {MergeRole, "isMerge"},
          {BusyRole, "busy"},
          {WipRole, "wip"}};
}

git::Diff CommitList::status() const {
  return static_cast<CommitModel *>(mModel)->status();
}

QString CommitList::selectedRange() const {
  QList<git::Commit> commits = selectedCommits();
  if (commits.isEmpty())
    return !selectedIndexes().isEmpty() ? "status" : QString();

  git::Commit first = commits.first();
  if (commits.size() == 1)
    return first.id().toString();

  git::Commit last = commits.last();
  return QString("%1..%2").arg(last.id().toString(), first.id().toString());
}

git::Diff CommitList::selectedDiff() const {
  QModelIndexList indexes = sortedIndexes();
  if (indexes.isEmpty())
    return git::Diff();

  if (indexes.size() == 1) {
    auto first = indexes.first().data(DiffRole);
    return first.isValid() ? first.value<git::Diff>() : git::Diff();
  }

  git::Commit first = indexes.first().data(CommitRole).value<git::Commit>();
  if (!first.isValid())
    return git::Diff();

  git::Commit last = indexes.last().data(CommitRole).value<git::Commit>();
  bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();
  git::Diff diff = first.diff(last, -1, ignoreWhitespace);
  diff.findSimilar();
  return diff;
}

QList<git::Commit> CommitList::selectedCommits() const {
  QList<git::Commit> selectedCommits;
  for (const QModelIndex &index : sortedIndexes()) {
    git::Commit commit = index.data(CommitRole).value<git::Commit>();
    if (commit.isValid())
      selectedCommits.append(commit);
  }

  return selectedCommits;
}

QModelIndexList CommitList::selectedIndexes() const {
  return mSelection ? mSelection->selectedIndexes() : QModelIndexList();
}

void CommitList::cancelStatus() {
  static_cast<CommitModel *>(mModel)->cancelStatus();
}

void CommitList::setReference(const git::Reference &ref) {
  static_cast<CommitModel *>(mModel)->setReference(ref);
  if (!isResetWalkerSuppressed())
    updateModel();
}

void CommitList::setFilter(const QString &filter) {
  mFilter = filter.simplified();
  updateModel();
}

void CommitList::setPathspec(const QString &pathspec, bool index) {
  if (index) {
    setFilter(!pathspec.isEmpty() ? kPathspecFmt.arg(pathspec) : QString());
  } else {
    static_cast<CommitModel *>(mModel)->setPathspec(pathspec);
  }
}

void CommitList::setCommits(const QList<git::Commit> &commits) {
  setModel(mList);
  static_cast<ListModel *>(mList)->setList(commits);
}

void CommitList::selectReference(const git::Reference &ref) {
  if (!ref.isValid())
    return;

  QModelIndex index = model()->index(0, 0);
  if (ref.isHead() && !index.data(CommitRole).isValid()) {
    selectFirstCommit();
  } else {
    selectRange(ref.target().id().toString());
  }
}

void CommitList::resetSelection(bool spontaneous) {
  // Just notify.
  mSpontaneous = spontaneous;
  notifySelectionChanged();
  mSpontaneous = true;
}

void CommitList::selectFirstCommit(bool spontaneous) {
  QModelIndex index = model()->index(0, 0);
  if (index.isValid()) {
    selectIndexes(QItemSelection(index, index), QString(), spontaneous);
  } else {
    // Invalidate any in-flight async diff so a stale result for a
    // previously selected commit can't be delivered after this reset.
    ++mDiffRequest;
    emit diffSelected(git::Diff());
  }

  // This is the automatic fallback selection, not a deliberate pick, so a
  // later background refresh is free to move it instead of pinning it here.
  mSelectionIsDefault = true;
}

void CommitList::selectCommitRelative(int offset) {
  QModelIndexList indexes = sortedIndexes();
  if (indexes.isEmpty())
    return;

  QModelIndex index = (offset < 0) ? indexes.first() : indexes.last();
  int row = index.row() + offset;
  if (row >= model()->rowCount() && model()->canFetchMore(QModelIndex()))
    model()->fetchMore(QModelIndex());

  QModelIndex next = model()->index(row, 0);
  if (next.isValid())
    selectIndexes(QItemSelection(next, next), QString(), true);
}

void CommitList::selectRow(int row) {
  QModelIndex index = model()->index(row, 0);
  if (index.isValid())
    selectIndexes(QItemSelection(index, index), QString(), true);
}

bool CommitList::selectRange(const QString &range, const QString &file,
                             bool spontaneous) {
  // Try to select the "status" index.
  QModelIndex index = model()->index(0, 0);
  if (range == "status" && !index.data(CommitRole).isValid()) {
    return true;
  }

  QStringList ids = range.split("..");
  if (ids.size() > 2)
    return false;

  // Invert range.
  bool one = (ids.size() == 1);
  git::Repository repo = mView->repo();
  git::Commit firstCommit = repo.lookupCommit(ids.last());
  git::Commit lastCommit = one ? firstCommit : repo.lookupCommit(ids.first());

  // Check for already selected range.
  QModelIndexList indexes = sortedIndexes();
  if (indexes.size() >= 2) {
    git::Commit first = indexes.first().data(CommitRole).value<git::Commit>();
    git::Commit last = indexes.last().data(CommitRole).value<git::Commit>();
    if (first.isValid() && first == firstCommit && last.isValid() &&
        last == lastCommit)
      return false;
  }

  // Find indexes.
  QItemSelection selection;
  QModelIndex first = findCommit(firstCommit);
  if (!first.isValid())
    return false;
  selection.select(first, first);

  if (lastCommit != firstCommit) {
    QModelIndex last = findCommit(lastCommit);
    if (!last.isValid())
      return false;
    selection.select(last, last);
  }

  selectIndexes(selection, file, spontaneous);
  return true;
}

void CommitList::suppressResetWalker(bool suppress) {
  static_cast<CommitModel *>(mModel)->suppressResetWalker(suppress);
}

void CommitList::resetReference(const git::Reference &ref) {
  static_cast<CommitModel *>(mModel)->resetReference(ref);
}

bool CommitList::isResetWalkerSuppressed() {
  return static_cast<CommitModel *>(mModel)->isResetWalkerSuppressed();
}

void CommitList::resetSettings() {
  static_cast<CommitModel *>(mModel)->resetSettings(true);
  emit settingsChanged();
}

bool CommitList::showAuthor() const {
  return Settings::instance()->value(Setting::Id::ShowCommitsAuthor, true)
      .toBool();
}

bool CommitList::showDate() const {
  return Settings::instance()->value(Setting::Id::ShowCommitsDate, true)
      .toBool();
}

bool CommitList::showId() const {
  return Settings::instance()->value(Setting::Id::ShowCommitsId, true).toBool();
}

bool CommitList::compact() const {
  return Settings::instance()
      ->value(Setting::Id::ShowCommitsInCompactMode)
      .toBool();
}

QString CommitList::refsFilterName() const {
  git::Config config = mView->repo().appConfig();
  switch (static_cast<RefsFilter>(config.value<int>(
      ConfigKeys::kRefsKey, static_cast<int>(RefsFilter::AllRefs)))) {
    case RefsFilter::AllRefs:
      return tr("All Branches");
    case RefsFilter::SelectedRef:
      return tr("Selected Branch");
    case RefsFilter::SelectedRefIgnoreMerge:
      return tr("Selected Branch, First Parent");
  }

  return QString();
}

QString CommitList::sortName() const {
  git::Config config = mView->repo().appConfig();
  return config.value<bool>(ConfigKeys::kSortKey, true) ? tr("By Date")
                                                        : tr("Topological");
}

bool CommitList::isSelected(int row) const {
  return mSelection && mSelection->isSelected(model()->index(row, 0));
}

void CommitList::click(int row, int modifiers) {
  QModelIndex index = model()->index(row, 0);
  if (!index.isValid())
    return;

  // Selecting a second commit selects the range between them.
  bool extend = modifiers & (Qt::ControlModifier | Qt::ShiftModifier |
                             Qt::MetaModifier);
  if (extend && !selectedIndexes().isEmpty()) {
    if (mSelection->isSelected(index) && selectedIndexes().size() > 1) {
      mSelection->select(index, QItemSelectionModel::Deselect);
      return;
    }

    QModelIndex anchor = sortedIndexes().first();
    QItemSelection selection;
    selection.select(anchor, anchor);
    selection.select(index, index);
    selectIndexes(selection, QString(), true);
    return;
  }

  if (selectedIndexes().size() == 1 && mSelection->isSelected(index))
    return;

  selectIndexes(QItemSelection(index, index), QString(), true);
}

void CommitList::toggleStar(int row) {
  QModelIndex index = model()->index(row, 0);
  git::Commit commit = index.data(CommitRole).value<git::Commit>();
  if (!commit.isValid())
    return;

  commit.setStarred(!commit.isStarred());
  emit model()->dataChanged(index, index, {StarredRole});
}

void CommitList::fetchMore() {
  if (model()->canFetchMore(QModelIndex()))
    model()->fetchMore(QModelIndex());
}

/// @brief Helper function to add a list of items to a menu.
/// A single item is added directly to the menu, whereas multiple items will
/// be added to a sub-menu.
static void addMenuEntries(QMenu &menu, const QString &operation,
                           const QList<git::Reference> &items,
                           std::function<void(const git::Reference &)> action) {
  QMenu *submenu = &menu;
  QString entryName(operation + " %1");
  if (items.count() > 1) {
    submenu = menu.addMenu(operation);
    entryName = QString("%1");
  }
  for (const git::Reference &ref : items) {
    submenu->addAction(entryName.arg(ref.name()),
                       [action, ref] { action(ref); });
  }
}

void CommitList::showContextMenu(int row, qreal x, qreal y) {
  QModelIndex index = model()->index(row, 0);
  if (!index.isValid())
    return;

  // Right-clicking outside of the selection selects the row first.
  if (!mSelection->isSelected(index))
    selectIndexes(QItemSelection(index, index), QString(), true);

  RepoView *view = mView;
  QPoint pos = view->mapFromPage(x, y);
  git::Commit commit = index.data(CommitRole).value<git::Commit>();

  if (!commit.isValid()) {
    QMenu menu;

    // clean
    QStringList untracked;
    if (git::Diff diff = status()) {
      for (int i = 0; i < diff.count(); i++) {
        if (diff.status(i) == GIT_DELTA_UNTRACKED)
          untracked.append(diff.name(i));
      }
    }

    QAction *clean =
        menu.addAction(tr("Remove Untracked Files"),
                       [view, untracked] { view->clean(untracked); });

    clean->setEnabled(!untracked.isEmpty());

    QmlSupport::execMenu(&menu, pos);
    return;
  }

  QMenu menu;
  menu.setToolTipsVisible(true);

  // stash
  git::Reference ref = static_cast<CommitModel *>(mModel)->reference();
  if (ref.isValid() && ref.isStash()) {
    menu.addAction(tr("Apply"),
                   [view, index] { view->applyStash(index.row()); });

    menu.addAction(tr("Pop"), [view, index] { view->popStash(index.row()); });

    menu.addAction(tr("Drop"), [view, index] { view->dropStash(index.row()); });

  } else {
    // multiple selection
    bool anyStarred = false;
    for (const QModelIndex &index : selectedIndexes()) {
      if (index.data(CommitRole).isValid() &&
          index.data(CommitRole).value<git::Commit>().isStarred()) {
        anyStarred = true;
        break;
      }
    }

    menu.addAction(anyStarred ? tr("Unstar") : tr("Star"), [this, anyStarred] {
      for (const QModelIndex &index : selectedIndexes()) {
        if (index.data(CommitRole).isValid()) {
          index.data(CommitRole).value<git::Commit>().setStarred(!anyStarred);
          emit model()->dataChanged(index, index, {StarredRole});
        }
      }
    });

    // Solo the branches of the commit, or stop soloing.
    QList<git::Reference> branches;
    for (const git::Reference &ref : view->repo().refs()) {
      if ((ref.isLocalBranch() || ref.isRemoteBranch()) &&
          !ref.name().endsWith("/HEAD") && ref.target() == commit)
        branches.append(ref);
    }

    if (!branches.isEmpty() || !mSolo.isEmpty())
      menu.addSeparator();

    if (branches.size() == 1) {
      QString name = branches.first().qualifiedName();
      bool soloed = isSoloed(name);
      QString text = soloed ? tr("Unsolo %1") : tr("Solo %1");
      menu.addAction(text.arg(branches.first().name()),
                     [this, name, soloed] { setSoloed(name, !soloed); });
    } else if (branches.size() > 1) {
      QMenu *soloMenu = menu.addMenu(tr("Solo"));
      for (const git::Reference &ref : branches) {
        QString name = ref.qualifiedName();
        QAction *action = soloMenu->addAction(
            ref.name(), [this, name](bool checked) { setSoloed(name, checked); });
        action->setCheckable(true);
        action->setChecked(isSoloed(name));
      }
    }

    if (!mSolo.isEmpty())
      menu.addAction(tr("Unsolo All"), [this] { unsoloAll(); });

    // Hide the branches of the commit from the graph, like GitKraken.
    QList<git::Reference> hideable;
    for (const git::Reference &ref : branches) {
      if (!ref.isHead())
        hideable.append(ref);
    }

    if (hideable.size() == 1) {
      QString name = hideable.first().qualifiedName();
      menu.addAction(tr("Hide %1").arg(hideable.first().name()),
                     [this, name] { setHidden(name, true); });
    } else if (hideable.size() > 1) {
      QMenu *hideMenu = menu.addMenu(tr("Hide"));
      for (const git::Reference &ref : hideable) {
        QString name = ref.qualifiedName();
        hideMenu->addAction(ref.name(),
                            [this, name] { setHidden(name, true); });
      }
    }

    if (!mHidden.isEmpty())
      menu.addAction(tr("Show All Hidden Branches"), [this] { showAll(); });

    // single selection
    if (selectedIndexes().size() <= 1) {
      menu.addSeparator();

      menu.addAction(tr("Add Tag..."),
                     [view, commit] { view->promptToAddTag(commit); });

      menu.addAction(tr("New Branch..."),
                     [view, commit] { view->promptToCreateBranch(commit); });

      // Add operations on existing references; there may be 0, 1, or multiple
      // of each type of reference on a commit.
      QList<git::Reference> rename_branches;
      QList<git::Reference> tags;
      QList<git::Reference> delete_branches;
      QList<git::Reference> all_branches; // used later
      for (const git::Reference &ref : commit.refs()) {
        if (ref.isTag()) {
          tags.append(ref);
        } else if (ref.isBranch()) {
          all_branches.append(ref);
          if (ref.isLocalBranch()) {
            rename_branches.append(ref);
            if (view->repo().head().name() != ref.name()) {
              delete_branches.append(ref);
            }
          }
        }
      }

      if (rename_branches.count() > 0 || delete_branches.count() > 0 ||
          tags.count() > 0) {
        menu.addSeparator();
      }
      addMenuEntries(menu, tr("Rename Branch"), rename_branches,
                     std::bind(&RepoView::promptToRenameBranch, view,
                               std::placeholders::_1));

      addMenuEntries(menu, tr("Delete Branch"), delete_branches,
                     std::bind(&RepoView::promptToDeleteBranch, view,
                               std::placeholders::_1));

      addMenuEntries(
          menu, tr("Delete Tag"), tags,
          std::bind(&RepoView::promptToDeleteTag, view, std::placeholders::_1));
      menu.addSeparator();

      auto addMergeAction = [&menu, view, commit](const QString &text,
                                                  RepoView::MergeFlag flag) {
        menu.addAction(text, [view, commit, flag] {
          MergeDialog *dialog = new MergeDialog(flag, view->repo(), view);
          connect(dialog, &QDialog::accepted, [view, dialog] {
            git::AnnotatedCommit upstream;
            git::Reference ref = dialog->reference();
            if (!ref.isValid())
              upstream = dialog->target().annotatedCommit();
            view->merge(dialog->flags(), ref, upstream);
          });

          dialog->setCommit(commit);
          dialog->open();
        });
      };

      addMergeAction(tr("Merge..."), RepoView::Merge);
      addMergeAction(tr("Rebase..."), RepoView::Rebase);

      // Pick, reword, squash, drop and reorder the commits of the current
      // branch after this one, like GitKraken.
      git::Reference current = view->repo().head();
      if (current.isValid() && current.isLocalBranch() &&
          view->interactiveRebase()->canOpen(current.qualifiedName(),
                                             commit)) {
        QString text = tr("Interactive Rebase %1 onto %2...")
                           .arg(current.name(), commit.shortId());
        menu.addAction(text, [view, current, commit] {
          view->interactiveRebase()->open(current.qualifiedName(), commit,
                                          commit.shortId());
        });
      }

      addMergeAction(tr("Squash..."), RepoView::Squash);

      menu.addSeparator();

      menu.addAction(tr("Revert"), [view, commit] { view->revert(commit); });

      menu.addAction(tr("Cherry-pick"),
                     [view, commit] { view->cherryPick(commit); });

      menu.addSeparator();

      git::Reference head = view->repo().head();
      auto submenu = &menu;
      auto entryName = tr("Checkout %1");
      if (all_branches.count() > 1) {
        submenu = menu.addMenu(tr("Checkout"));
        entryName = QString("%1");
      }
      for (const git::Reference &ref : all_branches) {
        if (ref.isLocalBranch()) {
          QAction *checkout = submenu->addAction(
              entryName.arg(ref.name()), [view, ref] { view->checkout(ref); });

          checkout->setEnabled(head.isValid() &&
                               head.qualifiedName() != ref.qualifiedName() &&
                               !view->repo().isBare());
        } else if (ref.isRemoteBranch()) {
          QAction *checkout = submenu->addAction(
              entryName.arg(ref.name()), [view, ref] { view->checkout(ref); });

          // Calculate local branch name in the same way as checkout() does
          QString local = ref.name().section('/', 1);
          if (!head.isValid()) { // I'm not sure when this can happen
            checkout->setEnabled(false);
          } else if (head.name() == local) {
            checkout->setEnabled(false);
            checkout->setToolTip(tr("Local branch is already checked out"));
          } else if (view->repo().isBare()) {
            checkout->setEnabled(false);
            checkout->setToolTip(tr("This is a bare repository"));
          }
        }
      }

      QString name = commit.detachedHeadName();
      QAction *checkout =
          menu.addAction(tr("Checkout %1").arg(name),
                         [view, commit] { view->checkout(commit); });

      checkout->setEnabled(head.isValid() && head.target() != commit &&
                           !view->repo().isBare());

      menu.addSeparator();

      QMenu *reset = menu.addMenu(tr("Reset"));
      reset->addAction(tr("Soft"))->setData(GIT_RESET_SOFT);
      reset->addAction(tr("Mixed"))->setData(GIT_RESET_MIXED);
      reset->addAction(tr("Hard"))->setData(GIT_RESET_HARD);
      connect(reset, &QMenu::triggered, [view, commit](QAction *action) {
        git_reset_t type = static_cast<git_reset_t>(action->data().toInt());
        view->promptToReset(commit, type);
      });

      reset->setEnabled(head.isValid() && head.isLocalBranch());
    }
  }

  QmlSupport::execMenu(&menu, pos);
}

void CommitList::showRefsFilterMenu(qreal x, qreal y) {
  git::Config config = mView->repo().appConfig();
  int current = config.value<int>(ConfigKeys::kRefsKey,
                                  static_cast<int>(RefsFilter::AllRefs));

  QMenu menu;
  QList<QPair<QString, RefsFilter>> entries = {
      {tr("Show All Branches"), RefsFilter::AllRefs},
      {tr("Show Selected Branch"), RefsFilter::SelectedRef},
      {tr("Show Selected Branch, First Parent Only"),
       RefsFilter::SelectedRefIgnoreMerge}};
  for (const auto &entry : entries) {
    QAction *action = menu.addAction(entry.first, [this, entry] {
      setConfigValue(ConfigKeys::kRefsKey, static_cast<int>(entry.second));
    });
    action->setCheckable(true);
    action->setChecked(current == static_cast<int>(entry.second));
  }

  QmlSupport::execMenu(&menu, mView->mapFromPage(x, y));
}

void CommitList::showSortMenu(qreal x, qreal y) {
  git::Config config = mView->repo().appConfig();
  bool date = config.value<bool>(ConfigKeys::kSortKey, true);

  QMenu menu;
  QAction *byDate = menu.addAction(
      tr("Sort by Date"), [this] { setConfigValue(ConfigKeys::kSortKey, true); });
  byDate->setCheckable(true);
  byDate->setChecked(date);

  QAction *topological =
      menu.addAction(tr("Sort Topologically"),
                     [this] { setConfigValue(ConfigKeys::kSortKey, false); });
  topological->setCheckable(true);
  topological->setChecked(!date);

  QmlSupport::execMenu(&menu, mView->mapFromPage(x, y));
}

void CommitList::showSettingsMenu(qreal x, qreal y) {
  git::Config config = mView->repo().appConfig();
  Settings *settings = Settings::instance();

  QMenu menu;
  QAction *graph = menu.addAction(tr("Show Graph"), [this](bool checked) {
    setConfigValue(ConfigKeys::kGraphKey, checked);
  });
  graph->setCheckable(true);
  graph->setChecked(config.value<bool>(ConfigKeys::kGraphKey, true));

  QAction *status =
      menu.addAction(tr("Show Clean Status"), [this](bool checked) {
        setConfigValue(ConfigKeys::kStatusKey, checked);
      });
  status->setCheckable(true);
  status->setChecked(config.value<bool>(ConfigKeys::kStatusKey, true));

  menu.addSeparator();

  auto addSetting = [this, &menu, settings](const QString &text,
                                            Setting::Id id, bool defaultValue) {
    QAction *action = menu.addAction(text, [this, settings, id](bool checked) {
      settings->setValue(id, checked);
      resetSettings();
    });
    action->setCheckable(true);
    action->setChecked(settings->value(id, defaultValue).toBool());
  };

  addSetting(tr("Compact Mode"), Setting::Id::ShowCommitsInCompactMode, false);
  menu.addSeparator();
  addSetting(tr("Show Author"), Setting::Id::ShowCommitsAuthor, true);
  addSetting(tr("Show Date"), Setting::Id::ShowCommitsDate, true);
  addSetting(tr("Show Id"), Setting::Id::ShowCommitsId, true);

  QmlSupport::execMenu(&menu, mView->mapFromPage(x, y));
}

QString CommitList::soloText() const {
  if (mSolo.size() == 1)
    return mView->repo().lookupRef(mSolo.first()).name();

  return tr("%1 branches").arg(mSolo.size());
}

bool CommitList::isSoloed(const QString &name) const {
  return mSolo.contains(name);
}

void CommitList::setSoloed(const QString &name, bool soloed) {
  QStringList solo = mSolo;
  if (soloed && !solo.contains(name))
    solo.append(name);
  else if (!soloed)
    solo.removeAll(name);

  setSolo(solo);
}

void CommitList::unsoloAll() { setSolo(QStringList()); }

QString CommitList::hiddenText() const {
  if (mHidden.size() == 1) {
    QString name = mHidden.first();
    if (name.endsWith('/'))
      return name.section('/', 2, 2);
    return mView->repo().lookupRef(name).name();
  }

  return tr("%1 branches").arg(mHidden.size());
}

bool CommitList::isHidden(const QString &name) const {
  return matchesHidden(mHidden, name);
}

bool CommitList::canHide(const QString &name) const {
  git::Reference head = mView->repo().head();
  return !head.isValid() || head.qualifiedName() != name;
}

void CommitList::setHidden(const QString &name, bool hidden) {
  QStringList list = mHidden;
  if (hidden && !list.contains(name) && canHide(name)) {
    list.append(name);
  } else if (!hidden) {
    list.removeAll(name);
  }

  setHidden(list);
}

void CommitList::showAll() { setHidden(QStringList()); }

void CommitList::setHidden(const QStringList &hidden) {
  if (hidden == mHidden)
    return;

  mHidden = hidden;
  mView->repo().appConfig().setValue(kHiddenKey, hidden.join(' '));

  updateRefs();
  static_cast<CommitModel *>(mModel)->setHidden(hidden);
  emit hiddenChanged();
}

void CommitList::setSolo(const QStringList &solo) {
  if (solo == mSolo)
    return;

  mSolo = solo;
  mView->repo().appConfig().setValue(kSoloKey, solo.join(' '));

  updateRefs();
  static_cast<CommitModel *>(mModel)->setSolo(solo);
  emit soloChanged();
}

void CommitList::setConfigValue(const QString &key, const QVariant &value) {
  git::Config config = mView->repo().appConfig();
  if (value.typeId() == QMetaType::Bool) {
    config.setValue(key, value.toBool());
  } else {
    config.setValue(key, value.toInt());
  }

  resetSettings();
}

void CommitList::setModel(QAbstractItemModel *model) {
  if (model == mCurrent)
    return;

  if (mCurrent)
    storeSelection();

  // Destroy the previous selection model.
  delete mSelection;

  mCurrent = model;
  mSelection = new SelectionModel(model);
  connect(mSelection, &QItemSelectionModel::selectionChanged, this, [this] {
    // Assume this selection is deliberate
    mSelectionIsDefault = false;

    ++mSelectionRevision;
    emit selectionRevisionChanged();

    notifySelectionChanged();
  });

  emit modelChanged();
  updateLaneCount();

  restoreSelection();
}

void CommitList::setLoading(bool loading) {
  if (loading == mLoading)
    return;

  mLoading = loading;
  emit loadingChanged(loading);
}

void CommitList::storeSelection() {
  // Don't pin the selection to a stale commit id across the reset: leave
  // mSelectedRange empty so restoreSelection() defers to the fallback
  // selection (selectFirstCommit(), triggered via statusFinished), which
  // picks up whatever the new default is
  mSelectedRange = mSelectionIsDefault ? QString() : selectedRange();
  DebugRefresh("Selected Range: " << mSelectedRange);
}

void CommitList::restoreSelection() {
  // Restore selection.
  DebugRefresh(mSelectedRange);
  if (!mRestoreSelection ||
      (!mSelectedRange.isEmpty() && mSelectedRange != "status" &&
       !selectRange(mSelectedRange))) {
    DebugRefresh("Failed to restore");
    // Invalidate any in-flight async diff so a stale result for a
    // previously selected commit can't be delivered after this reset.
    ++mDiffRequest;
    emit diffSelected(git::Diff());
  }

  mSelectedRange = QString();

  if (selectedIndexes().isEmpty())
    selectFirstCommit();

  // The rows were replaced, so the selection highlight has to be redrawn.
  ++mSelectionRevision;
  emit selectionRevisionChanged();
}

void CommitList::updateModel() {
  if (!mFilter.isEmpty()) {
    setCommits(mIndex->commits(mFilter));
    return;
  }

  git::Reference ref = static_cast<CommitModel *>(mModel)->reference();
  if (ref.isValid() && ref.isStash()) {
    setCommits(ref.repo().stashes());
    return;
  }

  // Reset model.
  setModel(mModel);
}

void CommitList::updateRefs() {
  mRefs.clear();

  git::Repository repo = mView->repo();
  git::Reference head = repo.head();
  if (repo.isHeadDetached()) {
    mRefs[head.target().id()].append(QVariantMap{
        {"name", head.name()}, {"head", true}, {"local", true}});
  }

  // Only the soloed branches and their upstream branches are shown while
  // soloing.
  QSet<QString> solo;
  for (const QString &name : mSolo) {
    solo.insert(name);
    git::Reference ref = repo.lookupRef(name);
    if (ref.isLocalBranch()) {
      if (git::Branch upstream = git::Branch(ref).upstream())
        solo.insert(upstream.qualifiedName());
    }
  }

  // Merge local branches with remote branches of the same name, the way
  // GitKraken shows them.
  QMap<git::Id, QVariantList> remotes;
  for (const git::Reference &ref : repo.refs()) {
    git::Commit target = ref.target();
    if (!target.isValid() || ref.isStash())
      continue;

    if (!solo.isEmpty() && (ref.isLocalBranch() || ref.isRemoteBranch()) &&
        !solo.contains(ref.qualifiedName()))
      continue;

    // Hidden branches have no labels, unless they are soloed.
    if (solo.isEmpty() && !ref.isHead() &&
        matchesHidden(mHidden, ref.qualifiedName()))
      continue;

    if (ref.isRemoteBranch()) {
      if (ref.name().endsWith("/HEAD"))
        continue;
      remotes[target.id()].append(ref.name());
      continue;
    }

    mRefs[target.id()].append(
        QVariantMap{{"name", ref.name()},
                    {"qualified", ref.qualifiedName()},
                    {"head", ref.isHead()},
                    {"tag", ref.isTag()},
                    {"local", ref.isLocalBranch()}});
  }

  for (auto it = remotes.cbegin(); it != remotes.cend(); ++it) {
    QVariantList &refs = mRefs[it.key()];
    for (const QVariant &name : it.value()) {
      QString remote = name.toString();
      QString local = remote.section('/', 1);

      bool merged = false;
      for (QVariant &ref : refs) {
        QVariantMap map = ref.toMap();
        if (map.value("local").toBool() && map.value("name") == local) {
          map.insert("remote", true);
          ref = map;
          merged = true;
          break;
        }
      }

      if (!merged)
        refs.append(QVariantMap{{"name", remote},
                                {"qualified", "refs/remotes/" + remote},
                                {"remote", true}});
    }
  }

  // Show HEAD first.
  for (QVariantList &refs : mRefs) {
    std::stable_sort(refs.begin(), refs.end(),
                     [](const QVariant &lhs, const QVariant &rhs) {
                       return lhs.toMap().value("head").toBool() &&
                              !rhs.toMap().value("head").toBool();
                     });
  }

  QAbstractItemModel *model = this->model();
  if (model && model->rowCount() > 0)
    emit model->dataChanged(model->index(0, 0),
                            model->index(model->rowCount() - 1, 0),
                            {RefsRole});
}

void CommitList::updateLaneCount() {
  int count = 0;
  if (mCurrent == mModel)
    count = static_cast<CommitModel *>(mModel)->laneCount();

  if (count != mLaneCount) {
    mLaneCount = count;
    emit laneCountChanged();
  }
}

QModelIndexList CommitList::sortedIndexes() const {
  QModelIndexList indexes = selectedIndexes();
  std::sort(indexes.begin(), indexes.end(),
            [](const QModelIndex &lhs, const QModelIndex &rhs) {
              return lhs.row() < rhs.row();
            });

  return indexes;
}

QModelIndex CommitList::findCommit(const git::Commit &commit) {
  // Get the 'uncommitted changes' index.
  QAbstractItemModel *model = this->model();
  if (!commit.isValid()) {
    QModelIndex index = model->index(0, 0);
    git::Commit tmp = index.data(CommitRole).value<git::Commit>();
    return !tmp.isValid() ? index : QModelIndex();
  }

  // Find the id.
  QDateTime date = commit.committer().date();
  for (int i = 0; i < model->rowCount(); ++i) {
    QModelIndex index = model->index(i, 0);
    if (git::Commit tmp = index.data(CommitRole).value<git::Commit>()) {
      if (tmp == commit)
        return index;

      // Cut off search if we find an older commit.
      if (tmp.committer().date() < date)
        return QModelIndex();
    }

    // Load more commits.
    if (i == model->rowCount() - 1 && model->canFetchMore(QModelIndex()))
      model->fetchMore(QModelIndex());
  }

  return QModelIndex();
}

void CommitList::selectIndexes(const QItemSelection &selection,
                               const QString &file, bool spontaneous) {
  mFile = file;
  mSpontaneous = spontaneous;
  mSelection->select(selection, QItemSelectionModel::ClearAndSelect);
  mSpontaneous = true;
  mFile = QString();

  QModelIndexList indexes = selection.indexes();
  if (!indexes.isEmpty()) {
    mSelection->setCurrentIndex(indexes.first(), QItemSelectionModel::NoUpdate);
    emit scrollRequested(indexes.first().row());
  }
}

void CommitList::notifySelectionChanged() {
  // Multiple selection means that the selected parameter
  // could be empty when there are still indexes selected.
  if (selectedIndexes().isEmpty())
    return;

  dispatchSelectedDiff(mFile, mSpontaneous);
}

void CommitList::dispatchSelectedDiff(const QString &file, bool spontaneous) {
  // Any in-flight request is now stale.
  int request = ++mDiffRequest;

  QModelIndexList indexes = sortedIndexes();
  if (indexes.isEmpty()) {
    emit diffSelected(git::Diff(), file, spontaneous);
    return;
  }

  // The uncommitted-changes row's diff is already computed asynchronously
  // elsewhere (CommitModel::status()); no need to compute it again.
  if (indexes.size() == 1) {
    git::Commit commit = indexes.first().data(CommitRole).value<git::Commit>();
    if (!commit.isValid()) {
      QVariant data = indexes.first().data(DiffRole);
      git::Diff diff = data.isValid() ? data.value<git::Diff>() : git::Diff();
      emit diffSelected(diff, file, spontaneous);
      return;
    }
  }

  git::Commit first = indexes.first().data(CommitRole).value<git::Commit>();
  if (!first.isValid()) {
    emit diffSelected(git::Diff(), file, spontaneous);
    return;
  }

  git::Commit last = indexes.last().data(CommitRole).value<git::Commit>();
  bool range = (indexes.size() > 1);
  bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();

  // Let the diff/blame/file-list views clear themselves and show a loading
  // indicator while the (potentially slow) diff is computed.
  emit diffLoading();

  // Compute the diff and run rename detection off the GUI thread; this can
  // be slow for large commits/ranges. Discard the result if a newer
  // selection has superseded this request by the time it finishes.
  auto *watcher = new QFutureWatcher<git::Diff>(this);
  connect(watcher, &QFutureWatcher<git::Diff>::finished, watcher,
          [this, watcher, request, file, spontaneous] {
            git::Diff diff = watcher->result();
            watcher->deleteLater();
            // TODO: It would be great to have some cancel pathway instead of
            // doing this hack
            if (request == mDiffRequest)
              emit diffSelected(diff, file, spontaneous);
          });

  watcher->setFuture(QtConcurrent::run([first, last, range, ignoreWhitespace] {
    git::Diff diff = range ? first.diff(last, -1, ignoreWhitespace)
                           : first.diff(git::Commit(), -1, ignoreWhitespace);
    diff.findSimilar();
    return diff;
  }));
}

#include "CommitList.moc"
