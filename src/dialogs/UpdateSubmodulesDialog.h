//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef UPDATESUBMODULESDIALOG_H
#define UPDATESUBMODULESDIALOG_H

#include "QmlDialog.h"
#include "git/Submodule.h"

namespace git {
class Repository;
} // namespace git

// Choose the submodules to update. qrc:/qml/UpdateSubmodulesDialog.qml
// draws it.
class UpdateSubmodulesDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QVariantList submodules READ submoduleItems NOTIFY changed)
  Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY changed)
  Q_PROPERTY(bool init READ init WRITE setInit NOTIFY changed)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY changed)

public:
  UpdateSubmodulesDialog(const git::Repository &repo,
                         QWidget *parent = nullptr);

  // The submodules to update.
  QList<git::Submodule> submodules() const;
  QVariantList submoduleItems() const;
  Q_INVOKABLE void setEnabled(int index, bool enabled);
  Q_INVOKABLE void setAllEnabled(bool enabled);

  bool recursive() const { return mRecursive; }
  void setRecursive(bool recursive);

  bool init() const { return mInit; }
  void setInit(bool init);

  bool isAcceptable() const { return !submodules().isEmpty(); }

signals:
  void changed();

private:
  QList<git::Submodule> mSubmodules;
  QList<bool> mEnabled;
  bool mRecursive = true;
  bool mInit = false;
};

#endif
