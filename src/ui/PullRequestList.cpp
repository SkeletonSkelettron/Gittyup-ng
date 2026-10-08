//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "PullRequestList.h"
#include "RepoView.h"
#include "git/Branch.h"
#include "git/Remote.h"
#include "host/Accounts.h"
#include <QDesktopServices>
#include <QLocale>
#include <QUrl>

namespace {

// The pull requests are listed again this often.
const int kRefreshInterval = 5 * 60 * 1000;

} // namespace

PullRequestList::PullRequestList(RepoView *view)
    : QObject(view), mView(view), mSource(new PullRequests(this)) {
  connect(mSource, &PullRequests::changed, this, [this] {
    // The pull request that's shown may have been closed.
    if (mCurrent > 0 && !find(mCurrent) && !mSource->isLoading())
      close();
    emit changed();
    emit currentChanged();
  });

  // Look up the account of the default remote for private repositories.
  git::Remote remote = view->repo().defaultRemote();
  if (remote.isValid()) {
    mRemote = remote.name();
    Repository *repo = Accounts::instance()->lookup(remote.url());
    mSource->setRemote(remote.url(), repo ? repo->account() : nullptr);
  }

  if (mSource->isSupported()) {
    mTimer.setInterval(kRefreshInterval);
    connect(&mTimer, &QTimer::timeout, this, &PullRequestList::refresh);
    mTimer.start();
    refresh();
  }
}

QVariantMap PullRequestList::current() const {
  const PullRequest *pr = find(mCurrent);
  if (!pr)
    return QVariantMap();

  QLocale locale;
  return {{"number", pr->number},
          {"title", pr->title},
          {"body", pr->body},
          {"author", pr->author},
          {"head", pr->fork ? QString("%1:%2").arg(pr->headRepo, pr->head)
                            : pr->head},
          {"base", pr->base},
          {"url", pr->url},
          {"draft", pr->draft},
          {"fork", pr->fork},
          {"labels", pr->labels},
          {"created", locale.toString(pr->created.toLocalTime(),
                                      QLocale::ShortFormat)},
          {"updated", locale.toString(pr->updated.toLocalTime(),
                                      QLocale::ShortFormat)}};
}

bool PullRequestList::canCheckout() const {
  return !remoteBranch().isEmpty() && !mView->repo().isBare();
}

void PullRequestList::refresh() { mSource->refresh(); }

void PullRequestList::show(int number) {
  if (!find(number) || mCurrent == number)
    return;

  mCurrent = number;
  emit currentChanged();
}

void PullRequestList::close() {
  if (mCurrent == 0)
    return;

  mCurrent = 0;
  emit currentChanged();
}

void PullRequestList::checkout() {
  const PullRequest *pr = find(mCurrent);
  QString name = remoteBranch();
  if (!pr || name.isEmpty())
    return;

  git::Repository repo = mView->repo();
  git::Reference upstream = repo.lookupRef(name);

  // Check out the local branch, or create one that tracks the remote one.
  git::Branch local = repo.lookupBranch(pr->head, GIT_BRANCH_LOCAL);
  if (local.isValid()) {
    if (!local.isHead())
      mView->checkout(local);
  } else {
    mView->createBranch(pr->head, upstream.target(), upstream, true);
  }

  close();
}

void PullRequestList::openInBrowser() {
  if (const PullRequest *pr = find(mCurrent))
    QDesktopServices::openUrl(QUrl(pr->url));
}

const PullRequest *PullRequestList::find(int number) const {
  const QList<PullRequest> &list = mSource->pullRequests();
  for (const PullRequest &pr : list) {
    if (pr.number == number)
      return &pr;
  }

  return nullptr;
}

QString PullRequestList::remoteBranch() const {
  const PullRequest *pr = find(mCurrent);
  if (!pr || pr->fork || mRemote.isEmpty())
    return QString();

  QString name = QString("refs/remotes/%1/%2").arg(mRemote, pr->head);
  return mView->repo().lookupRef(name).isValid() ? name : QString();
}
