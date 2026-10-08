//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef MERGEDIALOG_H
#define MERGEDIALOG_H

#include "QmlDialog.h"
#include "ReferenceItems.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include "ui/RepoView.h"

// Merge, rebase or squash a reference into HEAD. qrc:/qml/MergeDialog.qml
// draws it.
class MergeDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QVariantList refs READ refs NOTIFY changed)
  Q_PROPERTY(int refIndex READ refIndex WRITE setRefIndex NOTIFY changed)
  Q_PROPERTY(QStringList actions READ actions CONSTANT)
  Q_PROPERTY(int action READ action WRITE setAction NOTIFY changed)
  Q_PROPERTY(bool noCommit READ noCommit WRITE setNoCommit NOTIFY changed)
  Q_PROPERTY(bool noCommitVisible READ isNoCommitVisible NOTIFY changed)
  Q_PROPERTY(QString labelText READ labelText NOTIFY changed)
  Q_PROPERTY(QString buttonText READ buttonText NOTIFY changed)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY changed)

public:
  MergeDialog(RepoView::MergeFlags flags, const git::Repository &repo,
              QWidget *parent = nullptr);

  git::Commit target() const;
  git::Reference reference() const;
  RepoView::MergeFlags flags() const;

  void setCommit(const git::Commit &commit);
  void setReference(const git::Reference &ref);

  QVariantList refs() const { return mRefs.items(); }
  int refIndex() const { return mIndex; }
  void setRefIndex(int index);

  QStringList actions() const;
  int action() const { return mAction; }
  void setAction(int action);

  bool noCommit() const;
  void setNoCommit(bool noCommit);
  bool isNoCommitVisible() const;

  QString labelText() const;
  QString buttonText() const;
  bool isAcceptable() const { return target().isValid(); }

signals:
  void changed();

private:
  RepoView::MergeFlags actionFlags() const;

  git::Repository mRepo;
  git::Commit mCommit;
  ReferenceItems mRefs;
  int mIndex = -1;
  int mAction = 0;
};

#endif
