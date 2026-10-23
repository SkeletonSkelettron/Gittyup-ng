//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DeleteBranchDialog.h"
#include "git/Branch.h"
#include "git/Config.h"
#include "git/Remote.h"
#include "ui/RepoView.h"

namespace {

const QString kBranchMergeFmt = "branch.%1.merge";

} // namespace

DeleteBranchDialog::DeleteBranchDialog(const git::Branch &branch,
                                       QWidget *parent)
    : ConfirmDialog(parent) {
  QString text = tr("Are you sure you want to delete local branch '%1'?");
  setTitle(tr("Delete Branch?"));
  setText(text.arg(branch.name()));
  setAcceptText(tr("Delete"));
  setDanger(true);

  git::Branch upstream = branch.upstream();
  if (upstream.isValid())
    setCheckText(tr("Also delete the upstream branch from its remote"));

  connect(this, &QDialog::accepted, [this, branch, upstream] {
    if (upstream.isValid() && isChecked()) {
      RepoView *view = RepoView::parentView(this);
      QString key = kBranchMergeFmt.arg(branch.name());
      QString upstreamName = view->repo().gitConfig().value<QString>(key);
      view->deleteRemoteBranch(upstream.remote(), upstreamName);
    }

    git::Branch(branch).remove();
  });

  if (!branch.isMerged()) {
    setInformativeText(tr("The branch is not fully merged. Deleting "
                          "it may cause some commits to be lost."));
  }
}
