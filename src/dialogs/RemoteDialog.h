//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REMOTEDIALOG_H
#define REMOTEDIALOG_H

#include "QmlDialog.h"
#include "ReferenceItems.h"
#include "git/Repository.h"

class RepoView;

// Fetch, pull or push with options. qrc:/qml/RemoteDialog.qml draws it.
class RemoteDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(int kind READ kind CONSTANT)
  Q_PROPERTY(QString remote READ remote WRITE setRemote NOTIFY changed)
  Q_PROPERTY(bool hasRemotes READ hasRemotes CONSTANT)
  Q_PROPERTY(bool tags READ tags WRITE setTags NOTIFY changed)
  Q_PROPERTY(bool prune READ prune WRITE setPrune NOTIFY changed)
  Q_PROPERTY(QStringList actions READ actions CONSTANT)
  Q_PROPERTY(int action READ action WRITE setAction NOTIFY changed)
  Q_PROPERTY(QVariantList refs READ refs CONSTANT)
  Q_PROPERTY(int refIndex READ refIndex WRITE setRefIndex NOTIFY changed)
  Q_PROPERTY(QString remoteRef READ remoteRef WRITE setRemoteRef NOTIFY
                 changed)
  Q_PROPERTY(bool setUpstream READ setUpstream WRITE setSetUpstream NOTIFY
                 changed)
  Q_PROPERTY(bool force READ force WRITE setForce NOTIFY changed)

public:
  // Keep in sync with RemoteDialog.qml.
  enum Kind { Fetch, Pull, Push };

  RemoteDialog(Kind kind, RepoView *parent);

  int kind() const { return mKind; }

  QString remote() const { return mRemote; }
  void setRemote(const QString &remote);
  bool hasRemotes() const { return !mRepo.remotes().isEmpty(); }
  Q_INVOKABLE void showRemoteMenu(qreal x, qreal y);

  bool tags() const { return mTags; }
  void setTags(bool tags);

  bool prune() const { return mPrune; }
  void setPrune(bool prune);

  QStringList actions() const;
  int action() const { return mAction; }
  void setAction(int action);

  QVariantList refs() const { return mRefs.items(); }
  int refIndex() const { return mRefIndex; }
  void setRefIndex(int index);

  QString remoteRef() const { return mRemoteRef; }
  void setRemoteRef(const QString &ref);

  bool setUpstream() const { return mSetUpstream; }
  void setSetUpstream(bool setUpstream);

  bool force() const { return mForce; }
  void setForce(bool force);

signals:
  void changed();

private:
  void run();

  Kind mKind;
  git::Repository mRepo;
  QString mRemote;
  bool mTags = false;
  bool mPrune = false;
  int mAction = 0;
  ReferenceItems mRefs;
  int mRefIndex = -1;
  QString mRemoteRef;
  bool mSetUpstream = false;
  bool mForce = false;
};

#endif
