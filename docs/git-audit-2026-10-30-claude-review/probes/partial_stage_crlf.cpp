#include "Test.h"
#include "git/Repository.h"
#include "git/Index.h"
#include "git/Config.h"
#include "git/Cli.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/DiffModel.h"
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
  QTemporaryDir temp;
  QString dir = temp.path();
  auto repo = git::Repository::init(dir);
  repo.gitConfig().setValue("user.name", QString("Test"));
  repo.gitConfig().setValue("user.email", QString("t@e"));
  write(dir + "/.gitattributes", "*.txt text eol=lf\n");
  run(dir, {"add", "."});
  run(dir, {"commit", "-qm", "base"});
  write(dir + "/file.txt", "first\r\nsecond\r\n");
  auto index = repo.index();
  index.read();
  auto diff = repo.status(index, nullptr, false);
  MainWindow window(repo);
  auto view = window.currentView();
  DiffModel model(view);
  model.setDiff(diff, "file.txt");
  fprintf(stderr, "hunks=%d rows=%d line0_hex=%s\n", model.hunkCount(),
          model.rowCount(), model.lines(0).first().content.toHex().constData());
  model.toggleLine(model.row(0, 0));
  for (int i = 0; i < 10; ++i)
    QTest::qWait(10);
  fprintf(stderr, "partial_staged_hex=%s\n",
          run(dir, {"show", ":file.txt"}).output.toHex().constData());
  run(dir, {"add", "file.txt"});
  fprintf(stderr, "normal_staged_hex=%s\n",
          run(dir, {"show", ":file.txt"}).output.toHex().constData());
}
