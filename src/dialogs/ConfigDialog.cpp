//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ConfigDialog.h"
#include "AddRemoteDialog.h"
#include "ConfirmDialog.h"
#include "DeleteBranchDialog.h"
#include "NewBranchDialog.h"
#include "PluginsDialog.h"
#include "conf/Settings.h"
#include "git/Branch.h"
#include "git/Config.h"
#include "git/Reference.h"
#include "git/Remote.h"
#include "git/Submodule.h"
#include "index/Index.h"
#include "ui/FileEditor.h"
#include "ui/EditorWindow.h"
#include "ui/RepoView.h"
#include <QDir>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <array>

namespace {

const std::array kEncodings{
    "Utf8",  "Utf16",   "Utf16LE", "Utf16BE",
    "Utf32", "Utf32LE", "Utf32BE", "Latin1",
};

QList<git::Branch> remoteBranches(const git::Repository &repo) {
  QList<git::Branch> branches;
  for (const git::Branch &branch : repo.branches(GIT_BRANCH_REMOTE)) {
    if (!branch.name().endsWith("/HEAD"))
      branches.append(branch);
  }
  return branches;
}

} // namespace

ConfigDialog::ConfigDialog(RepoView *view, Index index)
    : QmlDialog(view), mView(view), mRepo(view->repo()), mSection(index) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Repository Settings"));

  // Follow changes of the repository.
  git::RepositoryNotifier *notifier = mRepo.notifier();
  connect(notifier, &git::RepositoryNotifier::remoteAdded, this,
          &ConfigDialog::remotesChanged);
  connect(notifier, &git::RepositoryNotifier::remoteRemoved, this,
          &ConfigDialog::remotesChanged);
  connect(notifier, &git::RepositoryNotifier::referenceAdded, this,
          &ConfigDialog::branchesChanged);
  connect(notifier, &git::RepositoryNotifier::referenceRemoved, this,
          &ConfigDialog::branchesChanged);
  connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
          &ConfigDialog::branchesChanged);

  if (lfsInitialized())
    updateLfs();

  setContent("RepoSettingsPage");
}

void ConfigDialog::addRemote(const QString &name) {
  setSection(Remotes);
  addRemoteWithName(name);
}

void ConfigDialog::editBranch(const QString &name) {
  setSection(Branches);
  emit editBranchRequested(name);
}

void ConfigDialog::setSection(int section) {
  if (section == mSection)
    return;

  mSection = section;
  emit sectionChanged();
}

QString ConfigDialog::repoName() const { return mRepo.dir(false).dirName(); }

QString ConfigDialog::gitConfig(const QString &key) const {
  return mRepo.gitConfig().value<QString>(key);
}

void ConfigDialog::setGitConfig(const QString &key, const QString &value) {
  git::Config config = mRepo.gitConfig();
  if (value.isEmpty()) {
    config.remove(key);
  } else {
    config.setValue(key, value);
  }
}

void ConfigDialog::editConfigFile() {
  QString file = mRepo.dir().filePath("config");
  if (EditorWindow *window = mView->openEditor(file))
    connect(window->editor(), &FileEditor::saved, this,
            &ConfigDialog::generalChanged);
}

bool ConfigDialog::fetchEnabled() const {
  bool fetch =
      Settings::instance()->value(Setting::Id::FetchAutomatically).toBool();
  return mRepo.appConfig().value<bool>("autofetch.enable", fetch);
}

void ConfigDialog::setFetchEnabled(bool enabled) {
  mRepo.appConfig().setValue("autofetch.enable", enabled);
  mView->startFetchTimer();
  emit generalChanged();
}

int ConfigDialog::fetchMinutes() const {
  int minutes = Settings::instance()
                    ->value(Setting::Id::AutomaticFetchPeriodInMinutes)
                    .toInt();
  return mRepo.appConfig().value<int>("autofetch.minutes", minutes);
}

void ConfigDialog::setFetchMinutes(int minutes) {
  mRepo.appConfig().setValue("autofetch.minutes", minutes);
  mView->startFetchTimer();
  emit generalChanged();
}

bool ConfigDialog::pushAfterCommit() const {
  bool push =
      Settings::instance()->value(Setting::Id::PushAfterEachCommit).toBool();
  return mRepo.appConfig().value<bool>("autopush.enable", push);
}

void ConfigDialog::setPushAfterCommit(bool push) {
  mRepo.appConfig().setValue("autopush.enable", push);
  emit generalChanged();
}

bool ConfigDialog::updateSubmodules() const {
  bool update = Settings::instance()
                    ->value(Setting::Id::UpdateSubmodulesAfterPullAndClone)
                    .toBool();
  return mRepo.appConfig().value<bool>("autoupdate.enable", update);
}

void ConfigDialog::setUpdateSubmodules(bool update) {
  mRepo.appConfig().setValue("autoupdate.enable", update);
  emit generalChanged();
}

bool ConfigDialog::pruneAfterFetch() const {
  bool prune =
      Settings::instance()->value(Setting::Id::PruneAfterFetch).toBool();
  return mRepo.appConfig().value<bool>("autoprune.enable", prune);
}

void ConfigDialog::setPruneAfterFetch(bool prune) {
  mRepo.appConfig().setValue("autoprune.enable", prune);
  emit generalChanged();
}

int ConfigDialog::diffContext() const {
  return mRepo.gitConfig().value<int>("diff.context", 3);
}

void ConfigDialog::setDiffContext(int lines) {
  mRepo.gitConfig().setValue("diff.context", lines);
  refreshViews();
}

QStringList ConfigDialog::encodings() const {
  QStringList encodings = {tr("System Locale")};
  for (const char *encoding : kEncodings)
    encodings.append(encoding);
  return encodings;
}

int ConfigDialog::encoding() const {
  QString name = gitConfig("gui.encoding");
  for (int i = 0; i < static_cast<int>(kEncodings.size()); ++i) {
    if (name == kEncodings[i])
      return i + 1;
  }

  return 0;
}

void ConfigDialog::setEncoding(int index) {
  git::Config config = mRepo.gitConfig();
  if (index <= 0 || index > static_cast<int>(kEncodings.size())) {
    config.remove("gui.encoding");
  } else {
    config.setValue("gui.encoding", QString(kEncodings[index - 1]));
  }

  refreshViews();
}

QVariantList ConfigDialog::remotes() const {
  QVariantList remotes;
  for (const git::Remote &remote : mRepo.remotes())
    remotes.append(QVariantMap{{"name", remote.name()}, {"url", remote.url()}});
  return remotes;
}

void ConfigDialog::addRemoteWithName(const QString &name) {
  AddRemoteDialog *dialog = new AddRemoteDialog(name, this);
  connect(dialog, &QDialog::accepted, this, [this, dialog] {
    mRepo.addRemote(dialog->name(), dialog->url());
    emit remotesChanged();
  });

  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->open();
}

void ConfigDialog::renameRemote(int index, const QString &name) {
  QList<git::Remote> remotes = mRepo.remotes();
  if (index < 0 || index >= remotes.size() || name.isEmpty() ||
      remotes.at(index).name() == name)
    return;

  git::Remote remote = remotes.at(index);
  remote.setName(name);
  emit remotesChanged();
}

void ConfigDialog::setRemoteUrl(int index, const QString &url) {
  QList<git::Remote> remotes = mRepo.remotes();
  if (index < 0 || index >= remotes.size() || url.isEmpty() ||
      remotes.at(index).url() == url)
    return;

  git::Remote remote = remotes.at(index);
  remote.setUrl(url);
  emit remotesChanged();
}

void ConfigDialog::deleteRemote(int index) {
  QList<git::Remote> remotes = mRepo.remotes();
  if (index < 0 || index >= remotes.size())
    return;

  QString name = remotes.at(index).name();
  ConfirmDialog dialog(this);
  dialog.setTitle(tr("Delete Remote?"));
  dialog.setText(tr("Are you sure you want to delete '%1'?").arg(name));
  dialog.setAcceptText(tr("Delete"));
  dialog.setDanger(true);
  if (dialog.exec() != QDialog::Accepted)
    return;

  mRepo.deleteRemote(name);
  emit remotesChanged();
}

QVariantList ConfigDialog::branches() const {
  QList<git::Branch> remotes = remoteBranches(mRepo);
  QVariantList branches;
  for (const git::Branch &branch : mRepo.branches(GIT_BRANCH_LOCAL)) {
    git::Branch upstream = branch.upstream();
    int upstreamIndex = 0;
    for (int i = 0; upstream.isValid() && i < remotes.size(); ++i) {
      if (remotes.at(i).qualifiedName() == upstream.qualifiedName())
        upstreamIndex = i + 1;
    }

    branches.append(QVariantMap{{"name", branch.name()},
                                {"upstream", upstreamIndex},
                                {"rebase", branch.isRebase()},
                                {"head", branch.isHead()}});
  }

  return branches;
}

QStringList ConfigDialog::upstreams() const {
  QStringList upstreams = {tr("None")};
  for (const git::Branch &branch : remoteBranches(mRepo))
    upstreams.append(branch.name());
  return upstreams;
}

void ConfigDialog::newBranch() {
  NewBranchDialog *dialog = new NewBranchDialog(mRepo, git::Commit(), this);
  connect(dialog, &QDialog::accepted, this, [this, dialog] {
    git::Branch branch = mRepo.createBranch(dialog->name(), dialog->target());

    // Start tracking.
    if (branch.isValid())
      branch.setUpstream(dialog->upstream());

    emit branchesChanged();
  });

  dialog->open();
}

bool ConfigDialog::renameBranch(int index, const QString &name) {
  QList<git::Branch> branches = mRepo.branches(GIT_BRANCH_LOCAL);
  if (index < 0 || index >= branches.size())
    return false;

  git::Branch branch = branches.at(index);
  if (branch.name() == name)
    return true;

  if (branch.isHead() || !git::Branch::isNameValid(name) ||
      mRepo.lookupBranch(name, GIT_BRANCH_LOCAL).isValid())
    return false;

  branch.rename(name);
  emit branchesChanged();
  return true;
}

void ConfigDialog::setBranchUpstream(int index, int upstream) {
  QList<git::Branch> branches = mRepo.branches(GIT_BRANCH_LOCAL);
  if (index < 0 || index >= branches.size())
    return;

  QList<git::Branch> remotes = remoteBranches(mRepo);
  git::Branch branch = branches.at(index);
  branch.setUpstream(upstream > 0 && upstream <= remotes.size()
                         ? remotes.at(upstream - 1)
                         : git::Branch());
  emit branchesChanged();
}

void ConfigDialog::setBranchRebase(int index, bool rebase) {
  QList<git::Branch> branches = mRepo.branches(GIT_BRANCH_LOCAL);
  if (index < 0 || index >= branches.size())
    return;

  git::Branch branch = branches.at(index);
  branch.setRebase(rebase);
  emit branchesChanged();
}

void ConfigDialog::deleteBranch(int index) {
  QList<git::Branch> branches = mRepo.branches(GIT_BRANCH_LOCAL);
  if (index < 0 || index >= branches.size() || branches.at(index).isHead())
    return;

  DeleteBranchDialog dialog(branches.at(index), this);
  dialog.exec();
  emit branchesChanged();
}

QVariantList ConfigDialog::submodules() const {
  QVariantList submodules;
  for (const git::Submodule &submodule : mRepo.submodules()) {
    submodules.append(QVariantMap{{"name", submodule.name()},
                                  {"url", submodule.url()},
                                  {"branch", submodule.branch()},
                                  {"initialized", submodule.isInitialized()}});
  }

  return submodules;
}

void ConfigDialog::setSubmoduleUrl(int index, const QString &url) {
  QList<git::Submodule> submodules = mRepo.submodules();
  if (index < 0 || index >= submodules.size() ||
      submodules.at(index).url() == url)
    return;

  git::Submodule submodule = submodules.at(index);
  submodule.setUrl(url);
  emit submodulesChanged();
}

void ConfigDialog::setSubmoduleBranch(int index, const QString &branch) {
  QList<git::Submodule> submodules = mRepo.submodules();
  if (index < 0 || index >= submodules.size() ||
      submodules.at(index).branch() == branch)
    return;

  git::Submodule submodule = submodules.at(index);
  submodule.setBranch(branch);
  emit submodulesChanged();
}

void ConfigDialog::setSubmoduleInitialized(int index, bool initialized) {
  QList<git::Submodule> submodules = mRepo.submodules();
  if (index < 0 || index >= submodules.size())
    return;

  git::Submodule submodule = submodules.at(index);
  if (initialized) {
    submodule.initialize();
    emit submodulesChanged();
    return;
  }

  // Deinitializing removes the working directory of the submodule.
  QDir dir(mRepo.workdir().filePath(submodule.path()));
  if (!dir.isEmpty()) {
    ConfirmDialog dialog(this);
    dialog.setTitle(tr("Deinitialize Submodule?"));
    dialog.setText(tr("Deinitializing '%1' will remove its working "
                      "directory. Are you sure you want to deinitialize?")
                       .arg(submodule.name()));
    if (GIT_SUBMODULE_STATUS_IS_WD_DIRTY(
            mRepo.submoduleStatus(submodule.name())))
      dialog.setInformativeText(
          tr("The submodule working directory contains uncommitted "
             "changes that will be lost if you continue."));
    dialog.setAcceptText(tr("Deinitialize"));
    dialog.setDanger(true);
    if (dialog.exec() != QDialog::Accepted) {
      emit submodulesChanged();
      return;
    }
  }

  submodule.deinitialize();
  emit submodulesChanged();
}

void ConfigDialog::openSubmodule(int index) {
  QList<git::Submodule> submodules = mRepo.submodules();
  if (index >= 0 && index < submodules.size())
    mView->openSubmodule(submodules.at(index));
}

bool ConfigDialog::indexEnabled() const {
  return mRepo.appConfig().value<bool>("index.enable", true);
}

void ConfigDialog::setIndexEnabled(bool enabled) {
  mRepo.appConfig().setValue("index.enable", enabled);
  if (enabled) {
    mView->startIndexing();
  } else {
    mView->cancelIndexing();
  }

  emit searchChanged();
}

int ConfigDialog::termLimit() const {
  return mRepo.appConfig().value<int>("index.termlimit", 1000000);
}

void ConfigDialog::setTermLimit(int limit) {
  mRepo.appConfig().setValue("index.termlimit", limit);
  emit searchChanged();
}

int ConfigDialog::indexContext() const {
  return mRepo.appConfig().value<int>("index.contextlines", 3);
}

void ConfigDialog::setIndexContext(int lines) {
  mRepo.appConfig().setValue("index.contextlines", lines);
  emit searchChanged();
}

bool ConfigDialog::indexValid() const { return mView->index()->isValid(); }

void ConfigDialog::removeIndex() {
  mView->cancelIndexing();
  mView->index()->remove();
  emit searchChanged();
}

void ConfigDialog::configurePlugins() {
  (new PluginsDialog(mRepo, this))->open();
}

bool ConfigDialog::lfsInitialized() const {
  return git::Repository(mRepo).lfsIsInitialized();
}

void ConfigDialog::initializeLfs() {
  mView->lfsInitialize();
  close();
}

void ConfigDialog::deinitializeLfs() {
  ConfirmDialog dialog(this);
  dialog.setTitle(tr("Deinitialize LFS?"));
  dialog.setText(
      tr("Are you sure you want uninstall LFS from this repository?"));
  dialog.setAcceptText(tr("Deinitialize"));
  dialog.setDanger(true);
  if (dialog.exec() != QDialog::Accepted)
    return;

  mView->lfsDeinitialize();
  close();
}

void ConfigDialog::trackLfs(const QString &pattern, bool tracked) {
  if (pattern.isEmpty())
    return;

  mRepo.lfsSetTracked(pattern, tracked);
  git::Repository::LfsTracking tracking = mRepo.lfsTracked();
  mLfsIncluded = tracking.included;
  mLfsExcluded = tracking.excluded;
  emit lfsChanged();
}

QString ConfigDialog::lfsSetting(const QString &key) const {
  for (const QString &string : git::Repository(mRepo).lfsEnvironment()) {
    if (string.section('=', 0, 0) == key)
      return string.section('=', 1).section(' ', 0, 0);
  }

  return QString();
}

void ConfigDialog::setLfsConfig(const QString &key, const QVariant &value) {
  git::Config config = mRepo.gitConfig();
  if (value.typeId() == QMetaType::Bool) {
    config.setValue(key, value.toBool());
  } else if (value.typeId() == QMetaType::Int ||
             value.typeId() == QMetaType::Double) {
    config.setValue(key, value.toInt());
  } else {
    config.setValue(key, value.toString());
  }
}

void ConfigDialog::showLfsEnvironment() {
  ConfirmDialog *dialog = ConfirmDialog::information(
      this, tr("LFS Environment"), tr("The output of 'git lfs env'."),
      git::Repository(mRepo).lfsEnvironment().join('\n'));
  dialog->open();
}

void ConfigDialog::refreshViews() { mView->refresh(); }

void ConfigDialog::updateLfs() {
  auto *watcher = new QFutureWatcher<git::Repository::LfsTracking>(this);
  connect(watcher, &QFutureWatcher<git::Repository::LfsTracking>::finished,
          this, [this, watcher] {
            mLfsIncluded = watcher->result().included;
            mLfsExcluded = watcher->result().excluded;
            watcher->deleteLater();
            emit lfsChanged();
          });

  watcher->setFuture(QtConcurrent::run(&git::Repository::lfsTracked, mRepo));
}
