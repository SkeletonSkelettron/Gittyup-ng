// Diagnostic: required clean failure while partially staging untracked text.
// The current Debug implementation aborts inside Patch::apply. A future safe
// rejection must reach stage_attempt_returned without creating an index blob.
#include "Test.h"
#include "git/Repository.h"
#include "git/Index.h"
#include "git/Config.h"
#include "git/Cli.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/DiffModel.h"
#include "git/Filter.h"
#include "git2/global.h"
#include "git2/config.h"
#include <QFile>
#include <cstdio>
#include <cstdlib>
void write(const QString &p, const QByteArray &c) {
  QFile f(p);
  if (!f.open(QFile::WriteOnly) || f.write(c) != c.size())
    std::abort();
}
git::RunResult run(const QString &d, const QStringList &a) {
  auto r = git::Cli(d).run(a);
  if (r.exitCode) {
    fprintf(stderr, "git failed %s\n", r.errors.constData());
    std::abort();
  }
  return r;
}
int main(int argc, char **argv) {
  Application::setInTest();
  auto app = Test::createApp(argc, argv);
  Test::ignoreSystemConfig();
  QTemporaryDir global;
  write(global.filePath(".gitconfig"),
        "[filter \"auditrequired\"]\n clean = false\n smudge = cat\n required "
        "= true\n");
  qputenv("GIT_CONFIG_GLOBAL", global.filePath(".gitconfig").toUtf8());
  git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_GLOBAL,
                   global.path().toUtf8().constData());
  git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, GIT_CONFIG_LEVEL_XDG,
                   global.path().toUtf8().constData());
  git::Filter::init();
  QTemporaryDir temp;
  QString dir = temp.path();
  auto repo = git::Repository::init(dir);
  repo.gitConfig().setValue("user.name", QString("Test"));
  repo.gitConfig().setValue("user.email", QString("t@e"));
  write(dir + "/.gitattributes", "file.txt filter=auditrequired\n");
  run(dir, {"add", "."});
  run(dir, {"commit", "-qm", "base"});
  write(dir + "/file.txt", "first\nsecond\n");
  auto native = git::Cli(dir).run({"add", "file.txt"});
  fprintf(stderr, "native_add_exit=%d cached_paths=%s", native.exitCode,
          git::Cli(dir)
              .run({"diff", "--cached", "--name-only"})
              .output.constData());
  auto cleaned = repo.cleanContent("file.txt", "first\nsecond\n");
  fprintf(stderr, "cleanContent_has_result=%d\n", cleaned.has_value());
  auto index = repo.index();
  index.read();
  auto diff = repo.status(index, nullptr, false);
  MainWindow window(repo);
  auto view = window.currentView();
  DiffModel model(view);
  model.setDiff(diff, "file.txt");
  fprintf(stderr, "hunks=%d rows=%d line0_hex=%s\n", model.hunkCount(),
          model.rowCount(), model.lines(0).first().content.toHex().constData());
  fprintf(stderr, "editable_before_attempt=%d notice=%s\n", model.isEditable(),
          model.notice().toUtf8().constData());
  model.toggleLine(model.row(0, 0));
  for (int i = 0; i < 10; ++i)
    QTest::qWait(10);
  auto staged = git::Cli(dir).run({"show", ":file.txt"});
  fprintf(stderr,
          "stage_attempt_returned=1 cached_blob_exit=%d cached_blob_hex=%s\n",
          staged.exitCode, staged.output.toHex().constData());
}
