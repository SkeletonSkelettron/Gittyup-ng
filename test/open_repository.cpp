//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "git2/errors.h"
#include "git2/repository.h"

using namespace Test;

// Makes libgit2 see the owner of every path as another user (internal).
extern "C" void git_fs_path__set_owner(int owner);

namespace {

const int kOwnerNone = 0;
const int kOwnerOther = 1 << 4;

} // namespace

// Opens repositories that libgit2 thinks belong to another user when git
// itself opens them, like folders of the Administrators on Windows.
class TestOpenRepository : public QObject {
  Q_OBJECT

private slots:
  void ownedByOther();
  void gitRefuses();
  void cleanup();

private:
  ScratchRepository mRepo;
};

void TestOpenRepository::ownedByOther() {
  QString path = mRepo->workdir().path();
  QVERIFY(git::Repository::open(path).isValid());

  // libgit2 alone refuses it, but git opens it, since it's ours.
  git_fs_path__set_owner(kOwnerOther);
  git_repository *repo = nullptr;
  QCOMPARE(git_repository_open(&repo, path.toUtf8()), GIT_EOWNER);
  QVERIFY(git::Repository::open(path).isValid());
  QVERIFY(git::Repository::open(path, true).isValid());
}

void TestOpenRepository::gitRefuses() {
#ifdef Q_OS_WIN
  QSKIP("git is found outside of the path on Windows");
#endif

  // Without git, libgit2 decides, and says why.
  QByteArray path = qgetenv("PATH");
  qputenv("PATH", "");
  git_fs_path__set_owner(kOwnerOther);
  git::Repository repo = git::Repository::open(mRepo->workdir().path());
  QString error = git::Repository::lastError();
  qputenv("PATH", path);

  QVERIFY(!repo.isValid());
  QVERIFY2(error.contains("not owned by current user"), qPrintable(error));
}

void TestOpenRepository::cleanup() { git_fs_path__set_owner(kOwnerNone); }

TEST_MAIN(TestOpenRepository)

#include "open_repository.moc"
