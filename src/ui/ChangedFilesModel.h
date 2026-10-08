//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef CHANGEDFILESMODEL_H
#define CHANGEDFILESMODEL_H

#include "git/Diff.h"
#include <QAbstractListModel>
#include <QSet>

// The files of a diff as a flat list or a tree with collapsible directories.
// For the uncommitted changes, the staged and the unstaged files are listed
// by separate models.
class ChangedFilesModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(int fileCount READ fileCount NOTIFY fileCountChanged)

public:
  enum Filter { All, Staged, Unstaged };

  enum Role {
    PathRole = Qt::UserRole,
    NameRole,
    DirRole,
    StatusRole,
    StageStateRole,
    DepthRole,
    IsDirRole,
    ExpandedRole
  };

  ChangedFilesModel(Filter filter, QObject *parent = nullptr);

  void setDiff(const git::Diff &diff);
  void setListMode(bool list);
  void setHideUntracked(bool hide);

  // Reload the stage state of the files.
  void refresh();

  int fileCount() const { return mFileCount; }

  QString path(int row) const;
  bool isDir(int row) const;
  // The files at or below the row.
  QStringList files(int row) const;
  QStringList allFiles() const;
  int rowOf(const QString &path) const;

  void toggle(int row);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

signals:
  void fileCountChanged();

private:
  struct Item {
    QString path;
    QString name;
    QString dir;
    QChar status;
    int state = 0;
    int depth = 0;
    bool isDir = false;
  };

  void rebuild();
  bool accepts(int index) const;

  Filter mFilter;
  git::Diff mDiff;
  bool mList = true;
  bool mHideUntracked = false;
  QSet<QString> mCollapsed;
  QList<Item> mItems;
  int mFileCount = 0;
};

#endif
