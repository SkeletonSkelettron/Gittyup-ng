#include "Test.h"
#include "git/Branch.h"
#include "git/Cli.h"
#include "git/Config.h"
#include "git/RefState.h"
#include "git/Remote.h"
#include "git/TagRef.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/UndoHistory.h"
#include <QFile>
#include <iostream>

static git::RunResult run(git::Repository repo, const QStringList &args) {
  return git::Cli(repo.workdir().path()).run(args);
}
static git::Repository fresh(QTemporaryDir &dir) {
  auto repo = git::Repository::init(dir.path());
  repo.gitConfig().setValue("user.name", QString("Audit"));
  repo.gitConfig().setValue("user.email", QString("audit@example.invalid"));
  run(repo, {"commit", "--allow-empty", "-m", "initial"});
  return repo;
}
int main(int argc, char **argv) {
  Application::setInTest();
  auto app = Test::createApp(argc, argv);
  Test::ignoreSystemConfig();
  QTemporaryDir a, b, c;
  auto local = fresh(a);
  auto origin = fresh(b);
  auto backup = git::Repository::init(c.path(), true);
  local.addRemote("origin", origin.workdir().path());
  local.addRemote("backup", backup.workdir().path());
  auto mainName = local.head().name();
  git::RefState before(local);
  local.gitConfig().setValue(QString("branch.%1.remote").arg(mainName),
                             QString("origin"));
  local.gitConfig().setValue(QString("branch.%1.merge").arg(mainName),
                             QString("refs/heads/trunk"));
  git::RefState after(local);
  std::cout << "upstream snapshots equal=" << (before == after)
            << " changed refs=" << before.changes(after).size() << std::endl;

  auto pushResult = local.lookupRemote("backup").push(nullptr, local.head());
  std::cout << "push-to-backup result=" << pushResult.error()
            << " source=" << mainName.toStdString()
            << " backup-trunk=" << backup.lookupBranch("trunk").isValid()
            << " backup-source=" << backup.lookupBranch(mainName).isValid()
            << std::endl;

  auto localUpstream =
      local.createBranch("local-upstream", local.head().target());
  MainWindow window(local);
  auto view = window.currentView();
  auto history = view->undoHistory();
  git::Branch(local.head()).setUpstream(localUpstream);
  history->check();
  std::cout << "set-upstream="
            << git::Branch(local.head()).upstream().name().toStdString()
            << " canUndo=" << history->canUndo() << std::endl;
  run(local, {"commit", "--allow-empty", "-m", "second"});
  history->check();
  std::cout << "before fetch undo=" << history->undoText().toStdString()
            << std::endl;
  run(origin, {"tag", "tag-fetched-after-action"});
  auto fetchResult = local.lookupRemote("origin").fetch(nullptr, true);
  history->check();
  std::cout << "after fetch undo=" << history->undoText().toStdString()
            << std::endl;
  bool hadTag = local.lookupTag("tag-fetched-after-action").isValid();
  bool undone = history->undo();
  std::cout << "fetch=" << fetchResult.error()
            << " fetched-tag-before-undo=" << hadTag << " undo=" << undone
            << " fetched-tag-after-undo="
            << local.lookupTag("tag-fetched-after-action").isValid()
            << " actual-tags="
            << run(local, {"tag", "--list"}).output.toStdString() << std::endl;
  QTemporaryDir linkedRoot;
  QString linkedPath = linkedRoot.path() + "/linked";
  run(local, {"worktree", "add", "-b", "linked", linkedPath});
  QFile file(local.workdir().filePath("only-in-new-commit.txt"));
  if (!file.open(QFile::WriteOnly))
    return 1;
  file.write("tracked content\n");
  file.close();
  run(local, {"add", "only-in-new-commit.txt"});
  run(local, {"commit", "-m", "new file"});
  auto target = local.head().target();
  auto blocked = run(local, {"branch", "-f", "linked", target.id().toString()});
  auto moved =
      local.lookupBranch("linked").setTarget(target, "audit fast-forward");
  auto status = git::Cli(linkedPath).run({"status", "--porcelain"});
  std::cout << "linked-worktree git-branch-force-exit=" << blocked.exitCode
            << " setTarget-success=" << moved.isValid()
            << " linked-file-exists="
            << QFile::exists(linkedPath + "/only-in-new-commit.txt")
            << " linked-status=" << status.output.toStdString() << std::endl;
  auto feature = local.createBranch("feature", local.head().target());
  history->check();
  auto renamed = feature.rename("feature/nested");
  history->check();
  bool renameUndo = history->undo();
  std::cout << "nested-rename=" << renamed.isValid() << " undo=" << renameUndo
            << " old-name-exists=" << local.lookupBranch("feature").isValid()
            << " renamed-exists="
            << local.lookupBranch("feature/nested").isValid() << std::endl;
  return 0;
}
