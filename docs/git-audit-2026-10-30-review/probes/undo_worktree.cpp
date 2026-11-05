#include "Test.h"
#include "git/Branch.h"
#include "git/Cli.h"
#include "git/Config.h"
#include "git/RefState.h"
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
static QByteArray read(git::Repository repo) {
  QFile file(repo.workdir().filePath("file.txt"));
  if (!file.open(QFile::ReadOnly))
    abort();
  return file.readAll();
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
  auto topic = repo.createBranch("topic", repo.head().target());
  auto mainName = repo.head().name();
  write(repo, "B\n");
  run(repo, {"add", "file.txt"});
  run(repo, {"commit", "-m", "B"});
  MainWindow window(repo);
  auto view = window.currentView();
  auto history = view->undoHistory();
  if (qEnvironmentVariableIsSet("AUDIT_DETACHED"))
    view->checkout(topic.target(), git::Reference(), true);
  else
    view->checkout(topic);
  history->check();
  std::cout << "checkout-before: head=" << repo.head().name().toStdString()
            << " file=" << read(repo).trimmed().toStdString() << " status="
            << run(repo, {"status", "--porcelain"}).output.toStdString()
            << " undo=" << history->undoText().toStdString() << std::endl;
  auto add = run(repo, {"worktree", "add", other.path() + "/linked", mainName});
  bool undone = history->undo();
  std::cout << "checkout-undo-occupied: worktree-add=" << add.exitCode
            << " undo=" << undone
            << " head=" << repo.head().name().toStdString()
            << " file=" << read(repo).trimmed().toStdString() << " index="
            << run(repo, {"show", ":file.txt"}).output.trimmed().toStdString()
            << " status="
            << run(repo, {"status", "--porcelain"}).output.toStdString()
            << std::endl;
}
