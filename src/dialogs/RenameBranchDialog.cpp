//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RenameBranchDialog.h"

RenameBranchDialog::RenameBranchDialog(const git::Repository &repo,
                                       const git::Branch &branch,
                                       QWidget *parent)
    : QmlDialog(parent), mRepo(repo), mBranch(branch), mName(branch.name()) {
  Q_ASSERT(branch.isValid() && branch.isLocalBranch());
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Rename Branch"));

  // Perform the rename when accepted.
  connect(this, &QDialog::accepted, this,
          [this] { git::Branch(mBranch).rename(mName); });

  setContent("RenameBranchDialog");
}

void RenameBranchDialog::setName(const QString &name) {
  if (name == mName)
    return;

  mName = name;
  emit nameChanged();
}

QString RenameBranchDialog::nameError() const {
  if (mName.isEmpty() || mName == mBranch.name())
    return QString();

  if (!git::Branch::isNameValid(mName))
    return tr("This isn't a valid branch name.");

  if (mRepo.lookupBranch(mName, GIT_BRANCH_LOCAL).isValid())
    return tr("A branch with this name already exists.");

  return QString();
}

bool RenameBranchDialog::isAcceptable() const {
  return !mName.isEmpty() && mName != mBranch.name() && nameError().isEmpty();
}
