#include "git/Repository.h"
#include "git/Reference.h"
#include "git/Config.h"
#include "git/Signature.h"
#include "git/Rewrite.h"
#include "git2/commit.h"
#include "git2/repository.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>
#include <iostream>
struct Out {
  int code;
  QByteArray out, err;
};
static Out cli(const QString &p, const QStringList &args) {
  QProcess proc;
  proc.setWorkingDirectory(p);
  proc.start("git", args);
  if (!proc.waitForFinished())
    exit(2);
  return {proc.exitCode(), proc.readAllStandardOutput().trimmed(),
          proc.readAllStandardError().trimmed()};
}
static QByteArray run(const QString &p, const QStringList &args) {
  auto r = cli(p, args);
  if (r.code) {
    std::cerr << args.join(' ').toStdString() << ":" << r.err.constData()
              << '\n';
    exit(3);
  }
  return r.out;
}
static void put(const QString &p, const QByteArray &data,
                bool executable = false) {
  QFile f(p);
  if (!f.open(QFile::WriteOnly))
    exit(4);
  f.write(data);
  f.close();
  if (executable)
    QFile::setPermissions(p, QFile::ReadOwner | QFile::WriteOwner |
                                 QFile::ExeOwner);
}
static git::Repository init(const QString &p) {
  auto r = git::Repository::init(p);
  r.gitConfig().setValue("user.name", QString("Review"));
  r.gitConfig().setValue("user.email", QString("review@example.test"));
  r.gitConfig().setValue("commit.gpgsign", false);
  return r;
}
static git::Commit add(git::Repository &r, const QString &file,
                       const QByteArray &data) {
  const QString p = r.workdir().path();
  put(p + "/" + file, data);
  run(p, {"add", file});
  run(p, {"commit", "-qm", file});
  r.index().read();
  return r.head().target();
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  qputenv("GIT_CONFIG_GLOBAL", "/dev/null");
  qputenv("GIT_CONFIG_NOSYSTEM", "1");
  git::Repository::init();
  {
    QTemporaryDir tmp;
    auto repo = init(tmp.path());
    auto old = add(repo, "a", "base\n");
    auto newer = add(repo, "b", "newer\n");
    git_repository *raw = nullptr;
    git_repository_open(&raw, tmp.path().toUtf8());
    git_oid oldid, nativeId;
    git_oid_fromstr(&oldid, old.id().toString().toUtf8());
    git_commit *oldptr = nullptr;
    git_commit_lookup(&oldptr, raw, &oldid);
    int native = git_commit_amend(&nativeId, oldptr, "HEAD", nullptr, nullptr,
                                  nullptr, "native stale amend", nullptr);
    git_commit_free(oldptr);
    git_repository_free(raw);
    bool project = repo.amend(old, repo.defaultSignature(),
                              repo.defaultSignature(), "stale project amend");
    auto head = repo.head().target();
    std::cout << "STALE_AMEND native=" << native << " project=" << project
              << " old=" << old.shortId().toStdString()
              << " newer=" << newer.shortId().toStdString()
              << " head=" << head.shortId().toStdString()
              << " head_parent_count=" << head.parents().size()
              << " newer_in_history="
              << (cli(tmp.path(), {"merge-base", "--is-ancestor",
                                   newer.id().toString(), "HEAD"})
                      .code == 0)
              << '\n';
  }
  {
    QTemporaryDir tmp;
    auto repo = init(tmp.path());
    auto base = add(repo, "a", "base\n");
    run(tmp.path(), {"commit", "-q", "--allow-empty", "-m", "empty marker"});
    auto empty = repo.head().target();
    git::Rewrite rewrite(repo);
    auto result =
        rewrite.apply(base, {{empty, git::Rewrite::Reword, "renamed marker"}},
                      repo.defaultSignature());
    std::cout << "EMPTY_REWORD valid=" << result.isValid()
              << " result_is_base=" << (result.id() == base.id())
              << " result_message=" << result.summary().toStdString()
              << " error=" << rewrite.error().toStdString() << '\n';
    put(tmp.path() + "/.git/sequence-editor",
        "#!/bin/sh\nsed -i 's/^pick /reword /' \"$1\"\n", true);
    put(tmp.path() + "/.git/message-editor",
        "#!/bin/sh\nprintf '%s\\n' 'renamed marker' > \"$1\"\n", true);
    qputenv("GIT_SEQUENCE_EDITOR",
            (tmp.path() + "/.git/sequence-editor").toUtf8());
    qputenv("GIT_EDITOR", (tmp.path() + "/.git/message-editor").toUtf8());
    run(tmp.path(), {"rebase", "-i", base.id().toString()});
    std::cout << "EMPTY_REWORD_NATIVE count="
              << run(tmp.path(),
                     {"rev-list", "--count", base.id().toString() + "..HEAD"})
                     .constData()
              << " message="
              << run(tmp.path(), {"log", "-1", "--format=%s"}).constData()
              << '\n';
    qunsetenv("GIT_SEQUENCE_EDITOR");
    qunsetenv("GIT_EDITOR");
  }
  {
    QTemporaryDir tmp;
    auto repo = init(tmp.path());
    auto base = add(repo, "a", "base\n");
    QString p = tmp.path(), key = p + "/key";
    QProcess keygen;
    keygen.start("ssh-keygen", {"-q", "-t", "ed25519", "-N", "", "-f", key});
    if (!keygen.waitForFinished() || keygen.exitCode())
      exit(5);
    QByteArray concurrent =
        run(p, {"commit-tree", base.id().toString() + "^{tree}", "-p",
                base.id().toString(), "-m", "Concurrent update"});
    QByteArray wrapper = "#!/bin/sh\ngit -C '" + p.toUtf8() +
                         "' update-ref HEAD " + concurrent +
                         " || exit 10\nexec /usr/bin/ssh-keygen \"$@\"\n";
    put(p + "/sign-wrapper", wrapper, true);
    put(p + "/signed.txt", "signed\n");
    run(p, {"add", "signed.txt"});
    repo.index().read();
    repo.gitConfig().setValue("commit.gpgsign", true);
    repo.gitConfig().setValue("gpg.format", QString("ssh"));
    repo.gitConfig().setValue("user.signingkey", key);
    repo.gitConfig().setValue("gpg.ssh.program", p + "/sign-wrapper");
    auto signedCommit = repo.commit("Signed change");
    QFile pubkey(key + ".pub");
    if (!pubkey.open(QFile::ReadOnly))
      exit(6);
    put(p + "/allowed_signers", "review@example.test " + pubkey.readAll());
    auto verified =
        cli(p, {"-c", "gpg.ssh.program=/usr/bin/ssh-keygen", "-c",
                "gpg.ssh.allowedSignersFile=" + p + "/allowed_signers",
                "verify-commit", signedCommit.id().toString()});
    std::cout << "SIGNING_CONCURRENT valid=" << signedCommit.isValid()
              << " signature=" << signedCommit.signatureKind().toStdString()
              << " verified=" << (verified.code == 0) << " head_is_signed="
              << (repo.head().target().id() == signedCommit.id())
              << " concurrent_in_history="
              << (cli(p, {"merge-base", "--is-ancestor",
                          QString::fromUtf8(concurrent), "HEAD"})
                      .code == 0)
              << " signed_parent_is_base="
              << (signedCommit.parents().value(0).id() == base.id()) << '\n';
    run(p, {"update-ref", "HEAD", base.id().toString()});
    auto native = cli(p, {"commit", "-qm", "Native signed change"});
    std::cout << "SIGNING_CONCURRENT_NATIVE exit=" << native.code
              << " head_is_concurrent="
              << (run(p, {"rev-parse", "HEAD"}) == concurrent)
              << " error=" << native.err.constData() << '\n';
  }
  git::Repository::shutdown();
}
