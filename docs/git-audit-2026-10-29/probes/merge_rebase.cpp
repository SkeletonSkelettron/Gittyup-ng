#include "git/Repository.h"
#include "git/Reference.h"
#include "git/Config.h"
#include "git/Signature.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>
#include <iostream>

static QByteArray runGit(const QString &path, const QStringList &args) {
  QProcess p;
  p.setWorkingDirectory(path);
  p.start("git", args);
  p.waitForFinished();
  if (p.exitCode() != 0)
    std::cerr << "CLI failure " << args.join(' ').toStdString() << ":"
              << p.readAllStandardError().constData() << '\n';
  return p.readAllStandardOutput().trimmed();
}
static void put(const QString &path, const char *file, const char *data) {
  QFile f(path + "/" + file);
  if (!f.open(QFile::WriteOnly))
    qFatal("Cannot create probe file");
  f.write(data);
  f.close();
  runGit(path, {"add", file});
  runGit(path, {"commit", "-qm", QString("change ") + file});
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  qputenv("GIT_CONFIG_GLOBAL", "/dev/null");
  qputenv("GIT_CONFIG_NOSYSTEM", "1");
  git::Repository::init();
  {
    QTemporaryDir tmp;
    auto repo = git::Repository::init(tmp.path());
    repo.gitConfig().setValue("user.name", QString("Configured User"));
    repo.gitConfig().setValue("user.email", QString("configured@example.test"));
    put(tmp.path(), "a", "base\n");
    put(tmp.path(), "a", "changed\n");
    auto target = repo.head().target();
    repo.index().read();
    bool result = target.revert();
    std::cout
        << "REVERT result=" << result << " state=" << repo.state()
        << " REVERT_HEAD=" << QFile::exists(tmp.path() + "/.git/REVERT_HEAD")
        << " staged="
        << runGit(tmp.path(), {"diff", "--cached", "--name-only"}).constData()
        << '\n';
  }
  {
    QTemporaryDir tmp;
    auto repo = git::Repository::init(tmp.path());
    repo.gitConfig().setValue("user.name", QString("Configured User"));
    repo.gitConfig().setValue("user.email", QString("configured@example.test"));
    put(tmp.path(), "a", "base\n");
    runGit(tmp.path(), {"branch", "base"});
    put(tmp.path(), "side", "side\n");
    runGit(tmp.path(), {"branch", "side"});
    runGit(tmp.path(), {"checkout", "-q", "base"});
    put(tmp.path(), "upstream", "upstream\n");
    runGit(tmp.path(), {"checkout", "-q", "side"});
    repo.index().read();
    auto upstream = repo.lookupRef("refs/heads/base").annotatedCommit();
    repo.rebase(upstream, "Override User", "override@example.test");
    auto author = repo.head().target().committer();
    std::cout << "REBASE state=" << repo.state()
              << " committer=" << author.name().toStdString() << " <"
              << author.email().toStdString()
              << "> expected=Override User <override@example.test>\n";
  }
  {
    QTemporaryDir tmp;
    auto repo = git::Repository::init(tmp.path());
    repo.gitConfig().setValue("user.name", QString("Configured User"));
    repo.gitConfig().setValue("user.email", QString("configured@example.test"));
    put(tmp.path(), "a", "base\n");
    runGit(tmp.path(), {"branch", "base"});
    runGit(tmp.path(), {"checkout", "-q", "-b", "one", "base"});
    put(tmp.path(), "one", "one\n");
    runGit(tmp.path(), {"checkout", "-q", "-b", "two", "base"});
    put(tmp.path(), "two", "two\n");
    runGit(tmp.path(), {"checkout", "-q", "base"});
    put(tmp.path(), "main", "main\n");
    repo.index().read();
    auto one = repo.lookupRef("refs/heads/one").annotatedCommit(),
         two = repo.lookupRef("refs/heads/two").annotatedCommit();
    bool first = repo.merge(one);
    repo.index().read();
    bool second = repo.merge(two);
    repo.index().read();
    auto merged = repo.commit("Second merge", two);
    std::cout << "NESTED_MERGE first=" << first << " second=" << second
              << " committed=" << merged.isValid() << " files="
              << runGit(tmp.path(), {"ls-tree", "--name-only", "HEAD"})
                     .replace('\n', ',')
                     .constData()
              << " second_parent="
              << merged.parents().value(1).shortId().toStdString()
              << " expected_one=" << one.commit().shortId().toStdString()
              << " wrong_two=" << two.commit().shortId().toStdString() << '\n';
  }
  {
    QTemporaryDir tmp;
    auto repo = git::Repository::init(tmp.path());
    repo.gitConfig().setValue("user.name", QString("Configured User"));
    repo.gitConfig().setValue("user.email", QString("configured@example.test"));
    put(tmp.path(), "a", "base\n");
    runGit(tmp.path(), {"branch", "base"});
    put(tmp.path(), "picked", "picked\n");
    auto picked = repo.head().target();
    runGit(tmp.path(), {"checkout", "-q", "base"});
    QFile f(tmp.path() + "/unrelated");
    if (f.open(QFile::WriteOnly)) {
      f.write("local staged work\n");
      f.close();
    }
    runGit(tmp.path(), {"add", "unrelated"});
    repo.index().read();
    bool success = repo.cherryPick(picked);
    auto committed = repo.commit("Cherry-pick");
    std::cout
        << "CHERRY_PICK_STAGED success=" << success
        << " committed=" << committed.isValid() << " files="
        << runGit(tmp.path(), {"ls-tree", "--name-only", "HEAD"})
               .replace('\n', ',')
               .constData()
        << " unrelated_cached_after="
        << runGit(tmp.path(), {"diff", "--cached", "--name-only"}).constData()
        << '\n';
  }
  {
    QTemporaryDir tmp;
    auto repo = git::Repository::init(tmp.path());
    repo.gitConfig().setValue("user.name", QString("Configured User"));
    repo.gitConfig().setValue("user.email", QString("configured@example.test"));
    put(tmp.path(), "a", "base\n");
    runGit(tmp.path(), {"branch", "base"});
    put(tmp.path(), "picked", "picked\n");
    auto picked = repo.head().target();
    runGit(tmp.path(), {"checkout", "-q", "base"});
    QFile f(tmp.path() + "/unrelated");
    if (f.open(QFile::WriteOnly)) {
      f.write("local staged work\n");
      f.close();
    }
    runGit(tmp.path(), {"add", "unrelated"});
    repo.index().read();
    bool success = repo.cherryPick(picked);
    bool aborted = repo.head().target().resetMerge();
    std::cout
        << "CHERRY_PICK_CANCEL success=" << success << " aborted=" << aborted
        << " unrelated_file_exists=" << QFile::exists(tmp.path() + "/unrelated")
        << " cached_after="
        << runGit(tmp.path(), {"diff", "--cached", "--name-only"}).constData()
        << '\n';
  }
  git::Repository::shutdown();
}
