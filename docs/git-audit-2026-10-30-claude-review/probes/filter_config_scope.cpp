#include "git/Cli.h"
#include "git/Config.h"
#include "git/Filter.h"
#include "git/Reference.h"
#include "git/Repository.h"
#include "git2.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

static void put(const QString &path, const QByteArray &data) {
  QFile f(path);
  if (!f.open(QFile::WriteOnly) || f.write(data) != data.size())
    qFatal("Cannot write probe fixture");
}

static QByteArray runGit(const QString &path, const QStringList &args) {
  auto result = git::Cli(path).run(args);
  if (result.exitCode)
    qFatal("Unexpected fixture command failure: %s", result.errors.constData());
  return result.output.trimmed();
}

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  QTemporaryDir global, work;
  if (!global.isValid() || !work.isValid())
    return 2;
  put(global.filePath(".gitconfig"), "[filter \"auditscope\"]\n"
                                     " clean = sed s/global/token/g\n"
                                     " smudge = sed s/token/global/g\n"
                                     " required = true\n");
  qputenv("GIT_CONFIG_GLOBAL", global.filePath(".gitconfig").toUtf8());
  qputenv("GIT_CONFIG_NOSYSTEM", "1");
  git_libgit2_init();
  git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_GLOBAL,
                   global.path().toUtf8().constData());
  git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_XDG,
                   global.path().toUtf8().constData());
  git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_SYSTEM, "");
  git::Filter::init();
  auto repo = git::Repository::init(work.path());
  if (!repo)
    return 2;
  repo.gitConfig().setValue("user.name", QString("Audit"));
  repo.gitConfig().setValue("user.email", QString("audit@example.test"));
  repo.gitConfig().setValue("filter.auditscope.clean",
                            QString("sed s/local/token/g"));
  repo.gitConfig().setValue("filter.auditscope.smudge",
                            QString("sed s/token/local/g"));
  put(work.filePath(".gitattributes"), "f filter=auditscope\n");
  put(work.filePath("f"), "token\n");
  runGit(work.path(), {"add", "."});
  runGit(work.path(), {"commit", "-qm", "base"});
  runGit(work.path(), {"branch", "base"});
  std::cout
      << "CLI_LOCAL_SMUDGE="
      << runGit(work.path(), {"cat-file", "--filters", "HEAD:f"}).constData()
      << '\n';
  git_repository *raw = nullptr;
  if (git_repository_open(&raw, work.path().toUtf8()))
    return 2;
  git_filter_list *filters = nullptr;
  if (git_filter_list_load(&filters, raw, nullptr, "f", GIT_FILTER_TO_WORKTREE,
                           GIT_FILTER_DEFAULT) ||
      !filters)
    return 2;
  git_buf out = GIT_BUF_INIT;
  int applied = git_filter_list_apply_to_buffer(&out, filters, "token\n", 6);
  std::cout << "PROJECT_FILTER_ERROR=" << applied << " PROJECT_SMUDGE="
            << QByteArray(out.ptr, out.size).trimmed().constData() << '\n';
  git_buf_dispose(&out);
  git_filter_list_free(filters);
  git_repository_free(raw);

  put(work.filePath("side"), "side\n");
  runGit(work.path(), {"add", "side"});
  runGit(work.path(), {"commit", "-qm", "side"});
  runGit(work.path(), {"branch", "side"});
  runGit(work.path(), {"checkout", "-q", "base"});
  put(work.filePath("main"), "main\n");
  put(work.filePath("f"), "token-upstream\n");
  runGit(work.path(), {"add", "main", "f"});
  runGit(work.path(), {"commit", "-qm", "main"});
  runGit(work.path(), {"checkout", "-q", "side"});
  runGit(work.path(), {"checkout", "HEAD", "--", "f"});
  repo.index().read();
  std::cout << "CLI_BEFORE_REBASE_STATUS="
            << runGit(work.path(), {"status", "--porcelain"}).constData()
            << '\n';
  bool initError = false;
  QObject::connect(repo.notifier(), &git::RepositoryNotifier::rebaseInitError,
                   [&] { initError = true; });
  repo.rebase(repo.lookupRef("refs/heads/base").annotatedCommit());
  std::cout << "PROJECT_REBASE_INIT_ERROR=" << initError
            << " state=" << repo.state()
            << " error=" << git::Repository::lastError().toStdString() << '\n';
  QFile result(work.filePath("f"));
  if (!result.open(QFile::ReadOnly))
    return 2;
  std::cout
      << "AFTER_PROJECT_REBASE_CONTENT="
      << result.readAll().trimmed().constData() << " CLI_EXPECTED="
      << runGit(work.path(), {"cat-file", "--filters", "HEAD:f"}).constData()
      << " CLI_STATUS="
      << runGit(work.path(), {"status", "--porcelain"}).constData() << '\n';
  if (initError) {
    auto native = git::Cli(work.path()).run({"rebase", "base"});
    std::cout << "CLI_REBASE_EXIT=" << native.exitCode << '\n';
  }
}
