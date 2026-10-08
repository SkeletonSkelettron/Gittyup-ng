//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "ChangedFilesModel.h"
#include "git/Index.h"
#include <QMap>
#include <QSharedPointer>
#include <functional>

ChangedFilesModel::ChangedFilesModel(Filter filter, QObject *parent)
    : QAbstractListModel(parent), mFilter(filter) {}

void ChangedFilesModel::setDiff(const git::Diff &diff) {
  mDiff = diff;
  rebuild();
}

void ChangedFilesModel::setListMode(bool list) {
  if (list == mList)
    return;

  mList = list;
  rebuild();
}

void ChangedFilesModel::setHideUntracked(bool hide) {
  if (hide == mHideUntracked)
    return;

  mHideUntracked = hide;
  rebuild();
}

void ChangedFilesModel::refresh() { rebuild(); }

bool ChangedFilesModel::accepts(int index) const {
  if (mFilter == All || !mDiff.isStatusDiff())
    return mFilter != Staged;

  git::Index::StagedState state = mDiff.index().isStaged(mDiff.name(index));
  if (mFilter == Staged)
    return state == git::Index::Staged || state == git::Index::PartiallyStaged;

  if (mHideUntracked && mDiff.status(index) == GIT_DELTA_UNTRACKED)
    return false;

  return state != git::Index::Staged;
}

void ChangedFilesModel::rebuild() {
  beginResetModel();
  mItems.clear();

  QList<Item> files;
  git::Index index = mDiff.isValid() ? mDiff.index() : git::Index();
  int count = mDiff.isValid() ? mDiff.count() : 0;
  for (int i = 0; i < count; ++i) {
    if (!accepts(i))
      continue;

    Item item;
    item.path = mDiff.name(i);
    int slash = item.path.lastIndexOf('/');
    item.name = item.path.mid(slash + 1);
    item.dir = (slash >= 0) ? item.path.left(slash) : QString();
    item.status = mDiff.isStatusDiff() && index.isValid() &&
                          index.isStaged(item.path) == git::Index::Conflicted
                      ? QChar('!')
                      : QChar(git::Diff::statusChar(mDiff.status(i)));
    item.state = index.isValid() && mDiff.isStatusDiff()
                     ? index.isStaged(item.path)
                     : git::Index::Disabled;
    files.append(item);
  }

  mFileCount = files.size();

  if (mList) {
    std::sort(files.begin(), files.end(), [](const Item &lhs, const Item &rhs) {
      return lhs.path.compare(rhs.path, Qt::CaseInsensitive) < 0;
    });
    mItems = files;
  } else {
    // Build a tree of directories. Directories come before files.
    struct Node {
      QMap<QString, QSharedPointer<Node>> dirs;
      QList<Item> files;
    };

    QSharedPointer<Node> root(new Node);
    for (const Item &file : files) {
      Node *node = root.data();
      if (!file.dir.isEmpty()) {
        for (const QString &part : file.dir.split('/')) {
          QSharedPointer<Node> &child = node->dirs[part];
          if (!child)
            child.reset(new Node);
          node = child.data();
        }
      }
      node->files.append(file);
    }

    std::function<void(const Node &, const QString &, int)> flatten =
        [this, &flatten](const Node &node, const QString &prefix, int depth) {
          for (auto it = node.dirs.cbegin(); it != node.dirs.cend(); ++it) {
            // Collapse chains of single directories into one row.
            QString name = it.key();
            const Node *child = it.value().data();
            while (child->files.isEmpty() && child->dirs.size() == 1) {
              name += '/' + child->dirs.firstKey();
              child = child->dirs.first().data();
            }

            Item dir;
            dir.isDir = true;
            dir.name = name;
            dir.path = prefix.isEmpty() ? name : prefix + '/' + name;
            dir.depth = depth;
            mItems.append(dir);

            if (!mCollapsed.contains(dir.path))
              flatten(*child, dir.path, depth + 1);
          }

          QList<Item> sorted = node.files;
          std::sort(sorted.begin(), sorted.end(),
                    [](const Item &lhs, const Item &rhs) {
                      return lhs.name.compare(rhs.name, Qt::CaseInsensitive) <
                             0;
                    });
          for (Item file : sorted) {
            file.depth = depth;
            mItems.append(file);
          }
        };

    flatten(*root, QString(), 0);
  }

  endResetModel();
  emit fileCountChanged();
}

QString ChangedFilesModel::path(int row) const {
  return (row >= 0 && row < mItems.size()) ? mItems.at(row).path : QString();
}

bool ChangedFilesModel::isDir(int row) const {
  return row >= 0 && row < mItems.size() && mItems.at(row).isDir;
}

QStringList ChangedFilesModel::files(int row) const {
  if (row < 0 || row >= mItems.size())
    return QStringList();

  const Item &item = mItems.at(row);
  if (!item.isDir)
    return {item.path};

  // All files of the diff below the directory, including collapsed ones.
  QStringList result;
  QString prefix = item.path + '/';
  for (int i = 0; mDiff.isValid() && i < mDiff.count(); ++i) {
    QString name = mDiff.name(i);
    if (name.startsWith(prefix) && accepts(i))
      result.append(name);
  }

  return result;
}

QStringList ChangedFilesModel::allFiles() const {
  QStringList result;
  for (int i = 0; mDiff.isValid() && i < mDiff.count(); ++i) {
    if (accepts(i))
      result.append(mDiff.name(i));
  }

  return result;
}

int ChangedFilesModel::rowOf(const QString &path) const {
  for (int i = 0; i < mItems.size(); ++i) {
    if (!mItems.at(i).isDir && mItems.at(i).path == path)
      return i;
  }

  return -1;
}

void ChangedFilesModel::toggle(int row) {
  if (!isDir(row))
    return;

  QString path = mItems.at(row).path;
  if (mCollapsed.contains(path)) {
    mCollapsed.remove(path);
  } else {
    mCollapsed.insert(path);
  }

  rebuild();
}

int ChangedFilesModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : mItems.size();
}

QVariant ChangedFilesModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= mItems.size())
    return QVariant();

  const Item &item = mItems.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
      return item.name;
    case Qt::ToolTipRole:
    case PathRole:
      return item.path;
    case DirRole:
      return item.dir;
    case StatusRole:
      return QString(item.status);
    case StageStateRole:
      return item.state;
    case DepthRole:
      return item.depth;
    case IsDirRole:
      return item.isDir;
    case ExpandedRole:
      return item.isDir && !mCollapsed.contains(item.path);
  }

  return QVariant();
}

QHash<int, QByteArray> ChangedFilesModel::roleNames() const {
  return {{PathRole, "path"},       {NameRole, "name"},
          {DirRole, "dir"},         {StatusRole, "status"},
          {StageStateRole, "stageState"}, {DepthRole, "depth"},
          {IsDirRole, "isDir"},     {ExpandedRole, "expanded"}};
}
