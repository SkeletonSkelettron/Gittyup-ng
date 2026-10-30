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
int main(int argc, char **argv) {
  Application::setInTest();
  auto app = Test::createApp(argc, argv);
  Test::ignoreSystemConfig();
  stagedRollback();
}
