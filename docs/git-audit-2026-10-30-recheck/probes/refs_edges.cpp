#include "Test.h"
#include "git/Branch.h"
#include "git/Cli.h"
#include "git/Config.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/UndoHistory.h"
#include <QFile>
#include <iostream>

static git::RunResult run(git::Repository repo, const QStringList &args) {
  return git::Cli(repo.workdir().path()).run(args);
}
static void write(git::Repository repo, const QByteArray &contents) {
  QFile file(repo.workdir().filePath("file.txt"));
  if (!file.open(QFile::WriteOnly))
    abort();
  file.write(contents);
}
int main(int argc, char **argv) {
  Application::setInTest();
  auto app = Test::createApp(argc, argv);
  Test::ignoreSystemConfig();
  QTemporaryDir dir, other;
  auto repo = git::Repository::init(dir.path());
  repo.gitConfig().setValue("user.name", QString("Audit"));
  repo.gitConfig().setValue("user.email", QString("audit@example.invalid"));
  write(repo, "A\n");
  run(repo, {"add", "file.txt"});
  run(repo, {"commit", "-m", "A"});
  auto upstream = repo.createBranch("upstream", repo.head().target());
  auto mainName = repo.head().name();
  MainWindow window(repo);
  auto view = window.currentView();
  auto history = view->undoHistory();
  view->createBranch("unrelated", repo.head().target());
  history->check();
  git::Branch(repo.head()).setUpstream(upstream);
  history->check();
  std::cout << "upstream-before: upstream="
            << git::Branch(repo.head()).upstream().name().toStdString()
            << " undo=" << history->undoText().toStdString() << std::endl;
  bool undone = history->undo();
  auto after = git::Branch(repo.head()).upstream();
  std::cout << "upstream-after-unrelated-undo: undo=" << undone
            << " unrelated-exists=" << repo.lookupBranch("unrelated").isValid()
            << " upstream="
            << (after.isValid() ? after.name().toStdString() : "<none>")
            << std::endl;

  write(repo, "B\n");
  run(repo, {"add", "file.txt"});
  run(repo, {"commit", "-m", "B"});
  history->check();
  QString path = other.path() + "/linked";
  auto add = run(repo, {"worktree", "add", path, "upstream"});
  auto blocked = run(repo, {"switch", "upstream"});
  view->checkout(repo.lookupBranch("upstream"));
  auto linkedHead = git::Cli(path).run({"symbolic-ref", "--short", "HEAD"});
  std::cout << "ordinary-checkout-occupied: worktree-add=" << add.exitCode
            << " git-switch=" << blocked.exitCode
            << " primary-head=" << repo.head().name().toStdString()
            << " linked-head=" << linkedHead.output.trimmed().toStdString()
            << std::endl;
  write(repo, "C\n");
  run(repo, {"add", "file.txt"});
  run(repo, {"commit", "-m", "C"});
  std::cout
      << "linked-after-primary-commit: status="
      << git::Cli(path).run({"status", "--porcelain"}).output.toStdString()
      << std::endl;
}
