//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef PULLREQUESTDIALOG_H
#define PULLREQUESTDIALOG_H

#include "QmlDialog.h"
#include <QStringList>
#include <QVariantMap>

class RepoView;
class Repository;

namespace git {
class Commit;
}

// Create a pull request on the hosting service.
// qrc:/qml/PullRequestDialog.qml draws it.
class PullRequestDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY changed)
  Q_PROPERTY(QString body READ body WRITE setBody NOTIFY changed)
  Q_PROPERTY(bool maintainerCanModify MEMBER mMaintainerCanModify NOTIFY
                 changed)
  Q_PROPERTY(QStringList branches READ branches CONSTANT)
  Q_PROPERTY(int branch READ branch WRITE setBranch NOTIFY changed)
  Q_PROPERTY(QStringList parents READ parents NOTIFY parentsChanged)
  Q_PROPERTY(QString toRepo MEMBER mToRepo NOTIFY changed)
  Q_PROPERTY(QString toBranch MEMBER mToBranch NOTIFY changed)

public:
  PullRequestDialog(RepoView *view);

  QString title() const { return mTitle; }
  void setTitle(const QString &title);
  QString body() const { return mBody; }
  void setBody(const QString &body);

  QStringList branches() const { return mBranches; }
  int branch() const { return mBranch; }
  void setBranch(int branch);

  // The repositories that the remote repository was forked from.
  QStringList parents() const { return mParents.keys(); }
  Q_INVOKABLE void chooseParent(const QString &repo);

  Q_INVOKABLE void create();

signals:
  void changed();
  void parentsChanged();

private:
  void setCommit(const git::Commit &commit);

  RepoView *mView;
  Repository *mRemoteRepo;
  QString mTitle;
  QString mBody;
  bool mMaintainerCanModify = true;
  QStringList mBranches;
  int mBranch = -1;
  QMap<QString, QString> mParents;
  QString mToRepo;
  QString mToBranch;
};

#endif
