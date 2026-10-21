//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Repository.h"
#include "Signing.h"
#include "AnnotatedCommit.h"
#include "Blame.h"
#include "Branch.h"
#include "Cli.h"
#include "Commit.h"
#include "Config.h"
#include "Filter.h"
#include "FilterList.h"
#include "Index.h"
#include "Patch.h"
#include "qtsupport.h"
#include "Rebase.h"
#include "Reference.h"
#include "Remote.h"
#include "RevWalk.h"
#include "Signature.h"
#include "Submodule.h"
#include "TagRef.h"
#include "Tree.h"
#include "util/Path.h"
#include "git2/buffer.h"
#include "git2/branch.h"
#include "git2/checkout.h"
#include "git2/cherrypick.h"
#include "git2/filter.h"
#include "git2/global.h"
#include "git2/ignore.h"
#include "git2/merge.h"
#include "git2/rebase.h"
#include "git2/refs.h"
#include "git2/remote.h"
#include "git2/repository.h"
#include "git2/signature.h"
#include "git2/stash.h"
#include "git2/tag.h"
#include "git2/sys/repository.h"
#include "git2/sys/errors.h"
#include "git2/attr.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPair>
#include <QProcess>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QVector>

#ifdef Q_OS_UNIX
#include <pwd.h>
#include <unistd.h>
#endif

namespace git {

namespace {

// Whether git itself opens the repository at 'path'.
bool gitAccepts(const QString &path) {
  QString git = QStandardPaths::findExecutable("git");
#ifdef Q_OS_WIN
  if (git.isEmpty() && QFileInfo::exists("C:/Program Files/Git/cmd/git.exe"))
    git = "C:/Program Files/Git/cmd/git.exe";
#endif
  if (git.isEmpty())
    return false;

  QProcess process;
  process.start(git, {"-C", path, "rev-parse", "--git-dir"});
  return process.waitForFinished(10000) &&
         process.exitStatus() == QProcess::NormalExit &&
         process.exitCode() == 0;
}

// Collects the paths that differ between the index and the workdir, as
// 'git status' lists them. git checks the workdir much faster than libgit2
// does on Windows, where it caches file system data (core.fscache). Returns
// false if git can't tell.
bool workdirChanges(const QString &workdir, bool untracked,
                    QStringList &paths) {
  // Don't refresh the index: that would race with Gittyup's own writes.
  QStringList args = {"--no-optional-locks", "status", "--porcelain=v2", "-z"};
  args.append(untracked ? "--untracked-files=all" : "--untracked-files=no");

  RunResult result = Cli(workdir).run(args);
  if (result.exitCode != 0)
    return false;

  const QList<QByteArray> records = result.output.split('\0');
  for (int i = 0; i < records.size(); ++i) {
    const QByteArray &record = records.at(i);
    if (record.size() < 3)
      continue;

    // The path follows a number of fields, which depends on the kind of entry.
    int fields = 0;
    switch (record.at(0)) {
      case '1': // changed
        fields = 8;
        break;
      case '2': // renamed or copied
        fields = 9;
        ++i; // The next record is the original path.
        break;
      case 'u': // unmerged
        fields = 10;
        break;
      case '?': // untracked
        fields = 1;
        break;
      default: // ignored
        continue;
    }

    // Changed, renamed and copied entries may differ only in the index.
    if ((record.at(0) == '1' || record.at(0) == '2') &&
        (record.size() < 4 || record.at(3) == '.'))
      continue;

    qsizetype pos = 0;
    for (int field = 0; field < fields && pos >= 0; ++field) {
      pos = record.indexOf(' ', pos);
      if (pos >= 0)
        ++pos;
    }

    if (pos > 0)
      paths.append(QString::fromUtf8(record.mid(pos)));
  }

  return true;
}

const QString kConfigDir = "gittyup";
const QString kConfigFile = "config";
const QString kStarFile = "starred";

#ifndef USE_SYSTEM_LIBGIT2
int blame_progress(const git_oid *suspect, void *payload) {
  return reinterpret_cast<Blame::Callbacks *>(payload)->progress() ? 0 : -1;
}
#endif

int insert_stash_id(size_t index, const char *message, const git_oid *id,
                    void *payload) {
  reinterpret_cast<QList<Id> *>(payload)->insert(index, id);
  return 0;
}

} // namespace

QMap<git_repository *, QWeakPointer<Repository::Data>> Repository::registry;

Repository::Data::Data(git_repository *repo)
    : repo(repo), notifier(new RepositoryNotifier) {
  // Load starred commits.
  git_oid_t hash_type = git_repository_oid_type(repo);
  QDir dir(git_repository_path(repo));
  QFile file(appDir(dir).filePath(kStarFile));
  if (!file.open(QIODevice::ReadOnly))
    return;

  QByteArray ids = file.readAll();
  if (ids.isEmpty())
    return;

  for (const QByteArray &id : ids.split('\n'))
    starredCommits.insert(git::Id(QByteArray::fromHex(id), hash_type));
}

Repository::Data::~Data() {
  delete notifier;
  git_repository_free(repo);
}

void Repository::unregisterRepository(Data *data) {
  registry.remove(data->repo);
  delete data;
}

QSharedPointer<Repository::Data>
Repository::registerRepository(git_repository *repo) {
  if (!repo)
    return QSharedPointer<Data>();

  auto it = registry.find(repo);
  if (it != registry.end())
    return *it;

  QSharedPointer<Data> ref(new Data(repo), unregisterRepository);
  registry[repo] = ref.toWeakRef();
  return ref;
}

Repository::Repository() {}

Repository::Repository(git_repository *repo) : d(registerRepository(repo)) {}

Repository::operator git_repository *() const { return d->repo; }

QDir Repository::dir(bool includeGitFolder) const {
  QDir dir(git_repository_path(d->repo));
  if (!includeGitFolder && dir.dirName() == ".git") {
    if (!dir.cdUp()) {
      assert(false); // must be done explicit, because in release build the
                     // assert will not be executed, so assert(dir.cdUp()) does
                     // not work in release build
    }
  }
  return dir;
}

QDir Repository::workdir() const {
  return isBare() ? dir() : QDir(git_repository_workdir(d->repo));
}

QDir Repository::appDir() const { return appDir(dir()); }

Id Repository::workdirId(const QString &path) const {
  git_oid id;
  if (git_blob_create_from_workdir(&id, d->repo, path.toUtf8()))
    return Id();

  return id;
}

QString Repository::message() const {
  git_buf buf = GIT_BUF_INIT;
  git_repository_message(&buf, d->repo);
  QString msg = QString::fromUtf8(buf.ptr, buf.size);
  git_buf_dispose(&buf);
  return msg;
}

// Config file used for git specific configs
// config file in <Repository>/.git/config
Config Repository::gitConfig() const {
  git_config *config = nullptr;
  git_repository_config(&config, d->repo);
  return Config(config);
}

// Config file used for app specific configs
// config file in <Repository>/.git/gittyup/config
Config Repository::appConfig() const {
  Config config = Config::appGlobal();
  QString path = appDir().filePath(kConfigFile);
  config.addFile(path, GIT_CONFIG_LEVEL_LOCAL, d->repo);
  return config;
}

bool Repository::forcesDiff3Conflicts() const {
  return gitConfig().value<QString>("merge.conflictStyle").isEmpty();
}

git_oid_t Repository::oidType() const {
  return git_repository_oid_type(d->repo);
}

bool Repository::isBare() const { return git_repository_is_bare(d->repo); }

Signature Repository::signature(const QString &name, const QString &email) {
  return Signature(name, email);
}

Signature Repository::signature(const QString &name, const QString &email,
                                const QDateTime &date) {
  return Signature(name, email, date);
}

Signature Repository::defaultSignature(bool *fake, const QString &overrideUser,
                                       const QString &overrideEmail) const {
  QString name, email;

  if (fake)
    *fake = false;

  git_signature *signature = nullptr;
  if (!git_signature_default(&signature, d->repo)) {
    Signature res(signature, true);

    if (res.isValid() &&
        (!overrideUser.isEmpty() || !overrideEmail.isEmpty())) {
      if (overrideUser.isEmpty())
        name = res.name();
      else
        name = overrideUser;

      if (overrideEmail.isEmpty())
        email = res.email();
      else
        email = overrideEmail;

      git_signature_new(&signature, name.toUtf8(), email.toUtf8(),
                        res.gitDate().time, res.gitDate().offset);

      res = Signature(signature, true);
    }

    return res;
  }

#ifdef Q_OS_UNIX
  // Get user name.
  passwd *pw = getpwuid(getuid());
  name = pw->pw_gecos;

  // Create fake email address.
  char hostname[256];
  gethostname(hostname, sizeof(hostname));
  email = QString("%1@%2").arg(pw->pw_name, hostname);
#else
  name = getenv("USERNAME");
  QString hostname = getenv("COMPUTERNAME");
  email = QString("%1@%2.local").arg(name, hostname);
#endif

  if (!overrideUser.isEmpty())
    name = overrideUser;
  if (!overrideEmail.isEmpty())
    email = overrideEmail;

  if (!git_signature_now(&signature, name.toUtf8(), email.toUtf8())) {
    if (fake)
      *fake = true;

    return Signature(signature, true);
  }

  return Signature();
}

bool Repository::isIgnored(const QString &path) const {
  int ignored = 0;
  git_ignore_path_is_ignored(&ignored, d->repo, path.toUtf8());
  return ignored;
}

Index Repository::index() const {
  git_index *index = nullptr;
  git_repository_index(&index, d->repo);
  return Index(index);
}

void Repository::setIndex(const Index &index) {
  git_repository_set_index(d->repo, index);
}

/*!
 * \brief Repository::status
 * Generates the diff from the index to the working directory
 * \param index
 * \param callbacks
 * \param ignoreWhitespace
 * \return
 */
Diff Repository::status(const Index &index, Diff::Callbacks *callbacks,
                        bool ignoreWhitespace) const {
  Tree tree;
  if (Reference ref = head()) {
    if (Commit commit = ref.target())
      tree = commit.tree();
  }

  Diff diff = diffTreeToIndex(tree, index, ignoreWhitespace);
  if (!diff.isValid())
    return Diff();

  // Let git find the changes in the workdir, and diff only those. Diff the
  // whole workdir if git can't tell.
  QStringList paths;
  bool untracked = !appConfig().value<bool>("untracked.hide", false);
  bool listed = workdirChanges(this->workdir().path(), untracked, paths);
  if (!listed || !paths.isEmpty()) {
    Diff workdir =
        diffIndexToWorkdir(index, callbacks, ignoreWhitespace, paths);
    if (!workdir.isValid())
      return Diff();

    diff.merge(workdir);
  }

  diff.setIndex(index);

  return diff.count() ? diff : Diff();
}

Diff Repository::diffTreeToIndex(const Tree &tree, const Index &index,
                                 bool ignoreWhitespace) const {
  git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
  opts.flags |= GIT_DIFF_INCLUDE_TYPECHANGE;

  if (!appConfig().value<bool>("untracked.hide", false))
    opts.flags |= GIT_DIFF_INCLUDE_UNTRACKED | GIT_DIFF_RECURSE_UNTRACKED_DIRS;

  if (ignoreWhitespace)
    opts.flags |= GIT_DIFF_IGNORE_WHITESPACE;

  git_diff *diff = nullptr;
  git_diff_tree_to_index(&diff, d->repo, tree, index, &opts);
  return Diff(diff);
}

Diff Repository::diffIndexToWorkdir(const Index &index,
                                    Diff::Callbacks *callbacks,
                                    bool ignoreWhitespace,
                                    const QStringList &paths) const {
  git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
  opts.flags |= GIT_DIFF_INCLUDE_TYPECHANGE;
#ifndef USE_SYSTEM_LIBGIT2
  opts.flags |= GIT_DIFF_DISABLE_MMAP;
#endif

  // libgit2 doesn't even look at other paths when they're matched exactly.
  QVector<QByteArray> storage;
  QVector<char *> rawPaths;
  if (!paths.isEmpty()) {
    storage.reserve(paths.size());
    for (const QString &path : paths)
      storage.append(path.toUtf8());

    rawPaths.reserve(storage.size());
    for (QByteArray &path : storage)
      rawPaths.append(path.data());

    opts.flags |= GIT_DIFF_DISABLE_PATHSPEC_MATCH;
    opts.pathspec.count = rawPaths.size();
    opts.pathspec.strings = rawPaths.data();
  }

  if (!appConfig().value<bool>("untracked.hide", false))
    opts.flags |= GIT_DIFF_INCLUDE_UNTRACKED | GIT_DIFF_RECURSE_UNTRACKED_DIRS;

  if (ignoreWhitespace)
    opts.flags |= GIT_DIFF_IGNORE_WHITESPACE;

  if (callbacks) {
    opts.progress_cb = &Diff::Callbacks::progress;
    opts.payload = callbacks;
  }

  git_diff *diff = nullptr;
  git_diff_index_to_workdir(&diff, d->repo, index, &opts);
  return Diff(diff);
}

Reference Repository::head() const {
  git_reference *ref = nullptr;
  git_repository_head(&ref, d->repo);
  return Reference(ref);
}

bool Repository::isHeadUnborn() const {
  return git_repository_head_unborn(d->repo);
}

bool Repository::isHeadDetached() const {
  return git_repository_head_detached(d->repo);
}

QString Repository::unbornHeadName() const {
  git_reference *head = nullptr;
  if (git_reference_lookup(&head, d->repo, "HEAD"))
    return QString();

  QString name = git_reference_symbolic_target(head);
  git_reference_free(head);

  return name.section('/', -1);
}

bool Repository::setHead(const Reference &ref) {
  int error = git_repository_set_head(d->repo, ref.qualifiedName().toUtf8());
  emit d->notifier->referenceUpdated(head());
  return !error;
}

bool Repository::setHeadDetached(const Commit &commit) {
  int error = git_repository_set_head_detached(d->repo, commit);
  emit d->notifier->referenceUpdated(head());
  return !error;
}

QList<Reference> Repository::refs() const {
  git_reference_iterator *it = nullptr;
  if (git_reference_iterator_new(&it, d->repo))
    return QList<Reference>();

  QList<Reference> refs;
  git_reference *ref = nullptr;
  while (!git_reference_next(&ref, it))
    refs.append(Reference(ref));

  git_reference_iterator_free(it);

  return refs;
}

Reference Repository::lookupRef(const QString &name) const {
  if (name.isEmpty())
    return Reference();

  git_reference *ref = nullptr;
  git_reference_lookup(&ref, d->repo, name.toUtf8());
  return Reference(ref);
}

QList<Branch> Repository::branches(git_branch_t flags) const {
  git_branch_iterator *it = nullptr;
  if (git_branch_iterator_new(&it, d->repo, flags))
    return QList<Branch>();

  QList<Branch> branches;
  git_reference *ref = nullptr;
  git_branch_t type = GIT_BRANCH_LOCAL;
  while (!git_branch_next(&ref, &type, it))
    branches.append(Branch(ref));

  git_branch_iterator_free(it);

  return branches;
}

Branch Repository::lookupBranch(const QString &name, git_branch_t flags) const {
  if (name.isEmpty())
    return Branch();

  git_reference *branch = nullptr;
  git_branch_lookup(&branch, d->repo, name.toUtf8(), flags);
  return Branch(branch);
}

Branch Repository::createBranch(const QString &name, const Commit &target,
                                bool force) {
  Commit commit = target;
  if (!commit.isValid()) {
    Branch branch = head();
    if (!branch.isValid())
      return Branch();

    commit = branch.target();
    if (!commit.isValid())
      return Branch();
  }

  emit d->notifier->referenceAboutToBeAdded(name);

  QStringList args = {"branch", name};

  if (force)
    args.append("-f");

  args.append(commit.id().toString());

  git::Cli cli(workdir().path());

  RunResult result = cli.run(args);

  if (result.exitCode != 0) {
    emit notifier()->gitCommandFailed(tr("create branch"),
                                      QString::fromUtf8(result.errors));
    return Branch();
  }

  Branch branch = lookupBranch(name, GIT_BRANCH_LOCAL);
  emit d->notifier->referenceAdded(branch);

  return branch;
}

QList<TagRef> Repository::tags() const {
  git_reference_iterator *it = nullptr;
  if (git_reference_iterator_new(&it, d->repo))
    return QList<TagRef>();

  QList<TagRef> refs;
  git_reference *ref = nullptr;
  while (!git_reference_next(&ref, it)) {
    TagRef tag(ref);
    if (tag.isValid())
      refs.append(tag);
  }

  git_reference_iterator_free(it);

  return refs;
}

TagRef Repository::lookupTag(const QString &name) const {
  return lookupRef(QString("refs/tags/%1")
                       .arg(name)); // TODO: check if possible to use
                                    // GIT_REFS_TAGS_DIR instead of refs/tags
}

QStringList Repository::existingTags() const {

  git_strarray array;
  git_tag_list(&array, operator git_repository *());

  QStringList list;

  for (size_t i = 0; i < array.count; i++) {
    list.append(array.strings[i]);
  }

  git_strarray_dispose(&array);
  return list;
}

TagRef Repository::createTag(const Commit &target, const QString &name,
                             const QString &message, bool force,
                             const QString &overrideUser,
                             const QString &overrideEmail) {
  emit d->notifier->referenceAboutToBeAdded(name);

  QStringList args = {"tag"};

  if (!message.isEmpty()) {
    args.append("-a");
    args.append("-m");
    args.append(message);
  } else {
    args.append("--no-sign");
  }
  args.append(name);
  args.append(target.id().toString());

  if (force)
    args.append("-f");

  QMap<QString, QString> envs;

  // Tag as the author chosen in the commit panel.
  if (!overrideUser.isEmpty())
    envs.insert("GIT_COMMITTER_NAME", overrideUser);
  if (!overrideEmail.isEmpty())
    envs.insert("GIT_COMMITTER_EMAIL", overrideEmail);

  git::Cli cli(workdir().path());
  RunResult result = cli.run(args, {}, envs);

  if (result.exitCode != 0) {
    emit notifier()->gitCommandFailed(tr("create tag"),
                                      QString::fromUtf8(result.errors));
    return TagRef();
  }

  TagRef tag = lookupTag(name);
  emit d->notifier->referenceAdded(tag);

  return tag;
}

Blob Repository::lookupBlob(const Id &id) const {
  git_object *obj = nullptr;
  git_object_lookup(&obj, d->repo, id, GIT_OBJECT_BLOB);
  return Blob(reinterpret_cast<git_blob *>(obj));
}

RevWalk Repository::walker(int sort) const {
  git_revwalk *revwalk = nullptr;
  if (git_revwalk_new(&revwalk, d->repo))
    return RevWalk();

  RevWalk walker(revwalk);
  git_revwalk_sorting(revwalk, sort);
  for (const Reference &ref : refs())
    git_revwalk_push_ref(revwalk, ref.qualifiedName().toUtf8());

  return walker;
}

Commit Repository::lookupCommit(const QString &prefix) const {
  git_oid id;
  git_oid_fromstrp(&id, prefix.toUtf8());

  git_commit *commit = nullptr;
  git_commit_lookup_prefix(&commit, d->repo, &id, prefix.length());
  return Commit(commit);
}

Commit Repository::lookupCommit(const Id &id) const {
  git_commit *commit = nullptr;
  git_commit_lookup(&commit, d->repo, id);
  return Commit(commit);
}

bool Repository::amend(const git::Commit &commitToAmend,
                       const git::Signature &author,
                       const git::Signature &committer,
                       const QString &commitMessage) {

  // Write the index tree.
  Index idx = index();
  if (!idx.isValid())
    return false;

  // Add new staged files to the amended commit
  Tree tree = idx.writeTree();
  if (!tree.isValid())
    return false;

  return commitToAmend.amend(author, committer, commitMessage, tree);
}

Commit Repository::commit(const QString &message,
                          const AnnotatedCommit &mergeHead, bool *fakeSignature,
                          const QString &overrideUser,
                          const QString &overrideEmail) {
  // Get the default signature for the repo.
  Signature signature =
      defaultSignature(fakeSignature, overrideUser, overrideEmail);

  return commit(signature, signature, message, mergeHead);
}

Commit Repository::commit(const Signature &author, const Signature &committer,
                          const QString &message,
                          const AnnotatedCommit &mergeHead) {
  if (!author.isValid() || !committer.isValid())
    return Commit();

  // Write the index tree.
  Index idx = index();
  if (!idx.isValid())
    return Commit();

  Tree tree = idx.writeTree();
  if (!tree.isValid())
    return Commit();

  // Lookup the parent commit.
  QVector<git_commit *> parents;
  if (Reference ref = head()) {
    if (Commit commit = ref.target())
      parents.append(commit);
  }

  // Add merge head.
  if (mergeHead.isValid())
    parents.append(mergeHead.commit());

  // Create the commit, signed when the configuration asks for it.
  git_oid id;
  QByteArray raw = message.toUtf8();
  if (Signing::createCommit(&id, d->repo, "HEAD", author, committer,
                            raw.constData(), tree, parents.size(),
                            (const git_commit **)parents.data()))
    return Commit();

  // Cleanup merge state.
  switch (state()) {
    case GIT_REPOSITORY_STATE_NONE:
    case GIT_REPOSITORY_STATE_MERGE:
    case GIT_REPOSITORY_STATE_REVERT:
    case GIT_REPOSITORY_STATE_CHERRYPICK:
      cleanupState();
      break;

    default:
      // no-op
      break;
  }

  git_commit *commit = nullptr;
  git_commit_lookup(&commit, d->repo, &id);
  emit d->notifier->referenceUpdated(head());
  return Commit(commit);
}

QList<Commit> Repository::starredCommits() const {
  QList<Commit> commits;
  for (const Id &id : d->starredCommits) {
    if (Commit commit = lookupCommit(id))
      commits.append(commit);
  }

  return commits;
}

bool Repository::isCommitStarred(const Id &commit) const {
  return d->starredCommits.contains(commit);
}

void Repository::setCommitStarred(const Id &commit, bool starred) {
  if (starred) {
    d->starredCommits.insert(commit);
  } else {
    d->starredCommits.remove(commit);
  }

  // Write to disk.
  QSaveFile file(appDir().filePath(kStarFile));
  if (!file.open(QIODevice::WriteOnly))
    return;

  QByteArrayList ids;
  for (const Id &id : d->starredCommits)
    ids.append(id.toByteArray().toHex());

  file.write(ids.join('\n'));
  file.commit();
}

void Repository::invalidateSubmoduleCache() {
  git_repository_submodule_cache_clear(d->repo);
  d->submodules.clear();
  d->submodulesCached = false;
}

QList<Submodule> Repository::submodules() const {
  ensureSubmodulesCached();

  QList<Submodule> submodules;
  for (const QString &name : d->submodules) {
    if (Submodule submodule = lookupSubmodule(name))
      submodules.append(submodule);
  }

  return submodules;
}

Submodule Repository::lookupSubmodule(const QString &name) const {
  ensureSubmodulesCached();

  git_submodule *submodule = nullptr;
  git_submodule_lookup(&submodule, d->repo, name.toUtf8());
  return Submodule(submodule);
}

int Repository::submoduleStatus(const QString &name) const {

  unsigned int status;
  // TODO: testing!!!!
  int returnValue =
      git_submodule_status(&status, d->repo, name.toLocal8Bit().data(),
                           GIT_SUBMODULE_IGNORE_UNSPECIFIED);
  if (returnValue < 0)
    return returnValue;
  return status;
}

Remote Repository::addRemote(const QString &name, const QString &url) {
  emit d->notifier->remoteAboutToBeAdded(name);

  git::Cli cli(workdir().path());
  RunResult result = cli.run({"remote", "add", name, url});

  if (result.exitCode != 0) {
    emit d->notifier->gitCommandFailed(tr("add remote"),
                                       QString::fromUtf8(result.errors));
    return Remote();
  }

  Remote remote = lookupRemote(name);
  emit d->notifier->remoteAdded(remote);
  return remote;
}

void Repository::deleteRemote(const QString &name) {
  Remote remote = lookupRemote(name);
  if (!remote.isValid())
    return;

  emit d->notifier->remoteAboutToBeRemoved(remote);

  git::Cli cli(workdir().path());
  RunResult result = cli.run({"remote", "remove", name});

  if (result.exitCode != 0) {
    emit d->notifier->gitCommandFailed(tr("delete remote"),
                                       QString::fromUtf8(result.errors));
  }

  emit d->notifier->remoteRemoved(name);
}

Remote Repository::defaultRemote() const {
  Branch branch = head();
  if (branch.isValid()) {
    Remote remote = branch.remote();
    if (remote.isValid())
      return remote;
  }

  return lookupRemote("origin");
}

QList<Remote> Repository::remotes() const {
  git_strarray names;
  if (git_remote_list(&names, d->repo))
    return QList<Remote>();

  QList<Remote> remotes;
  for (size_t i = 0; i < names.count; ++i) {
    if (Remote remote = lookupRemote(names.strings[i]))
      remotes.append(remote);
  }

  git_strarray_dispose(&names);

  return remotes;
}

Remote Repository::lookupRemote(const QString &name) const {
  git_remote *remote = nullptr;
  git_remote_lookup(&remote, d->repo, name.toUtf8());
  return Remote(remote);
}

Remote Repository::anonymousRemote(const QString &url) const {
  git_remote *remote = nullptr;
  git_remote_create_anonymous(&remote, d->repo, url.toUtf8());
  return Remote(remote);
}

Reference Repository::stashRef() const { return lookupRef("refs/stash"); }

QList<Commit> Repository::stashes() const {
  QList<Id> ids;
  git_stash_foreach(d->repo, insert_stash_id, &ids);

  QList<Commit> commits;
  for (const Id &id : ids) {
    git_commit *commit = nullptr;
    if (!git_commit_lookup(&commit, d->repo, id))
      commits.append(Commit(commit));
  }

  return commits;
}

Commit Repository::stash(const QString &message) {
  Id before = stashRef().target().id();
  QStringList args = {"stash", "push"};

  if (!message.isEmpty()) {
    args.append("-m");
    args.append(message);
  }

  git::Cli cli(workdir().path());

  RunResult result = cli.run(args);

  if (result.exitCode != 0) {
    emit d->notifier->gitCommandFailed(tr("stash"),
                                       QString::fromUtf8(result.errors));
    return Commit();
  }

  Commit commit = stashRef().target();

  if (commit.id() == before) {
    // "No local changes to save"
    emit d->notifier->gitCommandFailed(tr("stash"),
                                       QString::fromUtf8(result.output));
    return Commit();
  }

  emit d->notifier->referenceUpdated(stashRef());
  return commit;
}

bool Repository::applyStash(int index) {

  QStringList args = {"stash", "apply", QString("stash@{%1}").arg(index)};
  if (forcesDiff3Conflicts())
    args = QStringList{"-c", "merge.conflictStyle=diff3"} + args;

  git::Cli cli(workdir().path());

  RunResult result = cli.run(args);

  if (result.exitCode != 0) {
    QByteArray text = result.errors.isEmpty() ? result.output : result.errors;
    emit d->notifier->gitCommandFailed(tr("apply stash"),
                                       QString::fromUtf8(text));
    return false;
  }
  return true;
}

bool Repository::dropStash(int index) {
  // The stash reference goes away when this is the last stash.
  // Signal that the previous saved reference changed instead.
  Reference ref = stashRef();

  QStringList args = {"stash", "drop", QString("stash@{%1}").arg(index)};

  git::Cli cli(workdir().path());

  RunResult result = cli.run(args);

  if (result.exitCode != 0) {
    QByteArray text = result.errors.isEmpty() ? result.output : result.errors;
    emit d->notifier->gitCommandFailed(tr("drop stash"),
                                       QString::fromUtf8(text));
    return false;
  }

  emit d->notifier->referenceUpdated(ref);
  return true;
}

bool Repository::popStash(int index) {
  // The stash reference goes away when this is the last stash.
  // Signal that the previous saved reference changed instead.
  Reference ref = stashRef();

  QStringList args = {"stash", "pop", QString("stash@{%1}").arg(index)};
  if (forcesDiff3Conflicts())
    args = QStringList{"-c", "merge.conflictStyle=diff3"} + args;

  git::Cli cli(workdir().path());

  RunResult result = cli.run(args);

  if (result.exitCode != 0) {
    QByteArray text = result.errors.isEmpty() ? result.output : result.errors;
    emit d->notifier->gitCommandFailed(tr("pop stash"),
                                       QString::fromUtf8(text));
    return false;
  }

  emit d->notifier->referenceUpdated(ref);
  return true;
}

Blame Repository::blame(const QString &name, const Commit &from,
                        Blame::Callbacks *callbacks) const {
  git_blame *blame = nullptr;
  git_blame_options options = GIT_BLAME_OPTIONS_INIT;
  if (from.isValid()) // Set start commit.
    options.newest_commit = *git_commit_id(from);
#ifndef USE_SYSTEM_LIBGIT2
  if (callbacks) {
    options.progress_cb = blame_progress;
    options.payload = callbacks;
  }
#endif
  git_blame_file(&blame, d->repo, name.toUtf8(), &options);
  return Blame(blame, d->repo);
}

FilterList Repository::filters(const QString &path, const Blob &blob) const {
  git_filter_list *filters = nullptr;
  git_filter_list_load(&filters, d->repo, blob, path.toUtf8(),
                       GIT_FILTER_TO_WORKTREE, GIT_FILTER_DEFAULT);
  return FilterList(filters);
}

Commit Repository::mergeBase(const Commit &lhs, const Commit &rhs) const {
  git_oid id;
  if (git_merge_base(&id, d->repo, lhs, rhs))
    return Commit();

  git_commit *commit = nullptr;
  git_commit_lookup(&commit, d->repo, &id);
  return Commit(commit);
}

bool Repository::merge(const AnnotatedCommit &mergeHead) {
  int current = state();
  const git_annotated_commit *head = mergeHead;
  git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
  git_checkout_options checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
  checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;
  if (forcesDiff3Conflicts())
    checkoutOpts.checkout_strategy |= GIT_CHECKOUT_CONFLICT_STYLE_DIFF3;
  int error = git_merge(d->repo, &head, 1, &mergeOpts, &checkoutOpts);
  if (state() != current)
    emit d->notifier->stateChanged();

  return !error;
}

/*!
 * \brief Repository::rebaseOpen
 * Open current rebase and return a Rebase object.
 * If no current rebase is ongoing an invalid Rebase object is returned
 * \return
 */
Rebase Repository::rebaseOpen() {
  git_rebase *rebase = nullptr;
  git_rebase_options opts = GIT_REBASE_OPTIONS_INIT; // TODO: check quite option
  // Sign the commits when the configuration asks for it.
  opts.commit_create_cb = Signing::createRebaseCommit;
  opts.payload = d->repo;
  if (forcesDiff3Conflicts())
    opts.checkout_options.checkout_strategy |=
        GIT_CHECKOUT_CONFLICT_STYLE_DIFF3;
  git_rebase_open(&rebase, d->repo, &opts);
  return Rebase(d->repo, rebase);
}

void Repository::rebaseAbort() {

  Rebase r = rebaseOpen();
  if (r.isValid())
    r.abort();
}

// TODO: check that all arguments passed to the signals are valid when the
// RepoView gets the notification! (Using sharedpointer?)
void Repository::rebase(const AnnotatedCommit &mergeHead,
                        const QString &overrideUser,
                        const QString &overrideEmail) {
  git_rebase *r = nullptr;
  git_rebase_options opts = GIT_REBASE_OPTIONS_INIT;
  opts.commit_create_cb = Signing::createRebaseCommit;
  opts.payload = d->repo;
  if (forcesDiff3Conflicts())
    opts.checkout_options.checkout_strategy |=
        GIT_CHECKOUT_CONFLICT_STYLE_DIFF3;
  git_rebase_init(&r, d->repo, nullptr, mergeHead, nullptr, &opts);
  auto rebase = git::Rebase(d->repo, r, overrideUser, overrideEmail);

  if (!rebase.isValid()) {
    emit d->notifier->rebaseInitError();
    return;
  }

  // start rebasing
  rebaseContinue(QStringLiteral(""));
}

void Repository::rebaseContinue(const QString &commitMessage) {

  Rebase r = rebaseOpen();
  if (!r.isValid()) {
    // rebase anymore available. maybe rebased externally
    return;
  }

  if (r.currentIndex() != GIT_REBASE_NO_OPERATION) {
    // Rebase::next() was already called at leas once
    // externally or by a previous call of rebaseContinue

    // Check if it can be committed
    git::Commit c = r.commit(commitMessage);
    if (!c.isValid()) {
      emit d->notifier->rebaseConflict(r);
      return;
    } else {
      emit d->notifier->rebaseCommitSuccess(r, c, r.commitToRebase(),
                                            r.currentIndex() + 1);
      // Go on with the next rebase operation below
    }
  }
  // Loop over rebase operations.
  while (r.hasNext()) {
    git::Commit before = r.next();
    if (!before.isValid()) {
      emit d->notifier->rebaseCommitInvalid(r);
      rebaseAbort();
      return;
    }
    int currCommit =
        r.currentIndex() +
        1; // for showing to user it makes more sense starting from 1
    emit d->notifier->rebaseAboutToRebase(r, before, currCommit);

    QString message = before.message(); // use original message
    git::Commit after = r.commit(message);
    if (!after.isValid()) {
      emit d->notifier->rebaseConflict(r);
      return; // before ongoing, the user has to fix the conflicts.
    }

    emit d->notifier->rebaseCommitSuccess(r, after, before, currCommit);
  }

  if (r.finish())
    emit d->notifier->rebaseFinished(r);
  // TODO: implement
  // else
  // emit error
}

bool Repository::rebaseOngoing() {
  Rebase r = rebaseOpen();
  return r.isValid();
}

bool Repository::cherryPick(const Commit &commit) {
  int current = state();
  // libgit2 only allows conflicts by default when no strategy is given.
  git_cherrypick_options opts = GIT_CHERRYPICK_OPTIONS_INIT;
  opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_ALLOW_CONFLICTS;
  if (forcesDiff3Conflicts())
    opts.checkout_opts.checkout_strategy |= GIT_CHECKOUT_CONFLICT_STYLE_DIFF3;
  int error = git_cherrypick(d->repo, commit, &opts);
  if (state() != current)
    emit d->notifier->stateChanged();

  return !error;
}

bool Repository::checkout(const Commit &commit, CheckoutCallbacks *callbacks,
                          const QStringList &paths, int strategy) {
  // git read-tree checks out a whole tree like git_checkout_tree: it updates
  // the index and the workdir, but not HEAD.
  if (paths.isEmpty()) {
    bool force = strategy & GIT_CHECKOUT_FORCE;
    Q_ASSERT(!(strategy & ~GIT_CHECKOUT_FORCE));

    Cli cli(workdir().path());
    QString target = commit.isValid() ? commit.id().toString() : "HEAD";
    int flags = callbacks ? callbacks->flags() : GIT_CHECKOUT_NOTIFY_NONE;

    // The files that change, like libgit2 reports the updated ones.
    QList<QPair<char, QString>> files;
    if ((flags & GIT_CHECKOUT_NOTIFY_UPDATED) && !isHeadUnborn()) {
      RunResult result =
          cli.run({"diff-tree", "-r", "-z", "--name-status", "HEAD", target});
      QList<QByteArray> fields = result.output.split('\0');
      for (int i = 0; i + 1 < fields.size(); i += 2) {
        char status = fields.at(i).isEmpty() ? 'M' : fields.at(i).at(0);
        if (status != 'A' && status != 'D')
          status = 'M';
        files.append({status, QString::fromUtf8(fields.at(i + 1))});
      }
    }

    QStringList args = {"read-tree", "-u"};
    if (force) {
      args << "--reset" << target;
    } else if (isHeadUnborn()) {
      args << "-m" << target;
    } else {
      args << "-m"
           << "HEAD" << target;
    }

    RunResult result = cli.run(args);
    if (result.exitCode != 0) {
      // "error: Entry 'path' not uptodate. Cannot merge." and the like.
      if (callbacks) {
        for (const QByteArray &line : result.errors.split('\n')) {
          qsizetype begin = line.indexOf('\'');
          qsizetype end = line.lastIndexOf('\'');
          if (line.startsWith("error: ") && begin >= 0 && end > begin) {
            QByteArray path = line.mid(begin + 1, end - begin - 1);
            callbacks->notify('!', QString::fromUtf8(path));
          }
        }
      }

      return false;
    }

    index().read();

    if (callbacks) {
      // The local changes that are kept, like libgit2 reports the dirty files.
      if (flags & GIT_CHECKOUT_NOTIFY_DIRTY) {
        RunResult dirty =
            cli.run({"--no-optional-locks", "diff", "--name-only", "-z"});
        for (const QByteArray &path : dirty.output.split('\0')) {
          if (!path.isEmpty())
            files.append({'M', QString::fromUtf8(path)});
        }
      }

      // The callbacks log the files at the end.
      for (const QPair<char, QString> &file : files)
        callbacks->notify(file.first, file.second);
      callbacks->progress(QString(), files.size(), files.size());
    }

    return true;
  }

  // SAFE is 0: it is the absence of FORCE.
  bool force = strategy & GIT_CHECKOUT_FORCE;
  bool updateIndex = !(strategy & GIT_CHECKOUT_DONT_UPDATE_INDEX);
  Q_ASSERT(
      !(strategy & ~(GIT_CHECKOUT_FORCE | GIT_CHECKOUT_DONT_UPDATE_INDEX)));

  Cli cli(workdir().path());

  // git restore overwrites local changes, libgit2 refuses unless forced.
  if (!force) {
    RunResult result = cli.run({"diff", "--name-only", "-z"});
    if (result.exitCode != 0)
      return false;

    QSet<QString> changed;
    for (const QByteArray &path : result.output.split('\0'))
      changed.insert(QString::fromUtf8(path));

    bool conflicts = false;
    for (const QString &path : paths) {
      if (changed.contains(path)) {
        conflicts = true;
        if (callbacks)
          callbacks->notify('!', path);
      }
    }

    if (conflicts)
      return false;
  }

  QByteArray input;
  for (const QString &path : paths) {
    input.append(path.toUtf8());
    input.append('\0');
  }

  // Paths are matched exactly.
  QString source = commit.isValid() ? commit.id().toString() : "HEAD";
  QStringList args = {"--literal-pathspecs", "restore", "--source=" + source,
                      "--worktree"};
  if (updateIndex)
    args << "--staged";
  args << "--pathspec-from-file=-"
       << "--pathspec-file-nul";

  RunResult result = cli.run(args, input);
  if (result.exitCode != 0)
    return false;

  if (updateIndex)
    index().read();

  // Report the files like libgit2 does: the callbacks log them at the end.
  if (callbacks) {
    for (const QString &path : paths)
      callbacks->notify('M', path);
    callbacks->progress(QString(), paths.size(), paths.size());
  }

  return true;
}

int Repository::state() const { return git_repository_state(d->repo); }

void Repository::cleanupState() {
  int current = state();
  git_repository_state_cleanup(d->repo);
  if (state() != current) {
    Patch::clearConflictResolutions(d->repo);
    emit d->notifier->stateChanged();
  }
}

QStringConverter::Encoding Repository::encoding() const {
  QString encoding = gitConfig().value<QString>("gui.encoding");
  auto conv = QStringConverter::encodingForName(encoding.toLocal8Bit().data());
  return conv ? conv.value() : QStringConverter::Utf8;
}

QStringConverter::Encoding Repository::encoding(const QByteArray &text) const {
  QString encoding = gitConfig().value<QString>("gui.encoding");
  auto conv = QStringConverter::encodingForName(encoding.toLocal8Bit().data());
  if (conv)
    return conv.value();

  // Most text is UTF-8 now, and the encoding of the system isn't on Windows.
  // (Decoders decode when the result is converted.)
  QStringDecoder utf8(QStringConverter::Utf8);
  QString decoded = utf8.decode(text);
  if (!utf8.hasError())
    return QStringConverter::Utf8;

  // Older text is in the encoding of the system, or else in Latin-1, which
  // keeps every byte when the text is saved again.
  QStringDecoder system(QStringConverter::System);
  decoded = system.decode(text);
  return system.hasError() ? QStringConverter::Latin1
                           : QStringConverter::System;
}

QString Repository::decode(const QByteArray &text) const {
  return QStringDecoder{encoding(text)}.decode(text);
}

QString Repository::attributeValue(const QString &attribute,
                                   const QString &path) {
  const char *value;
  uint32_t flags = 0;
  git_attr_get(&value, d->repo, flags, path.toLocal8Bit().data(),
               attribute.toLocal8Bit().data());

  // value must not be freed!
  return QString(value);
}

bool Repository::lfsIsInitialized() { return dir().exists("hooks/pre-push"); }

bool Repository::lfsInitialize() {
  return (!lfsExecute({"install"}).isEmpty() && lfsIsInitialized());
}

bool Repository::lfsDeinitialize() {
  return !lfsExecute({"uninstall", "--local"}).isEmpty();
}

QByteArray Repository::lfsSmudge(const QByteArray &lfsPointerText,
                                 const QString &file) {
  return lfsExecute({"smudge", file}, lfsPointerText);
}

QStringList Repository::lfsEnvironment() {
  return QString(lfsExecute({"env"})).split('\n');
}

Repository::LfsTracking Repository::lfsTracked() {
  QString output = lfsExecute({"track"});
  QStringList lines = output.split('\n', Qt::SkipEmptyParts);
  if (!lines.isEmpty())
    lines.removeFirst();

  QStringList tracked, excluded;
  bool excluding = false;
  for (const QString &line : lines) {
    if (line[0] != ' ') {
      excluding = true;
    } else if (excluding) {
      excluded.append(line.trimmed().section(' ', 0, 0));
    } else {
      tracked.append(line.trimmed().section(' ', 0, 0));
    }
  }

  return LfsTracking{tracked, excluded};
}

bool Repository::lfsSetTracked(const QString &pattern, bool tracked) {
  QStringList args;
  args.append(tracked ? "track" : "untrack");
  args.append(pattern);
  if (tracked)
    args.append("--lockable");

  return !lfsExecute(args).isEmpty();
}

QSet<QString> Repository::lfsLocks() {
  if (!d->lfsLocksCached) {
    QByteArray output = lfsExecute({"locks", "--json"});
    if (!output.isEmpty()) {
      QJsonDocument doc = QJsonDocument::fromJson(output);
      QJsonArray array = doc.array();
      for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array.at(i).toObject();
        QString path = obj.value("path").toString();
        d->lfsLocks.insert(path);
      }
    }

    d->lfsLocksCached = true;
  }

  return d->lfsLocks;
}

bool Repository::lfsIsLocked(const QString &path) {
  return lfsLocks().contains(path);
}

bool Repository::lfsSetLocked(const QString &path, bool locked) {
  if (lfsExecute({locked ? "lock" : "unlock", path}).isEmpty())
    return false;

  if (locked) {
    d->lfsLocks.insert(path);
  } else {
    d->lfsLocks.remove(path);
  }

  emit d->notifier->lfsLocksChanged();
  return true;
}

bool Repository::clean(const QString &name) {
  QDir dir = workdir();
  return (dir.remove(name) || (dir.cd(name) && dir.removeRecursively()));
}

int Repository::lastErrorKind() {
  const git_error *err = git_error_last();
  return err ? err->klass : GIT_ERROR_NONE;
}

QString Repository::lastError(const QString &defaultError) {
  if (!defaultError.isEmpty())
    return defaultError;

  const git_error *err = git_error_last();
  return err ? err->message : tr("Unknown error");
}

QDir Repository::appDir(const QDir &dir) {
  QDir app = dir;
  app.mkpath(kConfigDir);
  app.cd(kConfigDir);
  return app;
}

Repository Repository::init(const QString &path, bool bare) {
  git_repository *repo = nullptr;
  git_repository_init(&repo, util::canonicalizePath(path).toUtf8(), bare);
  return Repository(repo);
}

Repository Repository::open(const QString &path, bool searchParents) {
  git_repository *repo = nullptr;
  int flags = searchParents ? 0 : GIT_REPOSITORY_OPEN_NO_SEARCH;
  QByteArray dir = util::canonicalizePath(path).toUtf8();
  int error = git_repository_open_ext(&repo, dir, flags, nullptr);

  // libgit2 refuses more repositories that aren't owned by the current user
  // than git on Windows, like folders of the Administrators while Gittyup
  // isn't elevated. Open the repositories that git itself opens.
  if (error == GIT_EOWNER && gitAccepts(path)) {
    git_libgit2_opts(GIT_OPT_SET_OWNER_VALIDATION, 0);
    git_repository_open_ext(&repo, dir, flags, nullptr);
    git_libgit2_opts(GIT_OPT_SET_OWNER_VALIDATION, 1);
  }

  return Repository(repo);
}

void Repository::init() {
  git_libgit2_init();

  // Set global options.
  git_libgit2_opts(GIT_OPT_ENABLE_STRICT_HASH_VERIFICATION, false);

  // Load global filters.
  Filter::init();

#ifdef Q_OS_LINUX
  // FIXME: There has to be a better way...
  QStringList paths = {"/etc/ssl/certs/ca-certificates.crt",
                       "/etc/pki/tls/certs/ca-bundle.crt"};

  for (const QString &path : paths) {
    if (QFile::exists(path)) {
      git_libgit2_opts(GIT_OPT_SET_SSL_CERT_LOCATIONS,
                       path.toUtf8().constData(), NULL);
      break;
    }
  }
#endif
}

void Repository::shutdown() { git_libgit2_shutdown(); }

RepositoryNotifier::RepositoryNotifier(QObject *parent) : QObject(parent) {}

void Repository::ensureSubmodulesCached() const {
  if (!d->submodulesCached) {
    d->submodulesCached = true;
    git_submodule_foreach(
        d->repo,
        [](git_submodule *, const char *name, void *payload) {
          reinterpret_cast<QStringList *>(payload)->append(name);
          return 0;
        },
        &d->submodules);
    git_repository_submodule_cache_all(d->repo);
  }
}

QByteArray Repository::lfsExecute(const QStringList &args,
                                  const QByteArray &input) const {
  QString path = QStandardPaths::findExecutable("git-lfs");
  if (path.isEmpty()) {
    emit d->notifier->lfsNotFound();
    git_error_set_str(GIT_ERROR_INVALID, tr("git-lfs not found").toUtf8());
    return QByteArray();
  }

  QProcess process;
  process.setWorkingDirectory(workdir().path());
  process.start(path, args);
  if (!input.isEmpty()) {
    process.write(input);
    process.closeWriteChannel();
  }

  process.waitForFinished();
  if (process.exitCode() != 0) {
    git_error_set_str(GIT_ERROR_INVALID, process.readAllStandardError());
    return QByteArray();
  }

  return process.readAllStandardOutput();
}

} // namespace git
