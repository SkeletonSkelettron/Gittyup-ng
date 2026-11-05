#include "git/Filter.h"
#include "git2.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <cstdio>

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  QTemporaryDir tmp;
  if (!tmp.isValid())
    return 2;
  QFile config(tmp.filePath(".gitconfig"));
  if (!config.open(QFile::WriteOnly))
    return 2;
  config.write("[filter \"auditoptional\"]\n clean = cat\n smudge = false\n "
               "required = false\n");
  config.close();
  git_libgit2_init();
  git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_GLOBAL,
                   tmp.path().toUtf8().constData());
  git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_XDG,
                   tmp.path().toUtf8().constData());
  git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_SYSTEM, "");
  git::Filter::init();
  QString repoPath = tmp.filePath("repo");
  git_repository *repo = nullptr;
  if (git_repository_init(&repo, repoPath.toUtf8(), false))
    return 2;
  QFile attrs(repoPath + "/.gitattributes");
  if (!attrs.open(QFile::WriteOnly))
    return 2;
  attrs.write("f filter=auditoptional\n");
  attrs.close();
  git_filter_list *filters = nullptr;
  int loaded = git_filter_list_load(&filters, repo, nullptr, "f",
                                    GIT_FILTER_TO_WORKTREE, GIT_FILTER_DEFAULT);
  fprintf(stderr,
          "OPTIONAL_FILTER load=%d loaded=%d; expected unchanged input on "
          "smudge exit 1\n",
          loaded, filters != nullptr);
  fflush(stderr);
  git_buf out = GIT_BUF_INIT;
  int applied = git_filter_list_apply_to_buffer(&out, filters, "original\n", 9);
  fprintf(stderr, "OPTIONAL_FILTER apply=%d content=%.*s\n", applied,
          int(out.size), out.ptr);
  git_buf_dispose(&out);
  git_filter_list_free(filters);
  git_repository_free(repo);
  git_libgit2_shutdown();
  return 0;
}
