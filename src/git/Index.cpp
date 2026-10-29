//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Index.h"
#include "Commit.h"
#include "Reference.h"
#include "Repository.h"
#include "Result.h"
#include "Signature.h"
#include "Submodule.h"
#include "Tree.h"
#include "git/Cli.h"
#include "git/RunResult.h"
#include "git2/repository.h"
#include "git2/tree.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

namespace git {

namespace {

void countDirectoryEntries(const QString &file, int &count) {
  QDir dir(file);
  auto filters = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot;
  for (const QString &entry : dir.entryList(filters)) {
    QString file = dir.filePath(entry);
    if (QFileInfo(file).isDir()) {
      countDirectoryEntries(file, count);
    } else {
      // Cut off at 100 entries.
      count = (count == 100) ? -1 : count + 1;
    }

    if (count < 0)
      return;
  }
}

} // namespace

Index::Data::Data(git_index *index) : index(index) {}

Index::Data::~Data() { git_index_free(index); }

Index::Index() {}

Index::Index(git_index *index) : d(index ? new Data(index) : nullptr) {}

Index::operator git_index *() const { return isValid() ? d->index : nullptr; }

Index::Conflict Index::conflict(const QString &path) const {
  const git_index_entry *ancestor, *ours, *theirs;
  if (git_index_conflict_get(&ancestor, &ours, &theirs, d->index,
                             path.toUtf8()))
    return Conflict();

  Id ancestorId = ancestor ? ancestor->id : Id();
  Id oursId = ours ? ours->id : Id();
  Id theirsId = theirs ? theirs->id : Id();
  return {ancestorId, oursId, theirsId};
}

bool Index::takeSide(const QStringList &paths, Side side) {
  Repository repo(git_index_owner(d->index));
  RepositoryNotifier *notifier = repo.notifier();
  QString action =
      (side == Ours) ? tr("take our version") : tr("take their version");

  // The files that the side kept, and the ones it deleted.
  QStringList kept;
  QStringList deleted;
  for (const QString &path : paths) {
    Conflict conflict = this->conflict(path);
    Id id = (side == Ours) ? conflict.ours : conflict.theirs;
    (id.isNull() ? deleted : kept).append(path);
  }

  // Run git with the paths on stdin, like setStaged.
  Cli cli(repo.workdir().path());
  auto run = [&](const QStringList &command, const QStringList &files) {
    QByteArray input;
    for (const QString &file : files) {
      input += file.toUtf8();
      input += '\0';
    }

    QStringList args = {"--literal-pathspecs", "-c", "core.splitindex=false"};
    args += command;
    args += {"--pathspec-from-file=-", "--pathspec-file-nul"};
    RunResult result = cli.run(args, input);
    if (result.exitCode != 0)
      emit notifier->gitCommandFailed(action, QString::fromUtf8(result.errors));
    return result.exitCode == 0;
  };

  // Write the version of the side to the workdir.
  QString option = (side == Ours) ? "--ours" : "--theirs";
  if (!kept.isEmpty() && !run({"checkout", option}, kept))
    return false;

  // Delete the files that the side deleted, which resolves them.
  if (!deleted.isEmpty()) {
    if (!run({"rm", "-q"}, deleted))
      return false;

    git_index_read(d->index, true);
    for (const QString &path : deleted)
      d->stagedCache.remove(path);
    emit notifier->indexChanged(deleted);
  }

  // Staging the version resolves the conflict.
  if (!kept.isEmpty())
    setStaged(kept, true);

  return true;
}

git_filemode_t Index::mode(const QString &path) const {
  const git_index_entry *entry = this->entry(path);
  if (!entry)
    return GIT_FILEMODE_UNREADABLE;

  return static_cast<git_filemode_t>(entry->mode);
}

void Index::setMode(const QString &path, git_filemode_t mode) {
  if (!entry(path))
    return;

  QString chmod =
      mode == GIT_FILEMODE_BLOB_EXECUTABLE ? "--chmod=+x" : "--chmod=-x";
  QStringList args = {"update-index", chmod, "--", path};
  Repository repo(git_index_owner(d->index));

  git::Cli cli(repo.workdir().path());

  RunResult result = cli.run(args);

  if (result.exitCode != 0) {
    emit repo.notifier()->gitCommandFailed(tr("set mode"),
                                           QString::fromUtf8(result.errors));
    return;
  }

  git_index_read(d->index, true);
  d->stagedCache.remove(path);
  emit repo.notifier()->indexChanged({path});
}

bool Index::isTracked(const QString &path) const { return entry(path); }

Index::StagedState Index::isStaged(const QString &path) const {
  QMap<QString, StagedState>::const_iterator it = d->stagedCache.find(path);
  if (it != d->stagedCache.end())
    return it.value();

  Repository repo(git_index_owner(d->index));
  Submodule sm = repo.lookupSubmodule(path);

  // Handle untracked directories.
  QFileInfo info(repo.workdir().filePath(path));
  if (!sm.isValid() && !info.isSymLink() && info.isDir())
    return d->stagedCache.insert(path, Unstaged).value();

  uint32_t headMode = GIT_FILEMODE_UNREADABLE;
  uint32_t indexMode = GIT_FILEMODE_UNREADABLE;
  Id head = sm.isValid() ? sm.headId() : headId(path, &headMode);
  Id index = sm.isValid() ? sm.indexId() : indexId(path, &indexMode);

  // Handle untracked files.
  if (!head.isValid() && !index.isValid())
    return d->stagedCache.insert(path, Unstaged).value();

  uint32_t workdirMode = GIT_FILEMODE_UNREADABLE;
  std::optional<Id> workdirOpt =
      sm.isValid() ? sm.workdirId() : workdirId(path, &workdirMode);

  // Handle filter callback error.
  if (!workdirOpt.has_value())
    return d->stagedCache.insert(path, Unstaged).value();

  Id workdir = workdirOpt.value();

  // Handle dirty submodules.
  if (sm.isValid() && head == workdir)
    return d->stagedCache.insert(path, Disabled).value();

  if (index == workdir && indexMode == workdirMode)
    return d->stagedCache.insert(path, Staged).value();

  if (head == index && headMode == indexMode)
    return d->stagedCache.insert(path, Unstaged).value();

  if (conflict(path).isValid())
    return d->stagedCache.insert(path, Conflicted).value();

  return d->stagedCache.insert(path, PartiallyStaged).value();
}

void Index::copyStagedStates(const Index &index) {
  if (isValid() && index.isValid() && d != index.d)
    d->stagedCache.insert(index.d->stagedCache);
}

void Index::setStaged(const QStringList &files, bool staged, bool yieldFocus) {
  bool dirAdded = false;
  QStringList changedFiles;
  Repository repo(git_index_owner(d->index));
  RepositoryNotifier *notifier = repo.notifier();

  QStringList pathList;
  QStringList subPathList;
  git::Cli cli(repo.workdir().path());
  for (const QString &file : files) {
    bool fileExists = false;
    if (staged)
      fileExists = repo.workdir().exists(file);

    // submodule
    if (repo.lookupSubmodule(file)) {
      subPathList.append(file);
      changedFiles.append(file);
      continue;
    }

    // regular file
    if (staged) {
      if (fileExists) {
        QDir dir = repo.workdir();
        QFileInfo info(dir.filePath(file));
        if (!info.isSymLink() && info.isDir()) {

          int count = 0;
          bool allow = true;
          countDirectoryEntries(dir.filePath(file), count);
          emit notifier->directoryAboutToBeStaged(file, count, allow);
          if (!allow)
            continue;
          pathList.append(file);
          dirAdded = true;
        } else {
          bool allow = false;
          if (repo.lfsIsInitialized()) {
            const auto value = repo.attributeValue("filter", file);
            if (value == "lfs") {
              allow = true;
            }
          }
          int size = QFileInfo(repo.workdir(), file).size();
          if (size <= 10000000) // 10MB
            allow = true;
          else if (!allow) {
            allow = true;
            emit notifier->largeFileAboutToBeStaged(file, size, allow);
          }

          if (!allow)
            continue;

          pathList.append(file);
        }

      } else {
        // This is staging delete file op
        pathList.append(file);
      }

    } else {
      pathList.append(file);
    }

    changedFiles.append(file);
  }

  if (!changedFiles.isEmpty()) {
    git_index_write(d->index);
    pathList += subPathList;
    if (!pathList.isEmpty()) {
      QByteArray input;
      for (const QString &file : pathList) {
        input += file.toUtf8();
        input += '\0';
      }
      const QStringList args{"--literal-pathspecs",
                             "-c",
                             "core.splitindex=false",
                             staged ? "add" : "reset",
                             staged ? "--ignore-errors" : "-q",
                             "--pathspec-from-file=-",
                             "--pathspec-file-nul"};
      RunResult result = cli.run(args, input);
      git_index_read(d->index, true);
      if (!subPathList.isEmpty())
        repo.invalidateSubmoduleCache();
      if (result.exitCode != 0)
        emit notifier->indexStageError(staged,
                                       QString::fromUtf8(result.errors));
    }
    for (const QString &changedFile : changedFiles)
      d->stagedCache.remove(changedFile);
    emit notifier->indexChanged(changedFiles, yieldFocus);
  }

  if (dirAdded)
    emit notifier->directoryStaged();
}

void Index::add(const QString &path, const QByteArray &buffer) {
  // The pointer dies when the index is read again below.
  const git_index_entry *entry = this->entry(path);
  uint32_t mode = entry ? entry->mode : GIT_FILEMODE_BLOB;

  Repository repo(git_index_owner(d->index));
  Cli cli(repo.workdir().path());

  // Write the staged lines as a blob.
  RunResult result = cli.run({"hash-object", "-w", "--stdin"}, buffer);
  if (result.exitCode != 0) {
    emit repo.notifier()->gitCommandFailed(tr("stage lines"),
                                           QString::fromUtf8(result.errors));
    return;
  }

  // Point the entry of the index to the blob.
  QString id = QString::fromUtf8(result.output).trimmed();
  result = cli.run({"update-index", "--add", "--cacheinfo",
                    QString::number(mode, 8), id, path});
  if (result.exitCode != 0) {
    emit repo.notifier()->gitCommandFailed(tr("stage lines"),
                                           QString::fromUtf8(result.errors));
    return;
  }

  // git changed the index file, read it again.
  git_index_read(d->index, true);
  d->stagedCache.remove(path);
  emit repo.notifier()->indexChanged({path});
}

void Index::read() { git_index_read(d->index, false); }

Tree Index::writeTree() const {
  // Write the index tree.
  git_oid id;
  if (git_index_write_tree(&id, d->index))
    return Tree();

  git_tree *tree = nullptr;
  git_repository *repo = git_index_owner(d->index);
  git_tree_lookup(&tree, repo, &id);
  return Tree(tree);
}

bool Index::hasConflicts() const { return git_index_has_conflicts(d->index); }

Index Index::reopen() const {
  // An index in memory has no file.
  const char *path = git_index_path(d->index);
  if (!path)
    return Index();

  git_index *index = nullptr;
#ifdef GIT_EXPERIMENTAL_SHA256
  git_index_options opts = GIT_INDEX_OPTIONS_INIT;
  if (git_repository *repo = git_index_owner(d->index))
    opts.oid_type = git_repository_oid_type(repo);
  if (git_index_open_ext(&index, path, &opts))
    return Index();
#else
  if (git_index_open(&index, path))
    return Index();
#endif

  // Compare paths like the index of the repository.
  git_index_set_caps(index, git_index_caps(d->index));
  return Index(index);
}

Index Index::create() {
  git_index *index = nullptr;
  git_index_new(&index);
  return Index(index);
}

const git_index_entry *Index::entry(const QString &path, int stage) const {
  return git_index_get_bypath(d->index, path.toUtf8(), stage);
}

Id Index::headId(const QString &path, uint32_t *mode) const {
  Repository repo(git_index_owner(d->index));
  Reference head = repo.head();
  if (!head.isValid())
    return Id();

  Commit commit = head.target();
  if (!head.isValid())
    return Id();

  git_tree_entry *entry = nullptr;
  const auto tree = commit.tree();
  if (!tree.isValid())
    return Id(); // broken repository?
  if (git_tree_entry_bypath(&entry, tree, path.toUtf8()))
    return Id();

  if (mode)
    *mode = git_tree_entry_filemode_raw(entry);

  Id id(git_tree_entry_id(entry));

  git_tree_entry_free(entry);

  return id;
}

Id Index::indexId(const QString &path, uint32_t *mode) const {
  const git_index_entry *entry = this->entry(path);
  if (!entry)
    return Id();

  if (mode)
    *mode = entry->mode;

  return entry->id;
}

std::optional<Id> Index::workdirId(const QString &path, uint32_t *mode) const {
  Repository repo(git_index_owner(d->index));

  git_oid id;
  if (int error = git_repository_hashfile(&id, repo, path.toUtf8(),
                                          GIT_OBJECT_BLOB, nullptr))
    return (error == GIT_EUSER) ? std::nullopt : std::optional<Id>(Id());

  if (mode) {
    *mode = GIT_FILEMODE_BLOB;
    QFileInfo info(repo.workdir().filePath(path));
#ifndef Q_OS_WIN
    if (info.isExecutable())
      *mode = GIT_FILEMODE_BLOB_EXECUTABLE;
#endif
    if (info.isSymLink())
      *mode = GIT_FILEMODE_LINK;
  }

  return id;
}

} // namespace git
