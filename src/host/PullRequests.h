//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef PULLREQUESTS_H
#define PULLREQUESTS_H

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QStringList>

class Account;
class QNetworkAccessManager;
class QNetworkReply;

struct PullRequest {
  int number = 0;
  QString title;
  QString body;
  QString author;
  // The branch with the changes, and the repository it's in when it's a
  // fork.
  QString head;
  QString headRepo;
  bool fork = false;
  // The branch that the changes are merged into.
  QString base;
  QString url;
  bool draft = false;
  QDateTime created;
  QDateTime updated;
  QStringList labels;
};

// The open pull requests of a repository on its hosting service, like the
// pull requests in GitKraken's left panel. GitHub, GitLab (merge requests)
// and Gitea are supported. Public repositories on github.com and gitlab.com
// don't need an account.
class PullRequests : public QObject {
  Q_OBJECT

public:
  enum Host { None, GitHub, GitLab, Gitea };

  PullRequests(QObject *parent = nullptr);

  // Find the service of the remote 'url', with 'account' for private
  // repositories and other servers. Returns false if it isn't supported.
  bool setRemote(const QString &url, Account *account = nullptr);

  Host host() const { return mHost; }
  QString hostName() const;
  // The owner and name of the repository, like "owner/repo".
  QString path() const { return mPath; }
  bool isSupported() const { return mHost != None; }

  // Request the open pull requests again.
  void refresh();

  bool isLoading() const { return !mReply.isNull(); }
  QString error() const { return mError; }
  const QList<PullRequest> &pullRequests() const { return mPullRequests; }
  // Use these pull requests instead, for tests.
  void setPullRequests(const QList<PullRequest> &pullRequests);

  static QList<PullRequest> parse(Host host, const QByteArray &json,
                                  QString *error = nullptr);
  // Split a remote URL into the name of its server and the path of the
  // repository, without ".git".
  static bool parseUrl(const QString &url, QString *server, QString *path);

signals:
  void changed();

private:
  QNetworkAccessManager *mMgr;
  QPointer<QNetworkReply> mReply;
  QPointer<Account> mAccount;

  Host mHost = None;
  QString mApi;
  QString mPath;
  QString mError;
  QList<PullRequest> mPullRequests;
};

#endif
