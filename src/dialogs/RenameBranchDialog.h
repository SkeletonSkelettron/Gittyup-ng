//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef RENAMEBRANCHDIALOG_H
#define RENAMEBRANCHDIALOG_H

#include "QmlDialog.h"
#include "git/Branch.h"
#include "git/Repository.h"

// Rename a local branch when accepted. qrc:/qml/RenameBranchDialog.qml
// draws it.
class RenameBranchDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString oldName READ oldName CONSTANT)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(QString nameError READ nameError NOTIFY nameChanged)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY nameChanged)

public:
  RenameBranchDialog(const git::Repository &repo, const git::Branch &branch,
                     QWidget *parent = nullptr);

  QString oldName() const { return mBranch.name(); }
  QString name() const { return mName; }
  void setName(const QString &name);
  QString nameError() const;
  bool isAcceptable() const;

signals:
  void nameChanged();

private:
  git::Repository mRepo;
  git::Branch mBranch;
  QString mName;
};

#endif
