//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef PULLREQUESTLIST_H
#define PULLREQUESTLIST_H

#include "host/PullRequests.h"
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class RepoView;

// The open pull requests of the repository of a view, like GitKraken lists
// them in its left panel. The references panel lists them, and a pull
// request that's shown replaces the graph with its description.
// QML pages see it as 'pullRequests'.
class PullRequestList : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool supported READ isSupported NOTIFY changed)
  Q_PROPERTY(bool loading READ isLoading NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)
  Q_PROPERTY(QString service READ service NOTIFY changed)
  // The pull request that's shown, or an empty map.
  Q_PROPERTY(QVariantMap current READ current NOTIFY currentChanged)
  Q_PROPERTY(bool active READ isActive NOTIFY currentChanged)
  Q_PROPERTY(bool canCheckout READ canCheckout NOTIFY currentChanged)

public:
  PullRequestList(RepoView *view);

  PullRequests *source() const { return mSource; }
  const QList<PullRequest> &pullRequests() const {
    return mSource->pullRequests();
  }

  bool isSupported() const { return mSource->isSupported(); }
  bool isLoading() const { return mSource->isLoading(); }
  QString error() const { return mSource->error(); }
  QString service() const { return mSource->hostName(); }

  QVariantMap current() const;
  bool isActive() const { return mCurrent > 0; }
  // The branch of the pull request is on the remote of the repository.
  bool canCheckout() const;

  Q_INVOKABLE void refresh();
  // Show the pull request with 'number' in place of the graph.
  Q_INVOKABLE void show(int number);
  Q_INVOKABLE void close();
  // Check out a local branch of the pull request that's shown.
  Q_INVOKABLE void checkout();
  Q_INVOKABLE void openInBrowser();

signals:
  void changed();
  void currentChanged();

private:
  const PullRequest *find(int number) const;
  QString remoteBranch() const;

  RepoView *mView;
  PullRequests *mSource;
  QString mRemote;
  int mCurrent = 0;
  QTimer mTimer;
};

#endif
