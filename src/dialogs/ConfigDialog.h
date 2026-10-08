//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "QmlDialog.h"
#include "git/Repository.h"
#include <QStringList>
#include <QVariantList>

class RepoView;

// The settings of a repository. qrc:/qml/RepoSettingsPage.qml draws them in
// sections.
class ConfigDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(int section READ section WRITE setSection NOTIFY sectionChanged)
  Q_PROPERTY(QString repoName READ repoName CONSTANT)

  // general
  Q_PROPERTY(bool fetchEnabled READ fetchEnabled WRITE setFetchEnabled NOTIFY
                 generalChanged)
  Q_PROPERTY(int fetchMinutes READ fetchMinutes WRITE setFetchMinutes NOTIFY
                 generalChanged)
  Q_PROPERTY(bool pushAfterCommit READ pushAfterCommit WRITE setPushAfterCommit
                 NOTIFY generalChanged)
  Q_PROPERTY(bool updateSubmodules READ updateSubmodules WRITE
                 setUpdateSubmodules NOTIFY generalChanged)
  Q_PROPERTY(bool pruneAfterFetch READ pruneAfterFetch WRITE setPruneAfterFetch
                 NOTIFY generalChanged)

  // lists
  Q_PROPERTY(QVariantList remotes READ remotes NOTIFY remotesChanged)
  Q_PROPERTY(QVariantList branches READ branches NOTIFY branchesChanged)
  Q_PROPERTY(QStringList upstreams READ upstreams NOTIFY branchesChanged)
  Q_PROPERTY(QVariantList submodules READ submodules NOTIFY submodulesChanged)

  // search
  Q_PROPERTY(bool indexEnabled READ indexEnabled WRITE setIndexEnabled NOTIFY
                 searchChanged)
  Q_PROPERTY(int termLimit READ termLimit WRITE setTermLimit NOTIFY
                 searchChanged)
  Q_PROPERTY(int indexContext READ indexContext WRITE setIndexContext NOTIFY
                 searchChanged)
  Q_PROPERTY(bool indexValid READ indexValid NOTIFY searchChanged)

  // LFS
  Q_PROPERTY(bool lfsInitialized READ lfsInitialized CONSTANT)
  Q_PROPERTY(QStringList lfsIncluded READ lfsIncluded NOTIFY lfsChanged)
  Q_PROPERTY(QStringList lfsExcluded READ lfsExcluded NOTIFY lfsChanged)

public:
  // Keep in sync with RepoSettingsPage.qml.
  enum Index {
    General,
    Diff,
    Remotes,
    Branches,
    Submodules,
    Search,
    Plugins,
    Lfs
  };

  ConfigDialog(RepoView *view, Index index = General);

  void addRemote(const QString &name);
  void editBranch(const QString &name);

  int section() const { return mSection; }
  void setSection(int section);
  QString repoName() const;

  Q_INVOKABLE QString gitConfig(const QString &key) const;
  Q_INVOKABLE void setGitConfig(const QString &key, const QString &value);
  Q_INVOKABLE void editConfigFile();

  bool fetchEnabled() const;
  void setFetchEnabled(bool enabled);
  int fetchMinutes() const;
  void setFetchMinutes(int minutes);
  bool pushAfterCommit() const;
  void setPushAfterCommit(bool push);
  bool updateSubmodules() const;
  void setUpdateSubmodules(bool update);
  bool pruneAfterFetch() const;
  void setPruneAfterFetch(bool prune);

  Q_INVOKABLE int diffContext() const;
  Q_INVOKABLE void setDiffContext(int lines);
  Q_INVOKABLE QStringList encodings() const;
  Q_INVOKABLE int encoding() const;
  Q_INVOKABLE void setEncoding(int index);

  QVariantList remotes() const;
  Q_INVOKABLE void addRemoteWithName(const QString &name = QString());
  Q_INVOKABLE void renameRemote(int index, const QString &name);
  Q_INVOKABLE void setRemoteUrl(int index, const QString &url);
  Q_INVOKABLE void deleteRemote(int index);

  QVariantList branches() const;
  QStringList upstreams() const;
  Q_INVOKABLE void newBranch();
  Q_INVOKABLE bool renameBranch(int index, const QString &name);
  Q_INVOKABLE void setBranchUpstream(int index, int upstream);
  Q_INVOKABLE void setBranchRebase(int index, bool rebase);
  Q_INVOKABLE void deleteBranch(int index);

  QVariantList submodules() const;
  Q_INVOKABLE void setSubmoduleUrl(int index, const QString &url);
  Q_INVOKABLE void setSubmoduleBranch(int index, const QString &branch);
  Q_INVOKABLE void setSubmoduleInitialized(int index, bool initialized);
  Q_INVOKABLE void openSubmodule(int index);

  bool indexEnabled() const;
  void setIndexEnabled(bool enabled);
  int termLimit() const;
  void setTermLimit(int limit);
  int indexContext() const;
  void setIndexContext(int lines);
  bool indexValid() const;
  Q_INVOKABLE void removeIndex();

  Q_INVOKABLE void configurePlugins();

  bool lfsInitialized() const;
  QStringList lfsIncluded() const { return mLfsIncluded; }
  QStringList lfsExcluded() const { return mLfsExcluded; }
  Q_INVOKABLE void initializeLfs();
  Q_INVOKABLE void deinitializeLfs();
  Q_INVOKABLE void trackLfs(const QString &pattern, bool tracked);
  Q_INVOKABLE QString lfsSetting(const QString &key) const;
  Q_INVOKABLE void setLfsConfig(const QString &key, const QVariant &value);
  Q_INVOKABLE void showLfsEnvironment();

signals:
  void sectionChanged();
  void generalChanged();
  void remotesChanged();
  void branchesChanged();
  void submodulesChanged();
  void searchChanged();
  void lfsChanged();
  // Start editing the name of a branch.
  void editBranchRequested(const QString &name);

private:
  void refreshViews();
  void updateLfs();

  RepoView *mView;
  git::Repository mRepo;
  int mSection = General;
  QStringList mLfsIncluded;
  QStringList mLfsExcluded;
};

#endif
