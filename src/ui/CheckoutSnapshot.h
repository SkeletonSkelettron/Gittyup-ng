//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef CHECKOUT_SNAPSHOT_H
#define CHECKOUT_SNAPSHOT_H

#include "git/Cli.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSet>
#include <QTemporaryDir>
#include <algorithm>
#include <filesystem>

namespace undo_detail {

// A failed reference update must restore the actual local state, including
// staged changes that already matched the checkout destination. Trees alone
// also lose unstaged deletions and the raw bytes produced by smudge filters.
class CheckoutSnapshot {
public:
  bool capture(const git::Repository &repo, const git::Commit &from,
               const git::Commit &to, QString &error) {
    mWorkdir = repo.workdir();
    if (!mBackup.isValid())
      return fail(error, QObject::tr("Unable to create an undo backup."));

    git::Cli cli(mWorkdir.path());
    git::RunResult result =
        cli.run({"rev-parse", "--path-format=absolute", "--git-path", "index"});
    if (result.exitCode)
      return fail(error, QString::fromUtf8(result.errors).trimmed());
    QByteArray indexPath = result.output;
    if (indexPath.endsWith('\n'))
      indexPath.chop(1);
    if (indexPath.endsWith('\r'))
      indexPath.chop(1);
    mIndexPath = QString::fromUtf8(indexPath);
    if (mIndexPath.isEmpty())
      return fail(error, QObject::tr("Unable to locate the repository index."));
    if (!saveNode(mIndexPath, mIndexBefore, true, error))
      return false;
    if (mIndexBefore.kind != File && mIndexBefore.kind != Missing)
      return fail(error, QObject::tr("The repository index is not a file."));

    if (from.isValid()) {
      result = cli.run({"diff-tree", "--no-commit-id", "--no-renames", "--raw",
                        "-r", "-z", from.id().toString(), to.id().toString()});
    } else {
      // An unborn checkout starts from the actual index. Listing its paths
      // against the destination also includes staged files absent there,
      // without writing a tree or changing the real index's cache extension.
      result =
          cli.run({"--no-optional-locks", "diff-index", "--cached",
                   "--no-renames", "--raw", "-r", "-z", to.id().toString()});
    }
    if (result.exitCode)
      return fail(error, QString::fromUtf8(result.errors).trimmed());

    const QList<QByteArray> fields = result.output.split('\0');
    QStringList candidates;
    for (int i = 0; i + 1 < fields.size(); i += 2) {
      const QList<QByteArray> header = fields.at(i).split(' ');
      if (header.size() != 5 || !header.first().startsWith(':'))
        return fail(error, QObject::tr("Unable to list the checkout paths."));
      // read-tree without --recurse-submodules doesn't change the contents
      // of a submodule when only its gitlink moves.
      if (header.at(0) == ":160000" && header.at(1) == "160000")
        continue;
      QString path = QString::fromUtf8(fields.at(i + 1));
      if (path.isEmpty() || QDir::isAbsolutePath(path) ||
          QDir::cleanPath(path) != path || path == ".." ||
          path.startsWith("../"))
        return fail(error,
                    QObject::tr("Invalid checkout path '%1'.").arg(path));
      // Adding/removing a gitlink over an initialized child repository only
      // changes the index without --recurse-submodules. Leave the child alone.
      if ((header.at(0) == ":160000" && header.at(1) == "000000") ||
          (header.at(0) == ":000000" && header.at(1) == "160000")) {
        Node directory, marker;
        if (!inspect(absolute(path), directory, error))
          return false;
        if (directory.kind == Directory) {
          if (!inspect(absolute(path + "/.git"), marker, error))
            return false;
          if (marker.kind != Missing)
            continue;
        }
      }
      // A non-directory parent must be saved itself. In particular, don't
      // follow a symlink out of the worktree to save one of its descendants.
      qsizetype end = path.indexOf('/');
      while (end >= 0) {
        Node parent;
        if (!inspect(absolute(path.left(end)), parent, error))
          return false;
        if (parent.kind != Directory) {
          path = path.left(end);
          break;
        }
        end = path.indexOf('/', end + 1);
      }
      candidates.append(path);
    }

    std::sort(
        candidates.begin(), candidates.end(),
        [](const QString &a, const QString &b) { return a.size() < b.size(); });
    for (const QString &path : candidates) {
      bool covered = std::any_of(
          mRoots.cbegin(), mRoots.cend(), [&path](const QString &root) {
            return path == root || path.startsWith(root + '/');
          });
      if (covered)
        continue;
      mRoots.append(path);
      if (!saveTree(path, mBefore, true, error))
        return false;
      QString parent = path.section('/', 0, -2);
      while (!parent.isEmpty() && parent != path) {
        mParents.insert(parent);
        if (!mBefore.contains(parent) &&
            !saveNode(absolute(parent), mBefore[parent], false, error))
          return false;
        pathParent(parent);
      }
    }

    // The manifest lets a retained backup be recovered even if a later
    // filesystem error prevents automatic restoration.
    QJsonArray entries;
    for (auto it = mBefore.cbegin(); it != mBefore.cend(); ++it) {
      QJsonObject entry{
          {"path", it.key()},
          {"kind", static_cast<int>(it.value().kind)},
          {"backup", it.value().backup},
          {"permissions", static_cast<int>(it.value().permissions)},
          {"symlink", display(it.value().link)}};
      entries.append(entry);
    }
    QJsonObject manifest{{"workdir", mWorkdir.path()},
                         {"index", mIndexPath},
                         {"indexBackup", mIndexBefore.backup},
                         {"indexExisted", mIndexBefore.kind == File},
                         {"entries", entries}};
    QFile file(mBackup.filePath("manifest.json"));
    QByteArray data = QJsonDocument(manifest).toJson();
    if (!file.open(QFile::WriteOnly) || file.write(data) != data.size())
      return fail(error,
                  QObject::tr("Unable to save the undo backup manifest."));
    return true;
  }

  bool afterCheckout(QString &error) {
    if (saveCurrent(mAfter, error) &&
        saveNode(mIndexPath, mIndexAfter, false, error))
      return true;
    return retain(error);
  }

  bool restore(QString &error) {
    QFile lock(mIndexPath + ".lock");
    if (!lock.open(QFile::WriteOnly | QFile::NewOnly)) {
      error = QObject::tr("Unable to lock the index for recovery: %1")
                  .arg(lock.errorString());
      return retain(error);
    }
    auto unlock = qScopeGuard([&lock] {
      QString path = lock.fileName();
      lock.close();
      QFile::remove(path);
    });

    QMap<QString, Node> current;
    Node index;
    if (!saveCurrent(current, error) ||
        !saveNode(mIndexPath, index, false, error))
      return retain(error);
    if (!same(current, mAfter) || !same(index, mIndexAfter)) {
      error = QObject::tr("The index or working files changed during undo; "
                          "they were not overwritten during recovery.");
      return retain(error);
    }

    std::error_code ec;
    for (const QString &root : mRoots) {
      std::filesystem::remove_all(native(absolute(root)), ec);
      if (ec) {
        error = systemError(absolute(root), ec);
        return retain(error);
      }
    }
    QStringList paths = mBefore.keys();
    std::sort(
        paths.begin(), paths.end(),
        [](const QString &a, const QString &b) { return a.size() < b.size(); });
    for (const QString &path : paths) {
      const Node &node = mBefore[path];
      if (node.kind == Missing)
        continue;
      auto target = native(absolute(path));
      if (node.kind == Directory) {
        std::filesystem::create_directories(target, ec);
      } else if (node.kind == Link) {
        if (node.directoryLink)
          std::filesystem::create_directory_symlink(node.link, target, ec);
        else
          std::filesystem::create_symlink(node.link, target, ec);
      } else if (!QFile::copy(node.backup, absolute(path))) {
        error = QObject::tr("Unable to restore '%1'.").arg(absolute(path));
        return retain(error);
      }
      if (ec) {
        error = systemError(absolute(path), ec);
        return retain(error);
      }
    }
    // Apply directory permissions last so that read-only directories don't
    // prevent their children from being restored.
    for (auto it = paths.crbegin(); it != paths.crend(); ++it) {
      const Node &node = mBefore[*it];
      if (node.kind != Directory && node.kind != File)
        continue;
      std::filesystem::permissions(native(absolute(*it)), node.permissions, ec);
      if (ec) {
        error = systemError(absolute(*it), ec);
        return retain(error);
      }
    }

    if (mIndexBefore.kind == Missing) {
      if (QFile::exists(mIndexPath) && !QFile::remove(mIndexPath)) {
        error = QObject::tr("Unable to restore the missing index.");
        return retain(error);
      }
    } else {
      QFile original(mIndexBefore.backup);
      QSaveFile restored(mIndexPath);
      if (!original.open(QFile::ReadOnly) || !restored.open(QFile::WriteOnly)) {
        error = QObject::tr("Unable to restore the index.");
        return retain(error);
      }
      while (!original.atEnd()) {
        QByteArray bytes = original.read(1024 * 1024);
        if (bytes.isEmpty() || restored.write(bytes) != bytes.size()) {
          error = QObject::tr("Unable to copy the saved index.");
          return retain(error);
        }
      }
      if (!restored.commit()) {
        error = QObject::tr("Unable to restore the index: %1")
                    .arg(restored.errorString());
        return retain(error);
      }
    }
    return true;
  }

private:
  enum Kind { Missing, Directory, File, Link };
  struct Node {
    Kind kind = Missing;
    std::filesystem::perms permissions = std::filesystem::perms::unknown;
    std::filesystem::path link;
    bool directoryLink = false;
    QByteArray hash;
    QString backup;
  };

  static std::filesystem::path native(const QString &path) {
#ifdef Q_OS_WIN
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(QFile::encodeName(path).constData());
#endif
  }
  static QString display(const std::filesystem::path &path) {
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.native());
#else
    return QFile::decodeName(path.native().c_str());
#endif
  }
  static void pathParent(QString &path) {
    qsizetype slash = path.lastIndexOf('/');
    path = slash >= 0 ? path.left(slash) : QString();
  }
  QString absolute(const QString &path) const {
    return mWorkdir.filePath(path);
  }
  static QString systemError(const QString &path, const std::error_code &ec) {
    return QObject::tr("Unable to save or restore '%1': %2")
        .arg(path, QString::fromStdString(ec.message()));
  }
  static bool fail(QString &error, const QString &text) {
    error = text;
    return false;
  }
  bool retain(QString &error) {
    mBackup.setAutoRemove(false);
    error += QObject::tr(" Undo recovery files were kept in '%1'.")
                 .arg(mBackup.path());
    return false;
  }
  bool inspect(const QString &path, Node &node, QString &error) const {
    std::error_code ec;
    auto status = std::filesystem::symlink_status(native(path), ec);
    if (status.type() == std::filesystem::file_type::not_found ||
        ec == std::errc::no_such_file_or_directory ||
        ec == std::errc::not_a_directory) {
      node.kind = Missing;
      return true;
    }
    if (ec)
      return fail(error, systemError(path, ec));
    node.permissions = status.permissions();
    if (std::filesystem::is_symlink(status)) {
      node.kind = Link;
      node.link = std::filesystem::read_symlink(native(path), ec);
      if (ec)
        return fail(error, systemError(path, ec));
      node.directoryLink = std::filesystem::is_directory(native(path), ec);
    } else if (std::filesystem::is_directory(status)) {
      node.kind = Directory;
    } else if (std::filesystem::is_regular_file(status)) {
      node.kind = File;
    } else {
      return fail(
          error, QObject::tr("Unable to back up special file '%1'.").arg(path));
    }
    return true;
  }
  bool saveNode(const QString &path, Node &node, bool backup, QString &error) {
    if (!inspect(path, node, error))
      return false;
    if (node.kind != File)
      return true;
    QFile file(path);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!file.open(QFile::ReadOnly) || !hash.addData(&file))
      return fail(
          error,
          QObject::tr("Unable to read '%1' for undo recovery.").arg(path));
    node.hash = hash.result();
    if (backup) {
      node.backup = mBackup.filePath(QString::number(mNextFile++));
      if (!QFile::copy(path, node.backup))
        return fail(error, QObject::tr("Unable to back up '%1'.").arg(path));
    }
    return true;
  }
  bool saveTree(const QString &path, QMap<QString, Node> &nodes, bool backup,
                QString &error) {
    if (!saveNode(absolute(path), nodes[path], backup, error))
      return false;
    if (nodes[path].kind != Directory)
      return true;
    // Never recursively copy or remove a child repository's object database.
    Node gitDir;
    if (!inspect(absolute(path + "/.git"), gitDir, error))
      return false;
    if (gitDir.kind != Missing)
      return fail(error, QObject::tr("Undo recovery cannot replace the "
                                     "repository directory '%1'.")
                             .arg(path));
    std::error_code ec;
    std::filesystem::directory_iterator it(native(absolute(path)), ec), end;
    if (ec)
      return fail(error, systemError(absolute(path), ec));
    for (; it != end; it.increment(ec)) {
      if (ec)
        return fail(error, systemError(absolute(path), ec));
      if (!saveTree(path + '/' + display(it->path().filename()), nodes, backup,
                    error))
        return false;
    }
    if (ec)
      return fail(error, systemError(absolute(path), ec));
    return true;
  }
  bool saveCurrent(QMap<QString, Node> &nodes, QString &error) {
    // Check parents before traversing leaves. An externally replaced parent
    // must never let recovery follow a symlink outside the working directory.
    QStringList parents = mParents.values();
    std::sort(
        parents.begin(), parents.end(),
        [](const QString &a, const QString &b) { return a.size() < b.size(); });
    for (const QString &parent : parents) {
      if (!saveNode(absolute(parent), nodes[parent], false, error))
        return false;
      if (nodes[parent].kind != Directory && nodes[parent].kind != Missing)
        return fail(error, QObject::tr("Checkout parent '%1' is no longer a "
                                       "directory.")
                               .arg(parent));
    }
    for (const QString &root : mRoots) {
      if (!saveTree(root, nodes, false, error))
        return false;
    }
    return true;
  }
  static bool same(const Node &a, const Node &b) {
    return a.kind == b.kind &&
           (a.kind == Missing ||
            (a.permissions == b.permissions && a.link == b.link &&
             a.hash == b.hash && a.directoryLink == b.directoryLink));
  }
  static bool same(const QMap<QString, Node> &a, const QMap<QString, Node> &b) {
    if (a.keys() != b.keys())
      return false;
    for (auto it = a.cbegin(); it != a.cend(); ++it) {
      if (!same(it.value(), b[it.key()]))
        return false;
    }
    return true;
  }

  QTemporaryDir mBackup{QDir::tempPath() + "/gittyup-undo-XXXXXX"};
  QDir mWorkdir;
  QString mIndexPath;
  Node mIndexBefore;
  Node mIndexAfter;
  QStringList mRoots;
  QSet<QString> mParents;
  QMap<QString, Node> mBefore;
  QMap<QString, Node> mAfter;
  int mNextFile = 0;
};

} // namespace undo_detail

#endif
