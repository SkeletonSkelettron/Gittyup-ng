//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "RefsPanel.h"
#include "PullRequestList.h"
#include "dialogs/PullRequestDialog.h"
#include "InteractiveRebase.h"
#include "qml/QmlSupport.h"
#include "CommitList.h"
#include "ConfigKeys.h"
#include "RepoView.h"
#include "dialogs/DeleteBranchDialog.h"
#include "dialogs/DeleteTagDialog.h"
#include "dialogs/MergeDialog.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Remote.h"
#include "git/Submodule.h"
#include <QMenu>
#include <QSettings>

namespace {

const QString kCollapsedKey = "refspanel/collapsed";

bool lessThan(const RefsModel::Item &lhs, const RefsModel::Item &rhs) {
  return lhs.name.compare(rhs.name, Qt::CaseInsensitive) < 0;
}

} // namespace

RefsModel::RefsModel(const git::Repository &repo, QObject *parent)
    : QAbstractListModel(parent), mRepo(repo) {
  const QStringList collapsed = QSettings().value(kCollapsedKey).toStringList();
  mCollapsed = QSet<QString>(collapsed.begin(), collapsed.end());
  update();
}

void RefsModel::setCurrent(const git::Reference &ref) {
  mCurrent = ref;
  if (!mItems.isEmpty())
    emit dataChanged(index(0), index(mItems.size() - 1), {CurrentRole});
}

void RefsModel::setFilter(const QString &filter) {
  QString simplified = filter.simplified();
  if (simplified == mFilter)
    return;

  mFilter = simplified;
  update();
}

void RefsModel::toggle(int row) {
  if (row < 0 || row >= mItems.size() || mItems.at(row).key.isEmpty())
    return;

  QString key = mItems.at(row).key;
  if (mCollapsed.contains(key)) {
    mCollapsed.remove(key);
  } else {
    mCollapsed.insert(key);
  }

  QSettings().setValue(kCollapsedKey, QStringList(mCollapsed.values()));
  update();
}

bool RefsModel::isExpanded(const QString &key) const {
  // Show everything that matches while filtering.
  return !mFilter.isEmpty() || !mCollapsed.contains(key);
}

bool RefsModel::matches(const QString &name) const {
  return mFilter.isEmpty() || name.contains(mFilter, Qt::CaseInsensitive);
}

void RefsModel::addHeader(Section section, const QString &name, int count) {
  Item header;
  header.kind = Header;
  header.section = section;
  header.name = name;
  header.count = count;
  header.key = QString("section/%1").arg(section);
  mItems.append(header);
}

void RefsModel::update() {
  beginResetModel();
  mItems.clear();

  // Refresh the current reference; its target may have moved.
  if (mCurrent.isValid())
    mCurrent = mRepo.lookupRef(mCurrent.qualifiedName());

  // local branches
  QList<Item> local;
  git::Reference head = mRepo.head();
  if (head.isValid() && !head.isBranch() && matches(head.name())) {
    Item item;
    item.kind = Branch;
    item.section = Local;
    item.name = head.name();
    item.depth = 1;
    item.ref = head;
    item.head = true;
    local.append(item);
  }

  QList<Item> branches;
  for (const git::Branch &branch : mRepo.branches(GIT_BRANCH_LOCAL)) {
    if (!matches(branch.name()))
      continue;

    Item item;
    item.kind = Branch;
    item.section = Local;
    item.name = branch.name();
    item.depth = 1;
    item.ref = branch;
    item.head = branch.isHead();
    if (git::Branch upstream = branch.upstream()) {
      item.ahead = branch.difference(upstream);
      item.behind = upstream.difference(branch);
    }
    branches.append(item);
  }

  std::sort(branches.begin(), branches.end(), lessThan);
  local.append(branches);

  addHeader(Local, tr("Local"), local.size());
  if (isExpanded(mItems.last().key))
    mItems.append(local);

  // remote branches grouped by remote
  QMap<QString, QList<Item>> remotes;
  for (const git::Branch &branch : mRepo.branches(GIT_BRANCH_REMOTE)) {
    QString name = branch.name();
    if (name.endsWith("/HEAD"))
      continue;

    QString remote = name.section('/', 0, 0);
    remotes[remote]; // Show remotes without matching branches too.
    if (!matches(name))
      continue;

    Item item;
    item.kind = RemoteBranch;
    item.section = Remote;
    item.name = name.section('/', 1);
    item.depth = 2;
    item.ref = branch;
    remotes[remote].append(item);
  }

  // Include remotes that don't have any branches yet.
  for (const git::Remote &remote : mRepo.remotes())
    remotes[remote.name()];

  if (!mFilter.isEmpty()) {
    for (auto it = remotes.begin(); it != remotes.end();) {
      it = it.value().isEmpty() ? remotes.erase(it) : std::next(it);
    }
  }

  addHeader(Remote, tr("Remote"), remotes.size());
  if (isExpanded(mItems.last().key)) {
    for (auto it = remotes.begin(); it != remotes.end(); ++it) {
      Item group;
      group.kind = RemoteGroup;
      group.section = Remote;
      group.name = it.key();
      group.depth = 1;
      group.count = it.value().size();
      group.key = QString("remote/%1").arg(it.key());
      mItems.append(group);

      if (isExpanded(group.key)) {
        std::sort(it.value().begin(), it.value().end(), lessThan);
        mItems.append(it.value());
      }
    }
  }

  // Open pull requests, like GitKraken lists them.
  if (mPullRequests && mPullRequests->isSupported()) {
    QList<Item> pulls;
    for (const ::PullRequest &pr : mPullRequests->pullRequests()) {
      QString name = QString("#%1 %2").arg(pr.number).arg(pr.title);
      if (!matches(name))
        continue;

      Item item;
      item.kind = PullRequest;
      item.section = PullRequests;
      item.name = name;
      item.depth = 1;
      item.index = pr.number;
      item.tip = tr("%1 wants to merge %2 into %3")
                     .arg(pr.author, pr.head, pr.base);
      pulls.append(item);
    }

    addHeader(PullRequests, tr("Pull Requests"), pulls.size());
    if (isExpanded(mItems.last().key)) {
      mItems.append(pulls);

      // Say why there are none.
      QString text;
      if (!mPullRequests->error().isEmpty()) {
        text = mPullRequests->error();
      } else if (pulls.isEmpty() && mPullRequests->isLoading()) {
        text = tr("Loading...");
      } else if (pulls.isEmpty() && mFilter.isEmpty()) {
        text = tr("No open pull requests");
      }

      if (!text.isEmpty()) {
        Item item;
        item.kind = Empty;
        item.section = PullRequests;
        item.name = text;
        item.tip = text;
        item.depth = 1;
        mItems.append(item);
      }
    }
  }

  // tags
  QList<Item> tags;
  for (const git::Reference &ref : mRepo.refs()) {
    if (!ref.isTag() || !matches(ref.name()))
      continue;

    Item item;
    item.kind = Tag;
    item.section = Tags;
    item.name = ref.name();
    item.depth = 1;
    item.ref = ref;
    tags.append(item);
  }

  std::sort(tags.begin(), tags.end(), lessThan);
  addHeader(Tags, tr("Tags"), tags.size());
  if (isExpanded(mItems.last().key))
    mItems.append(tags);

  // stashes
  QList<Item> stashes;
  git::Reference stashRef = mRepo.stashRef();
  QList<git::Commit> stashCommits = mRepo.stashes();
  for (int i = 0; i < stashCommits.size(); ++i) {
    QString summary = stashCommits.at(i).summary();
    if (!matches(summary))
      continue;

    Item item;
    item.kind = Stash;
    item.section = Stashes;
    item.name = summary;
    item.depth = 1;
    item.ref = stashRef;
    item.index = i;
    stashes.append(item);
  }

  addHeader(Stashes, tr("Stashes"), stashes.size());
  if (isExpanded(mItems.last().key))
    mItems.append(stashes);

  // submodules
  QList<Item> submodules;
  QList<git::Submodule> modules = mRepo.submodules();
  for (int i = 0; i < modules.size(); ++i) {
    QString name = modules.at(i).name();
    if (!matches(name))
      continue;

    Item item;
    item.kind = Submodule;
    item.section = Submodules;
    item.name = name;
    item.depth = 1;
    item.index = i;
    submodules.append(item);
  }

  addHeader(Submodules, tr("Submodules"), submodules.size());
  if (isExpanded(mItems.last().key))
    mItems.append(submodules);

  endResetModel();
}

int RefsModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : mItems.size();
}

QVariant RefsModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= mItems.size())
    return QVariant();

  const Item &item = mItems.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
      return item.name;
    case Qt::ToolTipRole:
      if (!item.tip.isEmpty())
        return item.tip;
      return item.ref.isValid() ? item.ref.qualifiedName() : item.name;
    case KindRole:
      return item.kind;
    case SectionRole:
      return item.section;
    case DepthRole:
      return item.depth;
    case HeadRole:
      return item.head;
    case CurrentRole:
      return item.kind != Stash && item.ref.isValid() && mCurrent.isValid() &&
             item.ref.qualifiedName() == mCurrent.qualifiedName();
    case AheadRole:
      return item.ahead;
    case BehindRole:
      return item.behind;
    case CountRole:
      return item.count;
    case ExpandedRole:
      return item.key.isEmpty() || isExpanded(item.key);
    case ExpandableRole:
      return !item.key.isEmpty();
    case SoloRole:
      return (item.kind == Branch || item.kind == RemoteBranch) &&
             item.ref.isValid() && mSolo.contains(item.ref.qualifiedName());
    case HiddenRole: {
      QString name;
      if (item.kind == RemoteGroup) {
        name = QString("refs/remotes/%1/").arg(item.name);
      } else if ((item.kind == Branch || item.kind == RemoteBranch) &&
                 item.ref.isValid()) {
        name = item.ref.qualifiedName();
      } else {
        return false;
      }

      for (const QString &entry : mHidden) {
        if (entry == name || (entry.endsWith('/') && name.startsWith(entry)))
          return true;
      }
      return false;
    }
    case NumberRole:
      return item.kind == PullRequest ? item.index : 0;
    case RefNameRole:
      if (item.kind == RemoteGroup)
        return QString("remote:%1").arg(item.name);
      if (item.kind == Stash || !item.ref.isValid())
        return QString();
      return item.ref.qualifiedName();
  }

  return QVariant();
}

void RefsModel::setPullRequests(PullRequestList *pullRequests) {
  mPullRequests = pullRequests;
  connect(pullRequests, &PullRequestList::changed, this, &RefsModel::update);
  update();
}

void RefsModel::setHidden(const QStringList &hidden) {
  if (hidden == mHidden)
    return;

  mHidden = hidden;
  if (!mItems.isEmpty())
    emit dataChanged(index(0), index(mItems.size() - 1), {HiddenRole});
}

void RefsModel::setSolo(const QStringList &solo) {
  if (solo == mSolo)
    return;

  mSolo = solo;
  if (!mItems.isEmpty())
    emit dataChanged(index(0), index(mItems.size() - 1), {SoloRole});
}

QHash<int, QByteArray> RefsModel::roleNames() const {
  return {{Qt::ToolTipRole, "toolTip"}, {KindRole, "kind"},
          {SectionRole, "section"},     {NameRole, "name"},
          {DepthRole, "depth"},         {HeadRole, "isHead"},
          {CurrentRole, "isCurrent"},   {AheadRole, "ahead"},
          {BehindRole, "behind"},       {CountRole, "count"},
          {ExpandedRole, "expanded"},   {ExpandableRole, "expandable"},
          {SoloRole, "soloed"},         {RefNameRole, "refName"},
          {HiddenRole, "hidden"},       {NumberRole, "number"}};
}

RefsPanel::RefsPanel(const git::Repository &repo, RepoView *view)
    : QObject(view), mView(view), mRepo(repo),
      mModel(new RefsModel(repo, this)) {
  // Start at HEAD without reloading the graph.
  mModel->setCurrent(repo.head());
  mEmitted = currentReference();

  auto update = [this] {
    bool valid = mModel->current().isValid();
    mModel->update();

    // Fall back to HEAD when the current reference was removed.
    if (valid && !mModel->current().isValid())
      select(mRepo.head());
  };

  git::RepositoryNotifier *notifier = repo.notifier();
  connect(notifier, &git::RepositoryNotifier::referenceAdded, this, update);
  connect(notifier, &git::RepositoryNotifier::referenceRemoved, this, update);
  connect(notifier, &git::RepositoryNotifier::referenceUpdated, this, update);
}

git::Reference RefsPanel::currentReference() const {
  git::Reference ref = mModel->current();
  if (!ref.isValid() || ref.isStash())
    return ref.isValid() ? ref : mRepo.head();

  // All branches are shown anyway, so keep the uncommitted changes row
  // unless the graph is limited to the selected branch.
  int filter = mRepo.appConfig().value<int>(
      ConfigKeys::kRefsKey, static_cast<int>(CommitList::RefsFilter::AllRefs));
  if (filter == static_cast<int>(CommitList::RefsFilter::AllRefs))
    return mRepo.head();

  return ref;
}

void RefsPanel::select(const git::Reference &ref, bool suppress) {
  mModel->setCurrent(ref);
  mEmitted = currentReference();
  if (!suppress)
    emit referenceChanged(mEmitted);
}

void RefsPanel::setCurrent(const git::Reference &ref) {
  mModel->setCurrent(ref);

  // Only reload the graph when it would show something different.
  git::Reference current = currentReference();
  bool changed = (current.isValid() != mEmitted.isValid()) ||
                 (current.isValid() &&
                  current.qualifiedName() != mEmitted.qualifiedName());
  mEmitted = current;
  if (changed)
    emit referenceChanged(current);
}

void RefsPanel::activate(int row) {
  if (row < 0 || row >= mModel->rowCount())
    return;

  const RefsModel::Item &item = mModel->item(row);
  switch (item.kind) {
    case RefsModel::Header:
    case RefsModel::RemoteGroup:
      toggle(row);
      break;

    case RefsModel::Branch:
    case RefsModel::RemoteBranch:
    case RefsModel::Tag: {
      // Show the graph again.
      mView->pullRequestList()->close();
      git::Reference ref = item.ref;
      setCurrent(ref);
      emit referenceSelected(ref);
      break;
    }

    case RefsModel::Stash: {
      int index = item.index;
      setCurrent(item.ref);
      emit stashSelected(index);
      break;
    }

    case RefsModel::PullRequest:
      mView->pullRequestList()->show(item.index);
      break;

    default:
      break;
  }
}

void RefsPanel::open(int row) {
  if (row < 0 || row >= mModel->rowCount())
    return;

  const RefsModel::Item &item = mModel->item(row);
  switch (item.kind) {
    case RefsModel::Branch:
    case RefsModel::RemoteBranch:
    case RefsModel::Tag:
      if (!item.head && !mRepo.isBare())
        mView->checkout(item.ref);
      break;

    case RefsModel::Submodule: {
      QList<git::Submodule> submodules = mRepo.submodules();
      if (item.index < submodules.size())
        mView->openSubmodule(submodules.at(item.index));
      break;
    }

    default:
      break;
  }
}

void RefsPanel::toggle(int row) { mModel->toggle(row); }

void RefsPanel::setFilter(const QString &filter) { mModel->setFilter(filter); }

void RefsPanel::setSolo(const QStringList &solo) {
  mModel->setSolo(solo);
  if (solo.isEmpty() == !mSoloActive)
    return;

  mSoloActive = !solo.isEmpty();
  emit soloChanged();
}

void RefsPanel::toggleSolo(int row) {
  if (row < 0 || row >= mModel->rowCount())
    return;

  const RefsModel::Item &item = mModel->item(row);
  if ((item.kind != RefsModel::Branch && item.kind != RefsModel::RemoteBranch) ||
      !item.ref.isValid())
    return;

  CommitList *commits = mView->commitList();
  QString name = item.ref.qualifiedName();
  commits->setSoloed(name, !commits->isSoloed(name));
}

void RefsPanel::toggleHidden(int row) {
  if (row < 0 || row >= mModel->rowCount())
    return;

  const RefsModel::Item &item = mModel->item(row);
  QString name;
  if (item.kind == RefsModel::RemoteGroup) {
    name = QString("refs/remotes/%1/").arg(item.name);
  } else if ((item.kind == RefsModel::Branch ||
              item.kind == RefsModel::RemoteBranch) &&
             item.ref.isValid()) {
    name = item.ref.qualifiedName();
  } else {
    return;
  }

  CommitList *commits = mView->commitList();
  if (!commits->isHidden(name)) {
    commits->setHidden(name, true);
    return;
  }

  // Show a branch of a hidden remote by showing the remote.
  if (!commits->hidden().contains(name) && item.ref.isRemoteBranch())
    name = QString("refs/remotes/%1/").arg(item.ref.name().section('/', 0, 0));
  commits->setHidden(name, false);
}

void RefsPanel::showContextMenu(int row, qreal x, qreal y) {
  if (row < 0 || row >= mModel->rowCount())
    return;

  RepoView *view = mView;
  const RefsModel::Item item = mModel->item(row);
  git::Reference ref = item.ref;

  QMenu menu;
  switch (item.kind) {
    case RefsModel::RemoteGroup: {
      git::Remote remote = mRepo.lookupRemote(item.name);
      if (!remote.isValid())
        return;

      menu.addAction(tr("Fetch %1").arg(item.name),
                     [view, remote] { view->fetch(remote); });

      // Hide the branches of the remote in the graph, like GitKraken.
      CommitList *commits = view->commitList();
      QString name = QString("refs/remotes/%1/").arg(item.name);
      bool hidden = commits->isHidden(name);
      menu.addAction(hidden ? tr("Show in Graph") : tr("Hide in Graph"),
                     [commits, name, hidden] {
                       commits->setHidden(name, !hidden);
                     });
      if (!commits->hidden().isEmpty())
        menu.addAction(tr("Show All Hidden Branches"),
                       [commits] { commits->showAll(); });
      menu.addSeparator();

      menu.addAction(tr("Edit Remotes..."), [view] {
        view->configureSettings(ConfigDialog::Remotes);
      });
      break;
    }

    case RefsModel::Header:
      if (item.section != RefsModel::PullRequests)
        return;
      menu.addAction(tr("Refresh Pull Requests"),
                     [view] { view->pullRequestList()->refresh(); });
      break;

    case RefsModel::PullRequest: {
      PullRequestList *pulls = view->pullRequestList();
      int number = item.index;
      menu.addAction(tr("Show"), [pulls, number] { pulls->show(number); });
      menu.addAction(tr("Open in Browser"), [pulls, number] {
        pulls->show(number);
        pulls->openInBrowser();
      });
      menu.addSeparator();
      menu.addAction(tr("Refresh Pull Requests"),
                     [pulls] { pulls->refresh(); });
      break;
    }

    case RefsModel::Stash: {
      int index = item.index;
      menu.addAction(tr("Apply"), [view, index] { view->applyStash(index); });
      menu.addAction(tr("Pop"), [view, index] { view->popStash(index); });
      menu.addAction(tr("Drop"), [view, index] { view->dropStash(index); });
      break;
    }

    case RefsModel::Submodule: {
      QList<git::Submodule> submodules = mRepo.submodules();
      if (item.index >= submodules.size())
        return;

      git::Submodule submodule = submodules.at(item.index);
      menu.addAction(tr("Open"),
                     [view, submodule] { view->openSubmodule(submodule); });
      menu.addAction(tr("Update"), [view, submodule] {
        view->updateSubmodules({submodule});
      });
      break;
    }

    case RefsModel::Branch:
    case RefsModel::RemoteBranch:
    case RefsModel::Tag: {
      // Show the branch alone in the graph, like GitKraken.
      if (item.kind != RefsModel::Tag) {
        CommitList *commits = view->commitList();
        QString name = ref.qualifiedName();
        bool soloed = commits->isSoloed(name);
        menu.addAction(soloed ? tr("Unsolo") : tr("Solo"),
                       [commits, name, soloed] {
                         commits->setSoloed(name, !soloed);
                       });

        if (!commits->solo().isEmpty())
          menu.addAction(tr("Unsolo All"), [commits] { commits->unsoloAll(); });

        // Hide the branch in the graph, like GitKraken.
        if (commits->canHide(name)) {
          bool hidden = commits->isHidden(name);
          menu.addAction(hidden ? tr("Show in Graph") : tr("Hide in Graph"),
                         [this, row] { toggleHidden(row); });
        }

        if (!commits->hidden().isEmpty())
          menu.addAction(tr("Show All Hidden Branches"),
                         [commits] { commits->showAll(); });

        menu.addSeparator();
      }

      QAction *checkout =
          menu.addAction(tr("Checkout"), [view, ref] { view->checkout(ref); });
      checkout->setEnabled(!item.head && !mRepo.isBare());

      menu.addSeparator();

      if (ref.isLocalBranch()) {
        QAction *rename = menu.addAction(tr("Rename..."), [view, ref] {
          view->promptToRenameBranch(ref);
        });
        rename->setEnabled(!ref.isHead());

        // Pull the checked out branch, or fast-forward another branch.
        QAction *pull =
            menu.addAction(tr("Pull"), [view, ref] { view->pullBranch(ref); });
        pull->setEnabled(git::Branch(ref).upstream().isValid() &&
                         !(ref.isHead() && mRepo.isBare()));

        QAction *push = menu.addAction(tr("Push"), [view, ref] {
          view->push(git::Remote(), ref);
        });
        push->setEnabled(ref.isHead() || git::Branch(ref).upstream());
      }

      if (ref.isTag() || ref.isLocalBranch()) {
        QAction *remove = menu.addAction(tr("Delete..."), [view, ref] {
          if (ref.isTag()) {
            view->promptToDeleteTag(ref);
          } else {
            view->promptToDeleteBranch(ref);
          }
        });
        remove->setEnabled(ref.isTag() || !ref.isHead());
      }

      if (ref.isTag()) {
        git::Remote remote = mRepo.defaultRemote();
        if (remote.isValid()) {
          menu.addAction(tr("Push Tag to %1").arg(remote.name()),
                         [ref, view, remote] { view->push(remote, ref); });
        }
      }

      if (ref.isRemoteBranch()) {
        menu.addAction(tr("New Local Branch"), [view, ref] {
          QString local = ref.name().section('/', 1);
          view->createBranch(local, ref.target(), ref, true);
        });
      }

      if (item.kind == RefsModel::Branch || item.kind == RefsModel::Tag ||
          item.kind == RefsModel::RemoteBranch) {
        menu.addAction(tr("New Branch Here..."), [view, ref] {
          view->promptToCreateBranch(ref.target());
        });
      }

      menu.addSeparator();

      auto addMergeAction = [&menu, view, ref](const QString &text,
                                               RepoView::MergeFlag flag) {
        QAction *action = menu.addAction(text, [view, ref, flag] {
          MergeDialog *dialog = new MergeDialog(flag, view->repo(), view);
          connect(dialog, &QDialog::accepted, [view, dialog] {
            view->merge(dialog->flags(), dialog->reference());
          });

          dialog->setReference(ref);
          dialog->open();
        });
        action->setEnabled(!ref.isHead());
      };

      addMergeAction(tr("Merge into Current Branch..."), RepoView::Merge);
      addMergeAction(tr("Rebase Current Branch onto This..."),
                     RepoView::Rebase);

      // Rebase the current branch interactively, like GitKraken.
      git::Reference head = mRepo.head();
      QAction *interactive = menu.addAction(
          tr("Interactive Rebase Current Branch onto This..."),
          [view, head, ref] {
            view->interactiveRebase()->open(head.qualifiedName(), ref.target(),
                                            ref.name());
          });
      interactive->setEnabled(
          head.isValid() && head.isLocalBranch() &&
          view->interactiveRebase()->canOpen(head.qualifiedName(),
                                             ref.target()));
      addMergeAction(tr("Squash into Current Branch..."), RepoView::Squash);
      break;
    }

    default:
      return;
  }

  QmlSupport::execMenu(&menu, mView->mapFromPage(x, y));
}

void RefsPanel::add(int section) {
  switch (static_cast<RefsModel::Section>(section)) {
    case RefsModel::Local:
      mView->promptToCreateBranch();
      break;
    case RefsModel::Remote:
      mView->configureSettings(ConfigDialog::Remotes);
      break;
    case RefsModel::Tags:
      if (git::Reference head = mRepo.head())
        mView->promptToAddTag(head.target());
      break;
    case RefsModel::Stashes:
      mView->promptToStash();
      break;
    case RefsModel::Submodules:
      mView->configureSettings(ConfigDialog::Submodules);
      break;
    case RefsModel::PullRequests:
      (new PullRequestDialog(mView))->open();
      break;
  }
}
