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
  for (int scenario = 0; scenario < 2; ++scenario) {
    QTemporaryDir temp;
    QString root = temp.path();
    setup(root, "parent");
    QString dir = root + "/parent";
    auto repo = git::Repository::open(dir);
    QString sha =
        QString::fromUtf8(run(dir + "/sm", {"rev-parse", "HEAD"}).output)
            .trimmed();
    fprintf(
        stderr,
        "scenario=%d clean=%d embedded_before=%d local_commit_on_origin=%d\n",
        scenario, run(dir, {"status", "--porcelain"}).output.isEmpty(),
        QFileInfo(dir + "/sm/.git").isDir(),
        run(root + "/source", {"cat-file", "-e", sha}, false).exitCode == 0);
    if (scenario == 0) {
      auto r = run(dir, {"submodule", "deinit", "--force", "--", "sm"}, false);
      fprintf(
          stderr,
          "native_deinit_exit=%d archived_commit=%d child_git_exists=%d\n",
          r.exitCode,
          run(dir, {"--git-dir=.git/modules/sm", "cat-file", "-e", sha}, false)
                  .exitCode == 0,
          QFileInfo::exists(dir + "/sm/.git"));
    } else {
      auto sm = repo.lookupSubmodule("sm");
      if (!sm.isValid())
        std::abort();
      sm.deinitialize();
      fprintf(stderr,
              "project_deinit child_git_exists=%d archived_git_exists=%d "
              "parent_has_local_commit=%d parent_clean=%d\n",
              QFileInfo::exists(dir + "/sm/.git"),
              QFileInfo::exists(dir + "/.git/modules/sm"),
              run(dir, {"cat-file", "-e", sha}, false).exitCode == 0,
              run(dir, {"status", "--porcelain"}).output.isEmpty());
    }
  }
}
