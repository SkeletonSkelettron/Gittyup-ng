//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Submodule.h"
#include "Cli.h"
#include "Config.h"
#include "Id.h"
#include "Repository.h"

namespace git {

Submodule::Submodule() {}

Submodule::Submodule(git_submodule *submodule)
    : d(submodule, git_submodule_free) {}

Submodule::operator git_submodule *() const { return d.data(); }

bool Submodule::isInitialized() const {
  Repository repo(git_submodule_owner(d.data()));
  QString key = QString("submodule.%1.url").arg(name());
  return !repo.gitConfig().value<QString>(key).isEmpty();
}

void Submodule::initialize() const {
  Repository repo(git_submodule_owner(d.data()));
  Cli(repo.workdir().path()).run({"submodule", "init", "--", path()});
}

void Submodule::deinitialize() const {
  // Remove git config entry.
  Repository repo(git_submodule_owner(d.data()));
  Config config = repo.gitConfig();
  QString regex = QString("submodule\\.%1\\..*").arg(name());
  Config::Iterator it = config.glob(regex);
  while (Config::Entry entry = it.next())
    config.remove(entry.name());

  // Remove submodule workdir.
  QDir dir = repo.workdir();
  if (dir.cd(path()) && dir.removeRecursively())
    dir.mkpath(".");
}

QString Submodule::name() const { return git_submodule_name(d.data()); }

QString Submodule::path() const { return git_submodule_path(d.data()); }

QString Submodule::url() const { return git_submodule_url(d.data()); }

void Submodule::setUrl(const QString &url) {
  if (url == this->url())
    return;

  QByteArray buffer = url.toUtf8();
  const char *data = !buffer.isEmpty() ? buffer.constData() : nullptr;
  git_repository *repo = git_submodule_owner(d.data());
  git_submodule_set_url(repo, git_submodule_name(d.data()), data);
}

QString Submodule::branch() const { return git_submodule_branch(d.data()); }

void Submodule::setBranch(const QString &branch) {
  if (branch == this->branch())
    return;

  QByteArray buffer = branch.toUtf8();
  const char *data = !buffer.isEmpty() ? buffer.constData() : nullptr;
  git_repository *repo = git_submodule_owner(d.data());
  git_submodule_set_branch(repo, git_submodule_name(d.data()), data);
}

Id Submodule::headId() const { return git_submodule_head_id(d.data()); }

Id Submodule::indexId() const { return git_submodule_index_id(d.data()); }

Id Submodule::workdirId() const { return git_submodule_wd_id(d.data()); }

Result Submodule::update(Remote::Callbacks *callbacks, bool init,
                         bool checkout_force) {
  // Check out the commit of the index, like libgit2, whatever the update
  // mode of the submodule is.
  QStringList args = {"submodule", "update", "--checkout"};
  if (init)
    args.append("--init");
  if (checkout_force)
    args.append("--force");
  args << "--" << path();

  Repository repo(git_submodule_owner(d.data()));
  RunResult result = Cli(repo.workdir().path()).run(args);

  // What the remote said while cloning or fetching.
  if (callbacks) {
    for (const QByteArray &line : result.errors.split('\n')) {
      if (line.startsWith("remote: ") && !line.mid(8).trimmed().isEmpty())
        callbacks->sideband(QString::fromUtf8(line.mid(8)) + '\n');
    }
  }

  if (result.exitCode != 0)
    return Result(-1, QString::fromUtf8(result.errors).trimmed());

  return 0;
}

Repository Submodule::open() const {
  git_repository *repo = nullptr;
  git_submodule_open(&repo, d.data());
  return Repository(repo);
}

} // namespace git
