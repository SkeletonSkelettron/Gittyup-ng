#include "Test.h"
#include "git/Branch.h"
#include "git/Cli.h"
#include "git/Config.h"
#include "git/Remote.h"
#include "log/LogEntry.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/UndoHistory.h"
#include <QElapsedTimer>
#include <QFile>
#include <iostream>

static void require(bool value, const char *where) {
  if (!value) {
    std::cerr << "SETUP FAILED: " << where << std::endl;
    abort();
  }
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
static git::Repository fresh(QTemporaryDir &dir, bool initial = true) {
  auto repo = git::Repository::init(dir.path());
  require(repo.isValid(), "init");
  repo.gitConfig().setValue("user.name", QString("Audit"));
  repo.gitConfig().setValue("user.email", QString("audit@example.invalid"));
  repo.appConfig().setValue("autofetch.enable", false);
  if (initial) {
    write(repo.workdir().filePath("file.txt"), "A\n");
    require(run(repo, {"add", "file.txt"}).exitCode == 0, "add A");
    require(run(repo, {"commit", "-m", "A"}).exitCode == 0, "commit A");
  }
  return repo;
}
static LogEntry *latest(RepoView *view) {
  for (auto entry : view->findChildren<LogEntry *>())
    if (!qobject_cast<LogEntry *>(entry->parent()))
      return entry->lastEntry();
  return nullptr;
}
static void wait(LogEntry *entry) {
  require(entry, "latest log");
  for (int i = 0; i < 500 && entry->progress() >= 0; ++i)
    QTest::qWait(10);
  require(entry->progress() < 0, "operation finished");
  QTest::qWait(10);
}
static bool contains(LogEntry *entry, const QString &text) {
  if (entry->text().contains(text))
    return true;
  for (auto child : entry->entries())
    if (contains(child, text))
      return true;
  return false;
}
static void firstCommitUndo() {
  QTemporaryDir dir;
  auto repo = fresh(dir, false);
  MainWindow window(repo);
  auto view = window.currentView();
  auto history = view->undoHistory();
  write(repo.workdir().filePath("file.txt"), "first\n");
  repo.index().setStaged({"file.txt"}, true);
  require(view->commit(QString("first")), "commit first");
  history->check();
  bool offered = history->canUndo();
  bool undone = history->undo();
  std::cout << "FIRST_COMMIT_UNDO offered=" << offered << " undo=" << undone
            << " unborn=" << repo.isHeadUnborn() << " cached="
            << run(repo, {"diff", "--cached", "--name-only"})
                   .output.trimmed()
                   .toStdString()
            << std::endl;
}
static void headLockUndo() {
  QTemporaryDir dir;
  auto repo = fresh(dir);
  QString main = repo.head().name();
  auto topic = repo.createBranch("topic", repo.head().target());
  write(repo.workdir().filePath("file.txt"), "B\n");
  require(run(repo, {"add", "file.txt"}).exitCode == 0, "add B");
  require(run(repo, {"commit", "-m", "B"}).exitCode == 0, "commit B");
  MainWindow window(repo);
  auto view = window.currentView();
  view->checkout(topic);
  auto history = view->undoHistory();
  history->check();
  QString before =
      QString::fromUtf8(read(repo.workdir().filePath("file.txt"))).trimmed();
  QString beforeStatus =
      QString::fromUtf8(run(repo, {"status", "--porcelain"}).output).trimmed();
  QString lockPath = repo.dir().filePath("HEAD.lock");
  write(lockPath, "another git process\n");
  auto native = run(repo, {"switch", main});
  std::cout
      << "HEAD_LOCK_NATIVE exit=" << native.exitCode << " file="
      << read(repo.workdir().filePath("file.txt")).trimmed().toStdString()
      << " status="
      << run(repo, {"status", "--porcelain"}).output.trimmed().toStdString()
      << std::endl;
  // Restore A independently: native switch also partially changes the
  // checkout when the later HEAD lock fails.
  require(run(repo, {"read-tree", "--reset", "-u", "HEAD"}).exitCode == 0,
          "restore fixture after native control");
  require(read(repo.workdir().filePath("file.txt")) == "A\n" &&
              run(repo, {"status", "--porcelain"}).output.isEmpty(),
          "clean A fixture before UI undo");
  bool undone = history->undo();
  std::cout
      << "HEAD_LOCK_UNDO before=" << before.toStdString()
      << " before_status=" << beforeStatus.toStdString() << " undo=" << undone
      << " head=" << repo.head().name().toStdString() << " file="
      << read(repo.workdir().filePath("file.txt")).trimmed().toStdString()
      << " index="
      << run(repo, {"show", ":file.txt"}).output.trimmed().toStdString()
      << " status="
      << run(repo, {"status", "--porcelain"}).output.trimmed().toStdString()
      << std::endl;
  QFile::remove(lockPath);
}
static void stalePush() {
  QTemporaryDir dir, remoteDir;
  auto repo = fresh(dir);
  auto bare = git::Repository::init(remoteDir.path(), true);
  auto remote = repo.addRemote("origin", bare.workdir().path());
  QString main = repo.head().name();
  require(run(repo, {"push", "-u", "origin", main}).exitCode == 0,
          "seed origin");
  require(git::Branch(repo.head()).upstream().isValid(), "seed upstream");
  MainWindow window(repo);
  auto view = window.currentView();
  require(run(bare, {"update-ref", "-d", "refs/heads/" + main}).exitCode == 0,
          "delete remote ref");
  view->push(remote);
  auto entry = latest(view);
  wait(entry);
  std::cout << "STALE_PUSH remote_branch_exists="
            << bare.lookupBranch(main).isValid() << " logged_up_to_date="
            << contains(entry, "Everything up-to-date.") << " tracking_exists="
            << git::Branch(repo.head()).upstream().isValid() << std::endl;
  auto native = run(repo, {"push", "origin", main});
  std::cout << "STALE_PUSH_NATIVE exit=" << native.exitCode
            << " remote_branch_exists=" << bare.lookupBranch(main).isValid()
            << std::endl;
}
static void rejectedPush() {
  QTemporaryDir dir, remoteDir;
  auto repo = fresh(dir);
  auto bare = git::Repository::init(remoteDir.path(), true);
  auto remote = repo.addRemote("origin", bare.workdir().path());
  QString main = repo.head().name();
  require(run(repo, {"push", "origin", main}).exitCode == 0,
          "seed untracked origin");
  require(!git::Branch(repo.head()).upstream().isValid(),
          "no initial upstream");
  QString hookPath = bare.dir().filePath("hooks/pre-receive");
  write(hookPath, "#!/bin/sh\necho denied-by-audit-hook >&2\nexit 1\n");
  require(QFile::setPermissions(hookPath, QFile::ReadOwner | QFile::WriteOwner |
                                              QFile::ExeOwner),
          "hook permissions");
  require(run(repo, {"commit", "--allow-empty", "-m", "B"}).exitCode == 0,
          "commit B empty");
  auto before = bare.lookupBranch(main).target().id();
  auto native = run(repo, {"push", "-u", "origin", main});
  std::cout << "REJECTED_PUSH_NATIVE exit=" << native.exitCode
            << " upstream_set=" << git::Branch(repo.head()).upstream().isValid()
            << std::endl;
  MainWindow window(repo);
  auto view = window.currentView();
  view->push(remote);
  auto entry = latest(view);
  wait(entry);
  std::cout << "REJECTED_PUSH remote_unchanged="
            << (bare.lookupBranch(main).target().id() == before)
            << " upstream_set=" << git::Branch(repo.head()).upstream().isValid()
            << " logged_rejection=" << contains(entry, "[remote rejected]")
            << std::endl;
}
static void canceledFetch() {
  QTemporaryDir dir, remoteDir, scripts;
  auto repo = fresh(dir, false);
  auto source = fresh(remoteDir);
  auto remote = repo.addRemote("origin", source.workdir().path());
  require(bool(remote.fetch(nullptr)), "seed fetch");
  QString main = source.head().name();
  write(source.workdir().filePath("file.txt"), "B\n");
  require(run(source, {"add", "file.txt"}).exitCode == 0, "add remote B");
  require(run(source, {"commit", "-m", "B"}).exitCode == 0, "commit remote B");
  QString marker = scripts.path() + "/started";
  QString helper = scripts.path() + "/slow-upload-pack";
  // QTemporaryDir generates a path with no shell metacharacters. Quote it
  // anyway; the remote path itself arrives through the positional arguments.
  write(helper, ("#!/bin/sh\n: > '" + marker +
                 "'\nsleep 2\nexec git upload-pack \"$@\"\n")
                    .toUtf8());
  require(QFile::setPermissions(helper, QFile::ReadOwner | QFile::WriteOwner |
                                            QFile::ExeOwner),
          "upload-pack permissions");
  repo.gitConfig().setValue("remote.origin.uploadpack", helper);
  MainWindow window(repo);
  auto view = window.currentView();
  view->fetch(remote);
  auto entry = latest(view);
  for (int i = 0; i < 500 && !QFile::exists(marker); ++i)
    QTest::qWait(10);
  require(QFile::exists(marker), "upload-pack started");
  QElapsedTimer timer;
  timer.start();
  view->cancelRemoteTransfer();
  qint64 elapsed = timer.elapsed();
  wait(entry);
  auto tracking = repo.lookupBranch("origin/" + main, GIT_BRANCH_REMOTE);
  std::cout << "CANCELED_FETCH cancel_wait_ms=" << elapsed
            << " tracking_updated="
            << (tracking.target().id() == source.head().target().id())
            << " logged_canceled=" << contains(entry, "Fetch canceled.")
            << std::endl;
}
int main(int argc, char **argv) {
  Application::setInTest();
  auto app = Test::createApp(argc, argv);
  Test::ignoreSystemConfig();
  firstCommitUndo();
  headLockUndo();
  stalePush();
  rejectedPush();
  canceledFetch();
}
