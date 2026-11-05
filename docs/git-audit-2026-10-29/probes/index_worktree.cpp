#include "git/Repository.h"
#include "git/Index.h"
#include "git/Config.h"
#include "git/Reference.h"
#include "git/Cli.h"
#include "git2.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDebug>
#include <cstdio>
void write(QString p, QByteArray c) {
  QFile f(p);
  if (!f.open(QFile::WriteOnly))
    qFatal("Cannot create probe file");
  f.write(c);
}
void run(QString d, QStringList a) {
  auto r = git::Cli(d).run(a);
  if (r.exitCode)
    qDebug() << "git failed" << a << r.errors;
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  git_libgit2_init();
  QTemporaryDir tmp;
  QString dir = tmp.path();
  auto repo = git::Repository::init(dir);
  repo.gitConfig().setValue("user.name", QString("Test"));
  repo.gitConfig().setValue("user.email", QString("t@e"));
  write(dir + "/f", "base\n");
  write(dir + "/g", "base\n");
  run(dir, {"add", "."});
  run(dir, {"commit", "-qm", "base"});
  run(dir, {"checkout", "-qb", "other"});
  run(dir, {"rm", "g"});
  run(dir, {"commit", "-qm", "delete"});
  run(dir, {"checkout", "-q", "-"});
  write(dir + "/g", "ours\n");
  run(dir, {"commit", "-qam", "modify"});
  run(dir, {"merge", "other"});
  auto idx = repo.index();
  idx.read();
  fprintf(stderr,
          "modify/delete state=%d Conflicted=%d hasConflicts=%d "
          "conflict.isValid=%d\n",
          idx.isStaged("g"), git::Index::Conflicted, idx.hasConflicts(),
          idx.conflict("g").isValid());
  run(dir, {"merge", "--abort"});
  idx.read();
  write(dir + "/f", "changed f\n");
  write(dir + "/g", "changed g\n");
  run(dir, {"add", "g"});
  fprintf(stderr, "before stage f: %s",
          git::Cli(dir)
              .run({"diff", "--cached", "--name-only"})
              .output.constData());
  idx.setStaged({"f"}, true);
  fprintf(stderr, "after stage f: %s",
          git::Cli(dir)
              .run({"diff", "--cached", "--name-only"})
              .output.constData());
  QTemporaryDir tmp2;
  QString dir2 = tmp2.path();
  auto repo2 = git::Repository::init(dir2);
  repo2.gitConfig().setValue("user.name", QString("Test"));
  repo2.gitConfig().setValue("user.email", QString("t@e"));
  write(dir2 + "/f", "old tracked\n");
  run(dir2, {"add", "."});
  run(dir2, {"commit", "-qm", "base"});
  auto old = repo2.head().target();
  run(dir2, {"rm", "f"});
  run(dir2, {"commit", "-qm", "delete"});
  write(dir2 + "/f", "precious untracked\n");
  bool checkedOut = repo2.checkout(
      old, nullptr, {"f"}, GIT_CHECKOUT_SAFE | GIT_CHECKOUT_DONT_UPDATE_INDEX);
  QFile after(dir2 + "/f");
  if (!after.open(QFile::ReadOnly))
    return 2;
  fprintf(stderr, "safe path checkout result=%d contents=%s", checkedOut,
          after.readAll().constData());

  after.close();
  write(dir2 + "/f", "precious untracked\n");
  git_repository *raw = nullptr;
  git_repository_open(&raw, dir2.toUtf8());
  git_object *obj = nullptr;
  git_revparse_single(&obj, raw, old.id().toString().toUtf8());
  git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
  opts.checkout_strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_DONT_UPDATE_INDEX;
  char pathF[] = "f";
  char *pathArray[] = {pathF};
  opts.paths = {pathArray, 1};
  int rawResult = git_checkout_tree(raw, obj, &opts);
  if (!after.open(QFile::ReadOnly))
    return 2;
  fprintf(stderr, "libgit2 safe path checkout result=%d contents=%s", rawResult,
          after.readAll().constData());
  git_object_free(obj);
  git_repository_free(raw);
}
