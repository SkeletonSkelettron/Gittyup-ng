#include "Test.h"
#include "git/Branch.h"
#include "git/Cli.h"
#include "git/Config.h"
#include "git/Remote.h"
#include "log/LogEntry.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/UndoHistory.h"
#include <QFile>
#include <iostream>

static void require(bool value, const char *where) {
  if (!value) { std::cerr << "SETUP FAILED: " << where << std::endl; abort(); }
}
static git::RunResult run(git::Repository repo, const QStringList &args) {
  return git::Cli(repo.workdir().path()).run(args);
}
static void write(const QString &path, const QByteArray &contents) {
  QFile file(path);
  require(file.open(QFile::WriteOnly), "write");
  file.write(contents);
}
static QByteArray read(const QString &path) {
  QFile file(path);
  require(file.open(QFile::ReadOnly), "read");
  return file.readAll();
}
static git::Repository fresh(QTemporaryDir &dir) {
  auto repo = git::Repository::init(dir.path());
  require(repo.isValid(), "init");
  repo.gitConfig().setValue("user.name", QString("Audit"));
  repo.gitConfig().setValue("user.email", QString("audit@example.invalid"));
  repo.appConfig().setValue("autofetch.enable", false);
  write(repo.workdir().filePath("file.txt"), "A\n");
  require(run(repo, {"add", "file.txt"}).exitCode == 0, "add A");
  require(run(repo, {"commit", "-m", "A"}).exitCode == 0, "commit A");
  return repo;
}
static LogEntry *latest(RepoView *view) {
  for (auto entry : view->findChildren<LogEntry *>())
    if (!qobject_cast<LogEntry *>(entry->parent())) return entry->lastEntry();
  return nullptr;
}
static void wait(LogEntry *entry) {
  require(entry, "latest log");
  for (int i = 0; i < 500 && entry->progress() >= 0; ++i) QTest::qWait(10);
  require(entry->progress() < 0, "operation finished");
  QTest::qWait(10);
}
static bool contains(LogEntry *entry, const QString &text) {
  if (entry->text().contains(text)) return true;
  for (auto child : entry->entries()) if (contains(child, text)) return true;
  return false;
}
static void stagedRollback() {
  QTemporaryDir dir;
  auto repo = fresh(dir);
  QString main = repo.head().name();
  auto topic = repo.createBranch("topic", repo.head().target());
  QString path = repo.workdir().filePath("file.txt");
  write(path, "B\n");
  require(run(repo, {"add", "file.txt"}).exitCode == 0, "add B");
  require(run(repo, {"commit", "-m", "B"}).exitCode == 0, "commit B");
  MainWindow window(repo);
  auto view = window.currentView();
  view->checkout(topic);
  auto history = view->undoHistory();
  history->check();
  // A local staged change happens to match the undo destination. A safe
  // forward checkout accepts it; rolling back to HEAD is not its inverse.
  write(path, "B\n");
  require(run(repo, {"add", "file.txt"}).exitCode == 0, "stage local B");
  auto beforeFile = read(path).trimmed();
  auto beforeIndex = run(repo, {"show", ":file.txt"}).output.trimmed();
  auto beforeStatus = run(repo, {"status", "--porcelain"}).output.trimmed();
  auto lockPath = repo.dir().filePath("HEAD.lock");
  write(lockPath, "another git process\n");
  auto native = run(repo, {"switch", main});
  std::cout << "STAGED_ROLLBACK_NATIVE exit=" << native.exitCode
            << " head=" << repo.head().name().toStdString()
            << " file=" << read(path).trimmed().toStdString()
            << " index=" << run(repo, {"show", ":file.txt"}).output.trimmed().toStdString()
            << " status=" << run(repo, {"status", "--porcelain"}).output.trimmed().toStdString()
            << std::endl;
  require(read(path).trimmed() == beforeFile &&
              run(repo, {"show", ":file.txt"}).output.trimmed() == beforeIndex &&
              run(repo, {"status", "--porcelain"}).output.trimmed() == beforeStatus,
          "native control preserved staged B fixture");
  bool undone = history->undo();
  std::cout << "STAGED_ROLLBACK before_file=" << beforeFile.toStdString()
            << " before_index=" << beforeIndex.toStdString()
            << " before_status=" << beforeStatus.toStdString()
            << " undo=" << undone << " head=" << repo.head().name().toStdString()
            << " after_file=" << read(path).trimmed().toStdString()
            << " after_index=" << run(repo, {"show", ":file.txt"}).output.trimmed().toStdString()
            << " after_status=" << run(repo, {"status", "--porcelain"}).output.trimmed().toStdString()
            << std::endl;
  QFile::remove(lockPath);
}
static void partialPush() {
  QTemporaryDir dir, remoteDir;
  auto repo = fresh(dir);
  auto bare = git::Repository::init(remoteDir.path(), true);
  auto remote = repo.addRemote("origin", bare.workdir().path());
  QString main = repo.head().name();
  require(run(repo, {"tag", "rejected-tag"}).exitCode == 0, "create tag");
  auto hook = bare.dir().filePath("hooks/update");
  write(hook, "#!/bin/sh\ncase \"$1\" in refs/tags/*) echo tag-denied >&2; exit 1;; esac\nexit 0\n");
  require(QFile::setPermissions(hook, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner), "hook permissions");
  auto native = run(repo, {"push", "-u", "origin", main, "refs/tags/rejected-tag"});
  bool nativeUp = git::Branch(repo.head()).upstream().isValid();
  git::Branch(repo.head()).setUpstream(git::Branch());
  require(run(bare, {"update-ref", "-d", "refs/heads/" + main}).exitCode == 0, "clear remote branch");
  require(run(repo, {"update-ref", "-d", "refs/remotes/origin/" + main}).exitCode == 0, "clear tracking branch");
  MainWindow window(repo);
  auto view = window.currentView();
  view->push(remote, repo.head(), QString(), true, false, true);
  auto entry = latest(view);
  wait(entry);
  std::cout << "PARTIAL_PUSH native_exit=" << native.exitCode
            << " native_upstream_set=" << nativeUp
            << " project_branch_pushed=" << bare.lookupBranch(main).isValid()
            << " project_tag_pushed=" << !run(bare, {"show-ref", "--verify", "refs/tags/rejected-tag"}).exitCode
            << " project_upstream_set=" << git::Branch(repo.head()).upstream().isValid()
            << " logged_rejection=" << contains(entry, "[remote rejected]")
            << std::endl;
}
int main(int argc, char **argv) {
  Application::setInTest();
  auto app = Test::createApp(argc, argv);
  Test::ignoreSystemConfig();
  stagedRollback();
  partialPush();
}
