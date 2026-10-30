//
//          Copyright (c) 2026, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"

#include "git/Commit.h"
#include "git/Config.h"
#include "git/Reference.h"
#include "git/Repository.h"
#include "git2/filter.h"
#include "git2/repository.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

namespace {

// Registers a throwaway global filter driver, scoped to a fake $HOME so
// the developer's real ~/.gitconfig is never touched. git::Filter::init()
// reads global filters once, during Application's constructor, before any
// QTest slot runs - so $HOME must be redirected during static
// initialization, which is why this lives in a namespace-scope static.
struct FakeHome {
  QTemporaryDir dir;
  QString argsOutPath;

  FakeHome() {
    argsOutPath = dir.filePath("filter_args.txt");

    QFile gitconfig(dir.filePath(".gitconfig"));
    bool opened = gitconfig.open(QIODevice::WriteOnly | QIODevice::Truncate |
                                 QIODevice::Text);
    Q_ASSERT(opened);
    Q_UNUSED(opened);

    QTextStream out(&gitconfig);
    // "clean" is unused but must be non-empty for the driver to register.
    out << "[filter \"gittyupsecuritytest\"]\n";
    out << "    clean = cat\n";
    out << "    smudge = printf 'ARG=[%s]' %f > \"" << argsOutPath << "\"\n";
    // Filters whose smudge fails.
    out << "[filter \"gittyupoptional\"]\n";
    out << "    clean = cat\n";
    out << "    smudge = false\n";
    out << "[filter \"gittyuprequired\"]\n";
    out << "    clean = cat\n";
    out << "    smudge = false\n";
    out << "    required = true\n";
    // A filter that repositories override.
    out << "[filter \"gittyupscope\"]\n";
    out << "    clean = sed s/global/token/\n";
    out << "    smudge = sed s/token/global/\n";
    gitconfig.close();

    qputenv("HOME", dir.path().toLocal8Bit());
    qputenv("USERPROFILE", dir.path().toLocal8Bit());
  }
};

FakeHome &fakeHome() {
  static FakeHome home;
  return home;
}

// Force construction (and therefore the $HOME override) before main().
FakeHome &gFakeHomeInit = fakeHome();

// Filter 'input' like libgit2 does when it checks out the file at 'path',
// or cleans it to compare it with the index. It's 1 without a filter.
int filter(git_repository *repo, const char *path, const QByteArray &input,
           QByteArray &result, git_filter_mode_t mode = GIT_FILTER_SMUDGE) {
  git_filter_list *filters = nullptr;
  if (git_filter_list_load(&filters, repo, nullptr, path, mode,
                           GIT_FILTER_DEFAULT) ||
      !filters)
    return 1;

  git_buf buf = GIT_BUF_INIT;
  int error = git_filter_list_apply_to_buffer(&buf, filters, input.constData(),
                                              input.size());
  result = QByteArray(buf.ptr, buf.size);
  git_buf_dispose(&buf);
  git_filter_list_free(filters);
  return error;
}

// The repository at 'path' with attributes for the test filters.
git_repository *open(const QString &path) {
  QFile attributes(path + "/.gitattributes");
  if (!attributes.open(QFile::WriteOnly))
    return nullptr;
  attributes.write("optional.txt filter=gittyupoptional\n"
                   "required.txt filter=gittyuprequired\n"
                   "scope.txt filter=gittyupscope\n");
  attributes.close();

  git_repository *repo = nullptr;
  git_repository_open(&repo, path.toUtf8());
  return repo;
}

} // namespace

class TestFilter : public QObject {
  Q_OBJECT

private slots:
  void rejectsShellInjectionInFilename();
  void failingFilters();
  void repositoryOverride();
};

// Regression test: a filename embedding a single quote, combined with a
// .gitattributes rule assigning it to any globally-registered filter (e.g.
// Git LFS), could break out of the quoted %f argument and run arbitrary
// shell commands during smudge.
void TestFilter::rejectsShellInjectionInFilename() {
  FakeHome &home = fakeHome();
  QFile::remove(home.argsOutPath);

  // FilterCommandInjection.zip's single commit contains one file, matching
  // this name, whose .gitattributes assigns it to the "gittyupsecuritytest"
  // filter registered above. If %f isn't escaped correctly, the embedded
  // "'" ends the quoted shell argument early and the ';'s that follow start
  // new, attacker-controlled statements.
  QString maliciousName = "evil';>injected_marker;echo'safe";

  QString path = Test::extractRepository("FilterCommandInjection.zip");
  QVERIFY(!path.isEmpty());
  git::Repository repo = git::Repository::open(path);
  QVERIFY(repo.isValid());
  QDir workdir = repo.workdir();

  // The injected ">injected_marker" redirection, if it runs, creates this
  // file in the checkout's working directory.
  QString markerPath = workdir.filePath("injected_marker");
  QFile::remove(markerPath);

  // Delete the working copy so checkout has to rewrite it from the repo,
  // which is what actually triggers the smudge filter.
  QVERIFY(QFile::remove(workdir.filePath(maliciousName)));

  // libgit2's own checkout implementation invokes the registered smudge
  // filter driver as it writes each file to the working directory.
  git::Reference head = repo.head();
  QVERIFY(head.isValid());
  QVERIFY(
      repo.checkout(head.target(), nullptr, QStringList(), GIT_CHECKOUT_FORCE));

  QVERIFY2(!QFile::exists(markerPath),
           "filter command injection via crafted filename was not blocked");

  // Confirms the filter actually ran (not just failed to start).
  QFile args(home.argsOutPath);
  QVERIFY2(args.exists(), "smudge filter did not run");
  QVERIFY(args.open(QIODevice::ReadOnly | QIODevice::Text));
  QString content = QString::fromUtf8(args.readAll());
  args.close();

  QCOMPARE(content, QString("ARG=[%1]").arg(maliciousName));
}

// When an optional filter fails, the content passes through it unchanged,
// like git does. A required one fails.
void TestFilter::failingFilters() {
  Test::ScratchRepository repo;
  git_repository *raw = open(repo->workdir().path());
  QVERIFY(raw);

  QByteArray result;
  QCOMPARE(filter(raw, "optional.txt", "content\n", result), 0);
  QCOMPARE(result, QByteArray("content\n"));
  QVERIFY(filter(raw, "required.txt", "content\n", result) < 0);
  git_repository_free(raw);
}

// The configuration of a repository overrides the global one of a filter,
// like git does, and only for that repository.
void TestFilter::repositoryOverride() {
  Test::ScratchRepository local;
  local->gitConfig().setValue("filter.gittyupscope.clean",
                              QString("sed s/local/token/"));
  local->gitConfig().setValue("filter.gittyupscope.smudge",
                              QString("sed s/token/local/"));
  local->gitConfig().setValue("filter.gittyupoptional.required", true);
  Test::ScratchRepository global;
  git_repository *localRepo = open(local->workdir().path());
  git_repository *globalRepo = open(global->workdir().path());
  QVERIFY(localRepo && globalRepo);

  QByteArray result;
  QCOMPARE(filter(localRepo, "scope.txt", "token\n", result), 0);
  QCOMPARE(result, QByteArray("local\n"));
  QCOMPARE(filter(globalRepo, "scope.txt", "token\n", result), 0);
  QCOMPARE(result, QByteArray("global\n"));

  // Files that the override checked out don't look changed.
  QCOMPARE(filter(localRepo, "scope.txt", "local\n", result, GIT_FILTER_CLEAN),
           0);
  QCOMPARE(result, QByteArray("token\n"));
  QCOMPARE(
      filter(globalRepo, "scope.txt", "global\n", result, GIT_FILTER_CLEAN), 0);
  QCOMPARE(result, QByteArray("token\n"));

  // The optional filter is required in one of them.
  QVERIFY(filter(localRepo, "optional.txt", "content\n", result) < 0);
  QCOMPARE(filter(globalRepo, "optional.txt", "content\n", result), 0);
  QCOMPARE(result, QByteArray("content\n"));

  git_repository_free(localRepo);
  git_repository_free(globalRepo);
}

TEST_MAIN(TestFilter)

#include "Filter.moc"
