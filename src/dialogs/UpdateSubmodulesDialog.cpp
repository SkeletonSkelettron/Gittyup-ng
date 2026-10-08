//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "UpdateSubmodulesDialog.h"
#include "git/Repository.h"

UpdateSubmodulesDialog::UpdateSubmodulesDialog(const git::Repository &repo,
                                               QWidget *parent)
    : QmlDialog(parent), mSubmodules(repo.submodules()),
      mEnabled(mSubmodules.size(), true) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Update Submodules"));
  setContent("UpdateSubmodulesDialog");
}

QList<git::Submodule> UpdateSubmodulesDialog::submodules() const {
  QList<git::Submodule> submodules;
  for (int i = 0; i < mSubmodules.size(); ++i) {
    if (mEnabled.at(i))
      submodules.append(mSubmodules.at(i));
  }

  return submodules;
}

QVariantList UpdateSubmodulesDialog::submoduleItems() const {
  QVariantList items;
  for (int i = 0; i < mSubmodules.size(); ++i) {
    const git::Submodule &submodule = mSubmodules.at(i);
    items.append(QVariantMap{{"name", submodule.name()},
                             {"path", submodule.path()},
                             {"enabled", mEnabled.at(i)}});
  }

  return items;
}

void UpdateSubmodulesDialog::setEnabled(int index, bool enabled) {
  if (index < 0 || index >= mEnabled.size() || mEnabled.at(index) == enabled)
    return;

  mEnabled[index] = enabled;
  emit changed();
}

void UpdateSubmodulesDialog::setAllEnabled(bool enabled) {
  mEnabled.fill(enabled);
  emit changed();
}

void UpdateSubmodulesDialog::setRecursive(bool recursive) {
  if (recursive == mRecursive)
    return;

  mRecursive = recursive;
  emit changed();
}

void UpdateSubmodulesDialog::setInit(bool init) {
  if (init == mInit)
    return;

  mInit = init;
  emit changed();
}
