//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "git/AnnotatedCommit.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Index.h"
#include "git/Reference.h"
#include "git/Rewrite.h"
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

using namespace Test;

// Signs commits with SSH keys like 'git commit -S'.
class TestSigning : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void unsigned_();
  void commit();
  void amend();
  void rewrite();
  void missingKey();
  void gpg();
  void rebase();

private:
  git::Commit commit(const QString &name);

  ScratchRepository mRepo;
  QTemporaryDir mKeys;
  QString mKey;
};

void TestSigning::initTestCase() {
  QString keygen = QStandardPaths::findExecutable("ssh-keygen");
  if (keygen.isEmpty())
    QSKIP("ssh-keygen isn't installed");

  // A key without a passphrase.
  mKey = mKeys.filePath("id_ed25519");
  QProcess process;
  process.start(keygen, {"-q", "-t", "ed25519", "-N", "", "-C", "test", "-f",
                         mKey});
  QVERIFY(process.waitForFinished());
  QCOMPARE(process.exitCode(), 0);

  commit("file.txt");
}

void TestSigning::unsigned_() {
  QCOMPARE(mRepo->head().target().signatureKind(), QString());
}

void TestSigning::commit() {
  git::Config config = mRepo->gitConfig();
  config.setValue("commit.gpgsign", true);
  config.setValue("gpg.format", QString("ssh"));
  config.setValue("user.signingkey", mKey + ".pub");

  git::Commit commit = this->commit("a.txt");
  QVERIFY2(commit.isValid(), qPrintable(git::Repository::lastError()));
  QCOMPARE(commit.signatureKind(), QString("SSH"));
  QCOMPARE(mRepo->head().target().id(), commit.id());
  QCOMPARE(commit.summary(), QString("Add a.txt"));

  // Git accepts the signature.
  QFile key(mKey + ".pub");
  QVERIFY(key.open(QFile::ReadOnly));
  QFile signers(mKeys.filePath("allowed_signers"));
  QVERIFY(signers.open(QFile::WriteOnly));
  signers.write(commit.committer().email().toUtf8() + " " + key.readAll());
  signers.close();

  QProcess git;
  git.setWorkingDirectory(mRepo->workdir().path());
  git.start(GIT_EXECUTABLE, {"-c", "gpg.ssh.allowedSignersFile=" +
                                       signers.fileName(),
                             "verify-commit", commit.id().toString()});
  QVERIFY(git.waitForFinished());
  QVERIFY2(git.exitCode() == 0, git.readAllStandardError().constData());
}

void TestSigning::amend() {
  git::Commit head = mRepo->head().target();
  git::Signature signature = mRepo->defaultSignature();
  QVERIFY(mRepo->amend(head, signature, signature, "Amended\n"));

  git::Commit amended = mRepo->head().target();
  QVERIFY(amended.id() != head.id());
  QCOMPARE(amended.summary(), QString("Amended"));
  QCOMPARE(amended.signatureKind(), QString("SSH"));
  QCOMPARE(amended.parents().first().id(), head.parents().first().id());
}

void TestSigning::rewrite() {
  git::Commit head = mRepo->head().target();
  git::Commit base = head.parents().first();

  git::Repository repo = mRepo;
  git::Rewrite rewrite(repo);
  QList<git::Rewrite::Step> steps = {
      {head, git::Rewrite::Reword, "Reworded\n"}};
  git::Commit result =
      rewrite.apply(base, steps, mRepo->defaultSignature());
  QVERIFY2(result.isValid(), qPrintable(rewrite.error()));
  QCOMPARE(result.summary(), QString("Reworded"));
  QCOMPARE(result.signatureKind(), QString("SSH"));
}

void TestSigning::missingKey() {
  git::Commit head = mRepo->head().target();
  mRepo->gitConfig().setValue("user.signingkey",
                              mKeys.filePath("missing.pub"));

  // The commit fails and says why.
  QVERIFY(!commit("b.txt").isValid());
  QVERIFY(git::Repository::lastError().contains("ssh-keygen"));
  QCOMPARE(mRepo->head().target().id(), head.id());

  // Without signing it works again.
  mRepo->gitConfig().setValue("commit.gpgsign", false);
  git::Commit plain = mRepo->commit("Add b.txt");
  QVERIFY(plain.isValid());
  QCOMPARE(plain.signatureKind(), QString());
}

void TestSigning::gpg() {
  QString gpg = QStandardPaths::findExecutable("gpg");
  if (gpg.isEmpty())
    QSKIP("gpg isn't installed");

  // A key in its own home without a passphrase.
  QTemporaryDir home;
  qputenv("GNUPGHOME", home.path().toUtf8());
  QProcess process;
  process.start(gpg, {"--batch", "--pinentry-mode", "loopback", "--passphrase",
                      "", "--quick-gen-key", "Test <signing@example.com>",
                      "ed25519", "sign", "never"});
  QVERIFY(process.waitForFinished(60000));
  QCOMPARE(process.exitCode(), 0);

  git::Config config = mRepo->gitConfig();
  config.setValue("commit.gpgsign", true);
  config.remove("gpg.format");
  config.setValue("user.signingkey", QString("signing@example.com"));

  git::Commit commit = this->commit("c.txt");
  QVERIFY2(commit.isValid(), qPrintable(git::Repository::lastError()));
  QCOMPARE(commit.signatureKind(), QString("GPG"));

  QProcess git;
  git.setWorkingDirectory(mRepo->workdir().path());
  git.start(GIT_EXECUTABLE, {"verify-commit", commit.id().toString()});
  QVERIFY(git.waitForFinished());
  QVERIFY2(git.exitCode() == 0, git.readAllStandardError().constData());

  QProcess::execute("gpgconf", {"--kill", "gpg-agent"});
  qunsetenv("GNUPGHOME");
}

void TestSigning::rebase() {
  git::Config config = mRepo->gitConfig();
  config.setValue("commit.gpgsign", true);
  config.setValue("gpg.format", QString("ssh"));
  config.setValue("user.signingkey", mKey + ".pub");

  // A branch from the parent of HEAD with a commit.
  git::Reference main = mRepo->head();
  git::Commit tip = main.target();
  git::Commit parent = tip.parents().first();
  git::Branch topic = mRepo->createBranch("topic", parent);
  QVERIFY(mRepo->checkout(parent, nullptr, {}, GIT_CHECKOUT_FORCE));
  QVERIFY(mRepo->setHead(topic));
  QVERIFY(commit("e.txt").isValid());

  // Rebasing it onto HEAD signs the new commit.
  mRepo->rebase(main.annotatedCommit(), QString(), QString());
  git::Commit rebased = mRepo->head().target();
  QCOMPARE(mRepo->head().name(), QString("topic"));
  QCOMPARE(rebased.summary(), QString("Add e.txt"));
  QCOMPARE(rebased.parents().first().id(), tip.id());
  QCOMPARE(rebased.signatureKind(), QString("SSH"));
}

git::Commit TestSigning::commit(const QString &name) {
  QFile file(mRepo->workdir().filePath(name));
  if (!file.open(QFile::WriteOnly))
    return git::Commit();
  file.write(name.toUtf8() + "\n");
  file.close();

  mRepo->index().setStaged({name}, true);
  return mRepo->commit(QString("Add %1").arg(name));
}

TEST_MAIN(TestSigning)

#include "signing.moc"
