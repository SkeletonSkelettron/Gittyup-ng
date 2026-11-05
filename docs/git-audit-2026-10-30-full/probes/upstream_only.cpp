#include "Test.h"
#include "git/Branch.h"
#include "git/Cli.h"
#include "git/Config.h"
#include "git/RefState.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/UndoHistory.h"
#include <iostream>

static std::string upstream(const git::Repository &repo) {
  auto ref = git::Branch(repo.head()).upstream();
  return ref.isValid() ? ref.name().toStdString() : "<none>";
}

int main(int argc, char **argv) {
  Application::setInTest();
  auto app = Test::createApp(argc, argv);
  Test::ignoreSystemConfig();
  QTemporaryDir dir;
  auto repo = git::Repository::init(dir.path());
  if (!repo)
    return 2;
  repo.gitConfig().setValue("user.name", QString("Audit"));
  repo.gitConfig().setValue("user.email", QString("audit@example.test"));
  if (git::Cli(dir.path()).run({"commit", "--allow-empty", "-qm", "base"}).exitCode)
    return 2;
  auto one = repo.createBranch("upstream-one", repo.head().target());
  auto two = repo.createBranch("upstream-two", repo.head().target());
  git::Branch(repo.head()).setUpstream(one);
  MainWindow window(repo);
  auto history = window.currentView()->undoHistory();
  git::RefState before(repo);
  std::cout << "upstream-before=" << upstream(repo) << '\n';
  git::Branch(repo.head()).setUpstream(two);
  git::RefState after(repo);
  history->check();
  std::cout << "upstream-after-change=" << upstream(repo)
            << " snapshots-equal=" << (before == after)
            << " changed-refs=" << before.changes(after).size()
            << " canUndo=" << history->canUndo() << '\n';
  bool undone = history->canUndo() && history->undo();
  std::cout << "upstream-undo=" << undone
            << " resulting-upstream=" << upstream(repo)
            << " expected-if-supported=upstream-one\n";
}
