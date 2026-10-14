//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "git/Remote.h"
#include <QFile>
#include <QProcess>
#include <QSignalSpy>

using namespace Test;
using namespace QTest;

// Adds, changes, renames and deletes remotes, with the remote-tracking
// branches and the upstreams that go with them.
class TestRemotes : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void addRemote();
  void addInvalidRemote();
  void setUrl();
  void setName();
  void setNameTaken();
  void deleteRemote();
  void deleteMissingRemote();

private:
  void commit(git::Repository repo);
  QString git(const QStringList &args);
  git::Branch head() { return mRepo->head(); }

  ScratchRepository mOrigin;
  ScratchRepository mRepo;
  QString mMain;
};

void TestRemotes::initTestCase() {
  // The signals name the type of their argument without its namespace.
  qRegisterMetaType<git::Remote>("Remote");

  commit(mOrigin);
  commit(mRepo);
  mMain = mOrigin->head().name();
}

void TestRemotes::addRemote() {
  QSignalSpy added(mRepo->notifier(), &git::RepositoryNotifier::remoteAdded);

  git::Remote remote = mRepo->addRemote("origin", mOrigin->workdir().path());
  QVERIFY(remote.isValid());
  QCOMPARE(remote.name(), QString("origin"));
  QCOMPARE(mRepo->lookupRemote("origin").url(), mOrigin->workdir().path());
  QCOMPARE(added.count(), 1);

  // A remote-tracking branch, and an upstream that tracks it.
  git({"fetch", "-q", "origin"});
  git::Branch tracking =
      mRepo->lookupBranch("origin/" + mMain, GIT_BRANCH_REMOTE);
  QVERIFY(tracking.isValid());
  head().setUpstream(tracking);
  QCOMPARE(head().upstream().name(), "origin/" + mMain);
}

// Names that aren't valid or are taken don't add remotes.
void TestRemotes::addInvalidRemote() {
  QString url = "https://example.com/repo.git";
  QVERIFY(!mRepo->addRemote("bad name", url).isValid());
  QVERIFY(!mRepo->addRemote("origin", url).isValid());

  QCOMPARE(mRepo->remotes().size(), 1);
  QCOMPARE(mRepo->lookupRemote("origin").url(), mOrigin->workdir().path());
}

void TestRemotes::setUrl() {
  QString url = "https://example.com/repo.git";
  git::Remote remote = mRepo->lookupRemote("origin");
  remote.setUrl(url);
  QCOMPARE(mRepo->lookupRemote("origin").url(), url);
}

// Renaming moves the remote-tracking branches and the upstreams.
void TestRemotes::setName() {
  git::Remote remote = mRepo->lookupRemote("origin");
  remote.setName("upstream");

  QVERIFY(!mRepo->lookupRemote("origin").isValid());
  QVERIFY(mRepo->lookupRemote("upstream").isValid());
  QVERIFY(!mRepo->lookupBranch("origin/" + mMain, GIT_BRANCH_REMOTE).isValid());
  QVERIFY(
      mRepo->lookupBranch("upstream/" + mMain, GIT_BRANCH_REMOTE).isValid());
  QCOMPARE(head().upstream().name(), "upstream/" + mMain);
}

// A name that is taken leaves both remotes as they were.
void TestRemotes::setNameTaken() {
  QString url = "https://example.com/other.git";
  QVERIFY(mRepo->addRemote("other", url).isValid());

  QSignalSpy failed(mRepo->notifier(),
                    &git::RepositoryNotifier::gitCommandFailed);
  git::Remote remote = mRepo->lookupRemote("upstream");
  remote.setName("other");

  QVERIFY(remote.isValid());
  QCOMPARE(failed.count(), 1);
  QVERIFY(mRepo->lookupRemote("upstream").isValid());
  QCOMPARE(mRepo->lookupRemote("other").url(), url);
}

// Deleting removes the remote-tracking branches and the upstreams.
void TestRemotes::deleteRemote() {
  QSignalSpy removed(mRepo->notifier(),
                     &git::RepositoryNotifier::remoteRemoved);
  mRepo->deleteRemote("upstream");

  QVERIFY(!mRepo->lookupRemote("upstream").isValid());
  QVERIFY(
      !mRepo->lookupBranch("upstream/" + mMain, GIT_BRANCH_REMOTE).isValid());
  QVERIFY(!head().upstream().isValid());
  QString key = QString("branch.%1.remote").arg(head().name());
  QVERIFY(git({"config", "--get", key}).isEmpty());
  QCOMPARE(removed.count(), 1);
  QCOMPARE(removed.first().first().toString(), QString("upstream"));
}

void TestRemotes::deleteMissingRemote() {
  QSignalSpy removed(mRepo->notifier(),
                     &git::RepositoryNotifier::remoteRemoved);
  mRepo->deleteRemote("missing");

  QCOMPARE(mRepo->remotes().size(), 1);
  QCOMPARE(removed.count(), 0);
}

void TestRemotes::commit(git::Repository repo) {
  QFile file(repo.workdir().filePath("file.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("content\n");
  file.close();

  repo.index().setStaged({"file.txt"}, true);
  QVERIFY(repo.commit("initial").isValid());
}

// Run git in the repository, for what Gittyup doesn't do itself.
QString TestRemotes::git(const QStringList &args) {
  QProcess process;
  process.setWorkingDirectory(mRepo->workdir().path());
  process.start("git", args);
  process.waitForFinished();
  return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

TEST_MAIN(TestRemotes)

#include "remotes.moc"
