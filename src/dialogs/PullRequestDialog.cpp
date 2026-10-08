//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "PullRequestDialog.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "host/Account.h"
#include "host/Repository.h"
#include "ui/RepoView.h"

PullRequestDialog::PullRequestDialog(RepoView *view)
    : QmlDialog(view), mView(view), mRemoteRepo(view->remoteRepo()) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Create Pull Request"));

  git::Repository repo = view->repo();
  for (const git::Branch &branch : repo.branches(GIT_BRANCH_LOCAL))
    mBranches.append(branch.name());
  mBranch = mBranches.indexOf(repo.head().name());
  setCommit(repo.head().target());

  mRemoteRepo->account()->requestForkParents(mRemoteRepo);
  connect(mRemoteRepo->account(), &Account::forkParentsReady, this,
          [this](const QMap<QString, QString> &parents) {
            mParents = parents;
            emit parentsChanged();
          });

  setContent("PullRequestDialog");
}

void PullRequestDialog::setTitle(const QString &title) {
  if (title == mTitle)
    return;

  mTitle = title;
  emit changed();
}

void PullRequestDialog::setBody(const QString &body) {
  if (body == mBody)
    return;

  mBody = body;
  emit changed();
}

void PullRequestDialog::setBranch(int branch) {
  if (branch == mBranch || branch < 0 || branch >= mBranches.size())
    return;

  mBranch = branch;
  git::Branch ref =
      mView->repo().lookupBranch(mBranches.at(branch), GIT_BRANCH_LOCAL);
  setCommit(ref.target());
  emit changed();
}

void PullRequestDialog::chooseParent(const QString &repo) {
  mToRepo = repo;
  mToBranch = mParents.value(repo);
  emit changed();
}

void PullRequestDialog::create() {
  if (mToRepo.isEmpty() || mToBranch.isEmpty())
    return;

  mRemoteRepo->account()->createPullRequest(
      mRemoteRepo, mToRepo, mTitle, mBody, mBranches.value(mBranch),
      mToBranch, mMaintainerCanModify);
  accept();
}

void PullRequestDialog::setCommit(const git::Commit &commit) {
  mTitle = commit.summary();
  mBody = commit.body().isEmpty() ? QString()
                                  : QString("%1\n\n").arg(commit.body());
  emit changed();
}
