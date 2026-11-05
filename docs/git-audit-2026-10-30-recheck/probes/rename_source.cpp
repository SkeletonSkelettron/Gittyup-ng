#include "git/Repository.h"
#include "git/Index.h"
#include "git/Config.h"
#include "git/Reference.h"
#include "git/Cli.h"
#include "git2.h"
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
QByteArray read(const QString &p) {
  QFile f(p);
  return f.open(QFile::ReadOnly) ? f.readAll() : QByteArray("<absent>\n");
}
void run(const QString &d, const QStringList &a) {
  auto r = git::Cli(d).run(a);
  if (r.exitCode) {
    fprintf(stderr, "unexpected git failure %s\n", r.errors.constData());
    std::abort();
  }
}
int native(const QString &dir, const QString &id, const char *path,
           int strategy) {
  git_repository *r = nullptr;
  git_repository_open(&r, dir.toUtf8());
  git_object *o = nullptr;
  git_revparse_single(&o, r, id.toUtf8());
  git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
  opts.checkout_strategy = strategy;
  char *paths[] = {const_cast<char *>(path)};
  opts.paths = {paths, 1};
  int result = git_checkout_tree(r, o, &opts);
  git_object_free(o);
  git_repository_free(r);
  return result;
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  git_libgit2_init();
  for (int scenario = 0; scenario < 2; scenario++) {
    QTemporaryDir temp;
    QString dir = temp.path();
    auto repo = git::Repository::init(dir);
    repo.gitConfig().setValue("user.name", QString("Test"));
    repo.gitConfig().setValue("user.email", QString("t@e"));
    write(dir + "/old.txt", "old\n");
    run(dir, {"add", "."});
    run(dir, {"commit", "-qm", "base"});
    auto old = repo.head().target();
    write(dir + "/old.txt", "precious renamed\n");
    run(dir, {"commit", "-qam", "second"});
    run(dir, {"mv", "old.txt", "new.txt"});
    fprintf(stderr, "scenario=%d initial_status=%s", scenario,
            git::Cli(dir).run({"status", "--short"}).output.constData());
    int strategy = GIT_CHECKOUT_SAFE;
    if (scenario == 0)
      strategy |= GIT_CHECKOUT_DONT_UPDATE_INDEX;
    int raw = native(dir, old.id().toString(), "old.txt", strategy);
    fprintf(stderr, "scenario=%d native_result=%d native_old=%s", scenario, raw,
            read(dir + "/old.txt").constData());
    bool project = repo.checkout(old, nullptr, {"old.txt"}, strategy);
    fprintf(stderr, "scenario=%d project_result=%d project_old=%s", scenario,
            project, read(dir + "/old.txt").constData());
    fprintf(stderr, "scenario=%d new_content=%s", scenario,
            read(dir + "/new.txt").constData());
    fprintf(stderr, "scenario=%d final_status=%s", scenario,
            git::Cli(dir).run({"status", "--short"}).output.constData());
    fprintf(stderr, "scenario=%d cached_old=%s", scenario,
            git::Cli(dir).run({"show", ":old.txt"}).output.constData());
  }
}
