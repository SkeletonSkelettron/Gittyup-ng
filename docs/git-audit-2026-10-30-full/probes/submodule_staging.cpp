#include "git/Repository.h"
#include "git/Reference.h"
#include "git/Config.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>
#include <iostream>

static QByteArray runGit(const QString &path, const QStringList &args) {
  QProcess p;
  p.setWorkingDirectory(path);
  p.start("git", args);
  if (!p.waitForFinished() || p.exitCode()) {
    std::cerr << args.join(' ').toStdString() << ": "
              << p.readAllStandardError().constData() << '\n';
    exit(2);
  }
  return p.readAllStandardOutput().trimmed();
}
static void put(const QString &path, const char *name, const char *text) {
  QFile f(path + "/" + name);
  if (!f.open(QFile::WriteOnly))
    exit(3);
  f.write(text);
  f.close();
}
static void init(const QString &path) {
  runGit(path, {"init", "-q"});
  runGit(path, {"config", "user.name", "Review"});
  runGit(path, {"config", "user.email", "review@example.test"});
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  qputenv("GIT_CONFIG_GLOBAL", "/dev/null");
  qputenv("GIT_CONFIG_NOSYSTEM", "1");
  git::Repository::init();
  {
    QTemporaryDir tmp;
    QString p = tmp.path(), sub = p + "/sm";
    QDir().mkpath(sub);
    init(p);
    init(sub);
    put(sub, "a", "base\n");
    runGit(sub, {"add", "a"});
    runGit(sub, {"commit", "-qm", "base"});
    put(p, ".gitmodules", "[submodule \"sm\"]\n\tpath = sm\n\turl = fake\n");
    runGit(p, {"add", ".gitmodules", "sm"});
    runGit(p, {"commit", "-qm", "base"});
    runGit(p, {"branch", "base"});
    put(p, "picked", "picked\n");
    runGit(p, {"add", "picked"});
    runGit(p, {"commit", "-qm", "picked"});
    auto repo = git::Repository::open(p);
    auto picked = repo.head().target();
    runGit(p, {"checkout", "-q", "base"});
    put(sub, "a", "new child commit\n");
    runGit(sub, {"add", "a"});
    runGit(sub, {"commit", "-qm", "new child"});
    runGit(p, {"add", "sm"});
    repo.index().read();
    std::cout << "guard_without_ignore=" << !repo.stagedChangesError().isEmpty()
              << '\n';
    repo.gitConfig().setValue("submodule.sm.ignore", QString("all"));
    std::cout << "guard_with_ignore=" << !repo.stagedChangesError().isEmpty()
              << " staged_before="
              << runGit(p, {"diff", "--cached", "--name-only",
                            "--ignore-submodules=none"})
                     .constData()
              << '\n';
    bool pickedOk = repo.cherryPick(picked);
    bool aborted = pickedOk && repo.head().target().resetMerge();
    std::cout << "pick=" << pickedOk << " abort=" << aborted << " staged_after="
              << runGit(p, {"diff", "--cached", "--name-only",
                            "--ignore-submodules=none"})
                     .constData()
              << " status_after="
              << runGit(p, {"status", "--short", "--ignore-submodules=none"})
                     .constData()
              << '\n';
  }
  git::Repository::shutdown();
}
