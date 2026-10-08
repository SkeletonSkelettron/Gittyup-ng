//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "dialogs/InputDialog.h"
#include "git/Remote.h"
#include "git2/credential.h"
#include "git2/errors.h"
#include "git2/sys/errors.h"
#include "git2/sys/credential.h"
#include "log/LogEntry.h"
#include "ui/RemoteCallbacks.h"
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtConcurrent>

using namespace QTest;

namespace {

const QString kUrl = "ssh://git@example.com/repo.git";

// Records what it's asked for, and cancels passwords.
class Callbacks : public git::Remote::Callbacks {
public:
  Callbacks(const QString &config = QString())
      : git::Remote::Callbacks(kUrl), mConfig(config) {}

  QString configFilePath() const override { return mConfig; }

  bool passphrase(const QString &url, const QString &keyFile,
                  QString &passphrase) override {
    mPassphrases.append(keyFile);
    passphrase = "secret";
    return true;
  }

  bool credentials(const QString &url, QString &username,
                   QString &password) override {
    mPasswords.append(username);
    return false;
  }

  QString mConfig;
  QStringList mPassphrases;
  QStringList mPasswords;
};

// The private key and passphrase of the credential that the callbacks
// return for 'types', or the error.
struct Request {
  int result = 0;
  QString key;
  QString passphrase;
  QString error;
};

Request request(git::Remote::Callbacks *callbacks, unsigned int types) {
  Request request;
  git_error_clear();
  git_credential *cred = nullptr;
  request.result = git::Remote::Callbacks::credentials(&cred, kUrl.toUtf8(),
                                                       "git", types, callbacks);
  if (const git_error *error = git_error_last())
    request.error = error->message;

  if (cred && cred->credtype == GIT_CREDENTIAL_SSH_KEY) {
    git_credential_ssh_key *key =
        reinterpret_cast<git_credential_ssh_key *>(cred);
    request.key = QString::fromLocal8Bit(key->privatekey);
    if (key->passphrase)
      request.passphrase = key->passphrase;
  }

  if (cred)
    git_credential_free(cred);

  return request;
}

} // namespace

// Offers the SSH keys one after another like OpenSSH, and asks for
// passwords only when they aren't accepted.
class TestSshCredentials : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void keys();
  void exhausted();
  void password();
  void noKeys();
  void dialogs();
  void automatic();
  void cleanupTestCase();

private:
  void setHome(const QString &path);
  bool key(const QString &name, const QString &type,
           const QString &passphrase = QString());
  QString dialogTitle(const std::function<bool()> &ask, QString *text);

  QTemporaryDir mHome;
  QTemporaryDir mEmptyHome;
  QByteArray mOldHome;
  QByteArray mOldProfile;
  QString mConfig;
};

void TestSshCredentials::initTestCase() {
  if (QStandardPaths::findExecutable("ssh-keygen").isEmpty())
    QSKIP("ssh-keygen isn't installed");

  mOldHome = qgetenv("HOME");
  mOldProfile = qgetenv("USERPROFILE");
  setHome(mHome.path());
  QVERIFY(QDir(mHome.path()).mkpath(".ssh"));
  QVERIFY(QDir(mEmptyHome.path()).mkpath(".ssh"));

  QVERIFY(key("work_key", "ed25519"));
  QVERIFY(key("id_ed25519", "ed25519"));
  QVERIFY(key("id_ecdsa", "ecdsa", "secret"));
  QVERIFY(key("id_rsa", "rsa"));

  // The configured key of the host comes first. Missing files are skipped.
  mConfig = mHome.filePath(".ssh/config");
  QFile config(mConfig);
  QVERIFY(config.open(QFile::WriteOnly));
  config.write("Host example.com\n"
               "  IdentityFile ~/.ssh/work_key\n"
               "Host other.com\n"
               "  IdentityFile ~/.ssh/other_key\n"
               "Host *\n"
               "  IdentityFile ~/.ssh/missing_key\n");
}

void TestSshCredentials::keys() {
  Callbacks callbacks(mConfig);
  QDir ssh(mHome.filePath(".ssh"));

  Request first = request(&callbacks, GIT_CREDENTIAL_SSH_KEY);
  QCOMPARE(first.result, 0);
  QCOMPARE(first.key, ssh.filePath("work_key"));
  QVERIFY(first.passphrase.isEmpty());

  Request second = request(&callbacks, GIT_CREDENTIAL_SSH_KEY);
  QCOMPARE(second.key, ssh.filePath("id_ed25519"));

  // Only the encrypted key asks for its passphrase.
  QVERIFY(callbacks.mPassphrases.isEmpty());
  Request third = request(&callbacks, GIT_CREDENTIAL_SSH_KEY);
  QCOMPARE(third.key, ssh.filePath("id_ecdsa"));
  QCOMPARE(third.passphrase, QString("secret"));
  QCOMPARE(callbacks.mPassphrases, QStringList{ssh.filePath("id_ecdsa")});

  Request fourth = request(&callbacks, GIT_CREDENTIAL_SSH_KEY);
  QCOMPARE(fourth.key, ssh.filePath("id_rsa"));
  QVERIFY(callbacks.mPasswords.isEmpty());
}

void TestSshCredentials::exhausted() {
  Callbacks callbacks(mConfig);
  for (int i = 0; i < 4; ++i)
    QVERIFY(!request(&callbacks, GIT_CREDENTIAL_SSH_KEY).key.isEmpty());

  // Then the error says which keys weren't accepted.
  Request last = request(&callbacks, GIT_CREDENTIAL_SSH_KEY);
  QVERIFY(last.result < 0);
  QVERIFY(last.key.isEmpty());
  QVERIFY2(last.error.contains("didn't accept"), qPrintable(last.error));
  QVERIFY(last.error.contains("work_key"));
  QVERIFY(last.error.contains("id_rsa"));
  QVERIFY(callbacks.mPasswords.isEmpty());
}

void TestSshCredentials::password() {
  // The server also allows passwords.
  unsigned int types =
      GIT_CREDENTIAL_SSH_KEY | GIT_CREDENTIAL_USERPASS_PLAINTEXT;
  Callbacks callbacks(mConfig);
  for (int i = 0; i < 4; ++i) {
    QVERIFY(!request(&callbacks, types).key.isEmpty());
    QVERIFY(callbacks.mPasswords.isEmpty());
  }

  // The password is asked for after the keys. Canceling it fails with the
  // keys that weren't accepted.
  Request last = request(&callbacks, types);
  QCOMPARE(callbacks.mPasswords, QStringList{"git"});
  QVERIFY(last.result < 0);
  QVERIFY2(last.error.contains("didn't accept"), qPrintable(last.error));
}

void TestSshCredentials::noKeys() {
  setHome(mEmptyHome.path());
  Callbacks callbacks;
  Request result = request(&callbacks, GIT_CREDENTIAL_SSH_KEY);
  setHome(mHome.path());

  QVERIFY(result.result < 0);
  QVERIFY2(result.error.contains("no SSH key was found"),
           qPrintable(result.error));
}

void TestSshCredentials::dialogs() {
  LogEntry log;
  RemoteCallbacks callbacks(RemoteCallbacks::Receive, &log, kUrl);

  // A password isn't called a passphrase.
  QString text;
  QString title = dialogTitle(
      [&callbacks] {
        QString username = "git";
        QString password;
        return callbacks.credentials(kUrl, username, password);
      },
      &text);
  QCOMPARE(title, QString("SSH Password"));
  QVERIFY2(text.contains("didn't accept your SSH keys"), qPrintable(text));

  // The passphrase dialog names the key.
  QString key = QDir(mHome.filePath(".ssh")).filePath("id_ecdsa");
  title = dialogTitle(
      [&callbacks, key] {
        QString passphrase;
        return callbacks.passphrase(kUrl, key, passphrase);
      },
      &text);
  QCOMPARE(title, QString("SSH Passphrase"));
  QVERIFY2(text.contains(QDir::toNativeSeparators(key)), qPrintable(text));
}

void TestSshCredentials::automatic() {
  LogEntry log;
  RemoteCallbacks callbacks(RemoteCallbacks::Receive, &log, kUrl);
  callbacks.setInteractive(false);

  // Callbacks of automatic fetches fail without a dialog.
  QString text;
  QString title = dialogTitle(
      [&callbacks] {
        QString username = "git";
        QString password;
        return callbacks.credentials(kUrl, username, password);
      },
      &text);
  QVERIFY2(title.isEmpty(), qPrintable(title));

  QString key = QDir(mHome.filePath(".ssh")).filePath("id_ecdsa");
  title = dialogTitle(
      [&callbacks, key] {
        QString passphrase;
        return callbacks.passphrase(kUrl, key, passphrase);
      },
      &text);
  QVERIFY2(title.isEmpty(), qPrintable(title));
}

void TestSshCredentials::cleanupTestCase() {
  qputenv("HOME", mOldHome);
  qputenv("USERPROFILE", mOldProfile);
}

void TestSshCredentials::setHome(const QString &path) {
  // QDir::homePath() reads HOME, or USERPROFILE on Windows.
  qputenv("HOME", path.toUtf8());
  qputenv("USERPROFILE", QDir::toNativeSeparators(path).toUtf8());
}

bool TestSshCredentials::key(const QString &name, const QString &type,
                             const QString &passphrase) {
  QProcess process;
  process.start(QStandardPaths::findExecutable("ssh-keygen"),
                {"-q", "-t", type, "-N", passphrase, "-C", "test", "-f",
                 mHome.filePath(".ssh/" + name)});
  return process.waitForFinished() && process.exitCode() == 0;
}

// Asks on another thread, like fetches do, and returns the title and text
// of the dialog that's shown, after canceling it.
QString TestSshCredentials::dialogTitle(const std::function<bool()> &ask,
                                        QString *text) {
  QString title;
  text->clear();

  // The timer also fires in the event loop of the dialog.
  QTimer timer;
  timer.setInterval(20);
  connect(&timer, &QTimer::timeout, [&title, text] {
    for (QWidget *widget : QApplication::topLevelWidgets()) {
      InputDialog *dialog = qobject_cast<InputDialog *>(widget);
      if (dialog && dialog->isVisible()) {
        title = dialog->title();
        *text = dialog->text();
        dialog->reject();
      }
    }
  });
  timer.start();

  QFuture<bool> future = QtConcurrent::run(ask);
  while (!future.isFinished())
    qWait(20);

  // Nothing was entered.
  return future.result() ? QString("accepted") : title;
}

TEST_MAIN(TestSshCredentials)

#include "ssh_credentials.moc"
