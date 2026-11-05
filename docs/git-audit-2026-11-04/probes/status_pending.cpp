// Review probe: pending refreshes must stay responsive and publish latest state.
// Only temporary repositories and helper scripts are written.
#include "Test.h"
#include "git/Config.h"
#include "git/Commit.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/CommitList.h"
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTimer>
#include <QThread>
#include <functional>
#include <cstdio>
#include <cstdlib>

static void require(bool ok, const char *message) {
  if (!ok) { fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
}
static void writeFile(const QString &path, const QByteArray &content) {
  QFile f(path);
  require(f.open(QFile::WriteOnly) && f.write(content) == content.size(), "write");
}
static int countLines(const QString &path) {
  QFile f(path); if (!f.open(QFile::ReadOnly)) return 0;
  return f.readAll().count('\n');
}
static bool until(const std::function<bool()> &predicate, int timeout = 10000) {
  QElapsedTimer timer; timer.start();
  while (!predicate() && timer.elapsed() < timeout) QTest::qWait(5);
  return predicate();
}
int main(int argc, char **argv) {
  QTemporaryDir helpers;
  require(helpers.isValid(), "helpers");
  const QString wrapper = helpers.filePath("git");
  writeFile(wrapper,
    "#!/bin/sh\n"
    "status=0\nfor arg in \"$@\"; do [ \"$arg\" = status ] && status=1; done\n"
    "if [ \"$status\" = 1 ] && [ -f \"$AUDIT_STATUS_DIR/delay\" ]; then\n"
    " echo start >> \"$AUDIT_STATUS_DIR/starts\"\n"
    " sleep 1\n"
    " /usr/bin/git \"$@\"\n"
    " result=$?\n"
    " echo end >> \"$AUDIT_STATUS_DIR/ends\"\n"
    " exit \"$result\"\nfi\nexec /usr/bin/git \"$@\"\n");
  require(QFile::setPermissions(wrapper, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner), "chmod");
  qputenv("AUDIT_STATUS_DIR", helpers.path().toUtf8());
  qputenv("PATH", helpers.path().toUtf8() + ":" + qgetenv("PATH"));
  Application::setInTest();
  auto app = Test::createApp(argc, argv);
  Test::ignoreSystemConfig();
  Test::ScratchRepository scratch;
  git::Repository repo = scratch;
  repo.appConfig().setValue("index.enable", false);
  const QString file = repo.workdir().filePath("file.txt");
  writeFile(file, "base\n");
  repo.index().setStaged({"file.txt"}, true);
  require(repo.commit("base").isValid(), "base commit");
  MainWindow window(repo);
  RepoView *view = window.currentView();
  Test::refresh(view, false);
  QSignalSpy finished(view, &RepoView::statusChanged);
  QTimer heartbeat;
  int ticks = 0;
  QObject::connect(&heartbeat, &QTimer::timeout, [&] { ++ticks; });
  heartbeat.start(10);

  writeFile(helpers.filePath("delay"), "1");
  writeFile(file, "transient\n");
  view->refresh();
  require(until([&] { return countLines(helpers.filePath("starts")) >= 1; }), "first scan starts");
  QElapsedTimer burst; burst.start();
  for (int i = 0; i < 20; ++i) view->refresh();
  const qint64 burstMs = burst.elapsed();
  writeFile(file, "base\n");
  writeFile(repo.workdir().filePath("final.txt"), "latest\n");
  view->refresh();
  const int ticksBefore = ticks;
  QTest::qWait(300);
  const int activeTicks = ticks - ticksBefore;
  require(until([&] { return !finished.isEmpty() && !view->isLoading(); }), "burst finishes");
  auto diff = view->commitList()->status();
  require(diff && diff.count() == 1 && diff.name(0) == "final.txt", "latest burst status");
  require(burstMs < 500 && activeTicks >= 10, "main loop responsive");
  fprintf(stderr, "BURST requests=21 refresh_ms=%lld active_300ms_ticks=%d status_count=%d latest=%s scans=%d\n",
          static_cast<long long>(burstMs), activeTicks, finished.count(),
          qPrintable(diff.name(0)), countLines(helpers.filePath("starts")));

  // Let a canceled worker finish without processing its finished signal,
  // then deliver one more refresh at that boundary.
  finished.clear();
  const int started = countLines(helpers.filePath("starts"));
  const int ended = countLines(helpers.filePath("ends"));
  view->refresh();
  require(until([&] { return countLines(helpers.filePath("starts")) > started; }), "boundary scan starts");
  view->refresh();
  QThread::msleep(1300);
  require(countLines(helpers.filePath("ends")) > ended, "worker ended before signal delivery");
  QFile::remove(repo.workdir().filePath("final.txt"));
  writeFile(repo.workdir().filePath("boundary.txt"), "last\n");
  view->refresh();
  require(until([&] { return !finished.isEmpty() && !view->isLoading(); }), "boundary finishes");
  diff = view->commitList()->status();
  require(diff && diff.count() == 1 && diff.name(0) == "boundary.txt", "latest boundary status");
  fprintf(stderr, "BOUNDARY status_count=%d latest=%s scans=%d\n", finished.count(),
          qPrintable(diff.name(0)), countLines(helpers.filePath("starts")) - started);
  QFile::remove(helpers.filePath("delay"));
  // A process requested during two consecutive busy operations starts once,
  // after both operations have returned.
  const QString indexer = QCoreApplication::applicationDirPath() + "/gittyup-indexer";
  require(!QFile::exists(indexer), "probe indexer path is unused");
  writeFile(indexer, "#!/bin/sh\necho start >> \"$AUDIT_STATUS_DIR/indexer-starts\"\n");
  require(QFile::setPermissions(indexer, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner), "indexer chmod");
  repo.appConfig().setValue("index.enable", true);
  bool requestedDuringBusy = false;
  QTimer::singleShot(10, view, [&] {
    require(view->isBusy(), "index request runs during operation");
    view->startIndexing();
    view->startIndexing();
    view->startIndexing();
    requestedDuringBusy = true;
  });
  require(view->runBusy([] { QThread::msleep(200); }), "first busy operation");
  require(requestedDuringBusy && countLines(helpers.filePath("indexer-starts")) == 0,
          "indexer waits for first operation");
  require(view->runBusy([] { QThread::msleep(200); }), "second busy operation");
  require(countLines(helpers.filePath("indexer-starts")) == 0,
          "indexer waits for second operation");
  require(until([&] { return countLines(helpers.filePath("indexer-starts")) == 1; }),
          "indexer starts after busy operations");
  QTest::qWait(100);
  require(countLines(helpers.filePath("indexer-starts")) == 1, "indexer requests coalesced");
  fprintf(stderr, "INDEXER requests=3 busy_operations=2 starts_during_busy=0 starts_after=1\n");
  window.close();
  fprintf(stderr, "PASS pending status and deferred indexer scenarios\n");
}
