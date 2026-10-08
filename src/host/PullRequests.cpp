//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "PullRequests.h"
#include "Account.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

namespace {

// At most this many pull requests are listed.
const int kPerPage = 50;

QDateTime date(const QJsonValue &value) {
  return QDateTime::fromString(value.toString(), Qt::ISODate);
}

} // namespace

PullRequests::PullRequests(QObject *parent)
    : QObject(parent), mMgr(new QNetworkAccessManager(this)) {}

bool PullRequests::setRemote(const QString &url, Account *account) {
  if (mReply)
    mReply->abort();

  mHost = None;
  mApi.clear();
  mPath.clear();
  mError.clear();
  mPullRequests.clear();
  mAccount = nullptr;

  QString server;
  QString path;
  if (!parseUrl(url, &server, &path)) {
    emit changed();
    return false;
  }

  if (account) {
    switch (account->kind()) {
      case Account::GitHub:
        mHost = GitHub;
        mApi = account->hasCustomUrl() ? account->url() + "/api/v3"
                                       : QString("https://api.github.com");
        break;
      case Account::GitLab:
        mHost = GitLab;
        mApi = account->url();
        break;
      case Account::Gitea:
        mHost = Gitea;
        mApi = account->url() + "/api/v1";
        break;
      default:
        break;
    }

    if (mHost != None)
      mAccount = account;
  }

  // Public repositories need no account.
  if (mHost == None) {
    if (server == "github.com") {
      mHost = GitHub;
      mApi = "https://api.github.com";
    } else if (server == "gitlab.com") {
      mHost = GitLab;
      mApi = "https://gitlab.com/api/v4";
    }
  }

  if (mHost != None)
    mPath = path;

  emit changed();
  return isSupported();
}

QString PullRequests::hostName() const {
  switch (mHost) {
    case GitHub:
      return QStringLiteral("GitHub");
    case GitLab:
      return QStringLiteral("GitLab");
    case Gitea:
      return QStringLiteral("Gitea");
    case None:
      break;
  }

  return QString();
}

void PullRequests::refresh() {
  if (!isSupported())
    return;

  if (mReply)
    mReply->abort();

  QString url;
  if (mHost == GitLab) {
    QString project = QUrl::toPercentEncoding(mPath);
    url = QString("%1/projects/%2/merge_requests?state=opened&per_page=%3")
              .arg(mApi, project)
              .arg(kPerPage);
  } else {
    url = QString("%1/repos/%2/pulls?state=open&per_page=%3")
              .arg(mApi, mPath)
              .arg(kPerPage);
  }

  QNetworkRequest request(QUrl(url, QUrl::StrictMode));
  request.setRawHeader("Accept", mHost == GitHub
                                     ? "application/vnd.github+json"
                                     : "application/json");
  QString agent = QString("%1/%2").arg(QCoreApplication::applicationName(),
                                       QCoreApplication::applicationVersion());
  request.setHeader(QNetworkRequest::UserAgentHeader, agent);

  if (mAccount) {
    QString password = mAccount->password();
    if (!password.isEmpty()) {
      if (mHost == GitLab) {
        request.setRawHeader("PRIVATE-TOKEN", password.toUtf8());
      } else {
        QString cred = QString("%1:%2").arg(mAccount->username(), password);
        request.setRawHeader("Authorization",
                             "Basic " + cred.toUtf8().toBase64());
      }
    }
  }

  mError.clear();
  QNetworkReply *reply = mMgr->get(request);
  mReply = reply;
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    if (reply != mReply)
      return;

    mReply = nullptr;
    if (reply->error() == QNetworkReply::OperationCanceledError)
      return;

    QByteArray data = reply->readAll();
    QString error;
    QList<PullRequest> list = parse(mHost, data, &error);
    if (!error.isEmpty() || reply->error() != QNetworkReply::NoError) {
      mError = !error.isEmpty() ? error : reply->errorString();
    } else {
      mPullRequests = list;
    }

    emit changed();
  });

  emit changed();
}

void PullRequests::setPullRequests(const QList<PullRequest> &pullRequests) {
  mPullRequests = pullRequests;
  mError.clear();
  emit changed();
}

QList<PullRequest> PullRequests::parse(Host host, const QByteArray &json,
                                       QString *error) {
  QList<PullRequest> result;
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
  if (!doc.isArray()) {
    if (error) {
      QString message = doc.object().value("message").toString();
      if (message.isEmpty())
        message = doc.object().value("error").toString();
      if (message.isEmpty() && parseError.error != QJsonParseError::NoError)
        message = parseError.errorString();
      *error = message.isEmpty() ? QString("Unexpected reply") : message;
    }
    return result;
  }

  for (const QJsonValue &value : doc.array()) {
    QJsonObject obj = value.toObject();
    PullRequest pr;
    pr.title = obj.value("title").toString();
    pr.created = date(obj.value("created_at"));
    pr.updated = date(obj.value("updated_at"));

    if (host == GitLab) {
      pr.number = obj.value("iid").toInt();
      pr.body = obj.value("description").toString();
      pr.author = obj.value("author").toObject().value("username").toString();
      pr.head = obj.value("source_branch").toString();
      pr.base = obj.value("target_branch").toString();
      pr.url = obj.value("web_url").toString();
      pr.draft = obj.value("draft").toBool() ||
                 obj.value("work_in_progress").toBool();
      pr.fork = obj.value("source_project_id").toInt() !=
                obj.value("target_project_id").toInt();
      for (const QJsonValue &label : obj.value("labels").toArray())
        pr.labels.append(label.toString());
    } else {
      QJsonObject head = obj.value("head").toObject();
      QJsonObject base = obj.value("base").toObject();
      pr.number = obj.value("number").toInt();
      pr.body = obj.value("body").toString();
      pr.author = obj.value("user").toObject().value("login").toString();
      pr.head = head.value("ref").toString();
      pr.base = base.value("ref").toString();
      pr.url = obj.value("html_url").toString();
      pr.draft = obj.value("draft").toBool();

      QString headRepo =
          head.value("repo").toObject().value("full_name").toString();
      QString baseRepo =
          base.value("repo").toObject().value("full_name").toString();
      pr.fork = headRepo != baseRepo;
      if (pr.fork)
        pr.headRepo = headRepo;

      for (const QJsonValue &label : obj.value("labels").toArray())
        pr.labels.append(label.toObject().value("name").toString());
    }

    result.append(pr);
  }

  return result;
}

bool PullRequests::parseUrl(const QString &url, QString *server,
                            QString *path) {
  QString text = url.trimmed();
  QString host;
  QString rest;
  if (text.contains("://")) {
    QUrl parsed(text);
    host = parsed.host();
    rest = parsed.path();
  } else {
    // Like git@github.com:owner/repo.git
    static const QRegularExpression scp("^(?:[^@/]+@)?([^:/]+):(.+)$");
    QRegularExpressionMatch match = scp.match(text);
    if (!match.hasMatch())
      return false;

    host = match.captured(1);
    rest = match.captured(2);
  }

  while (rest.startsWith('/'))
    rest.remove(0, 1);
  while (rest.endsWith('/'))
    rest.chop(1);
  if (rest.endsWith(".git"))
    rest.chop(4);

  if (host.isEmpty() || !rest.contains('/'))
    return false;

  *server = host.toLower();
  *path = rest;
  return true;
}
