#include "git/Repository.h"
#include "git/Config.h"
#include "git/Reference.h"
#include "git/Submodule.h"
#include "git/Cli.h"
#include "git2/global.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <cstdio>
#include <cstdlib>
void write(const QString &p, const QByteArray &c) {
  QFile f(p);
  if (!f.open(QFile::WriteOnly) || f.write(c) != c.size())
    std::abort();
}
git::RunResult run(const QString &d, const QStringList &a,
                   bool required = true) {
  auto r = git::Cli(d).run(a);
  if (required && r.exitCode) {
    fprintf(stderr, "git failed %s\n", r.errors.constData());
    std::abort();
  }
  return r;
}
void setup(const QString &root, const QString &parent) {
  run(root, {"init", "-q", "source"});
  QString source = root + "/source";
  run(source, {"config", "user.name", "Test"});
  run(source, {"config", "user.email", "t@e"});
  write(source + "/file.txt", "origin\n");
  run(source, {"add", "."});
  run(source, {"commit", "-qm", "origin"});
  run(root, {"init", "-q", parent});
  QString dir = root + "/" + parent;
  run(dir, {"config", "user.name", "Test"});
  run(dir, {"config", "user.email", "t@e"});
  run(dir, {"clone", "-q", source, "sm"});
  run(dir + "/sm", {"config", "user.name", "Test"});
  run(dir + "/sm", {"config", "user.email", "t@e"});
  write(dir + "/sm/file.txt", "local committed only\n");
  run(dir + "/sm", {"commit", "-qam", "unpushed local"});
  run(dir,
      {"-c", "protocol.file.allow=always", "submodule", "add", source, "sm"});
  run(dir, {"commit", "-qam", "register submodule"});
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  git_libgit2_init();
  QTemporaryDir temp;
  QString root = temp.path();
  setup(root, "parent");
  QString dir = root + "/parent";
  for (const auto &pair :
       QList<QPair<QString, QString>>{{"a.b", "a"}, {"axb", "b"}}) {
    run(dir, {"-c", "protocol.file.allow=always", "submodule", "add", "--name",
              pair.first, root + "/source", pair.second});
  }
  run(dir, {"commit", "-qam", "add two more"});
  auto repo = git::Repository::open(dir);
  auto first = repo.lookupSubmodule("a.b");
  auto second = repo.lookupSubmodule("axb");
  fprintf(stderr, "before first_initialized=%d second_initialized=%d\n",
          first.isInitialized(), second.isInitialized());
  first.deinitialize();
  fprintf(
      stderr,
      "after first_url_exit=%d second_url_exit=%d second_worktree_exists=%d\n",
      run(dir, {"config", "--get", "submodule.a.b.url"}, false).exitCode,
      run(dir, {"config", "--get", "submodule.axb.url"}, false).exitCode,
      QFileInfo::exists(dir + "/b/.git"));
}
