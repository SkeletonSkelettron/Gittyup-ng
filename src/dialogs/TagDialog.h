//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TAGDIALOG_H
#define TAGDIALOG_H

#include "QmlDialog.h"
#include "git/Remote.h"
#include "git/Repository.h"
#include <QStringList>

// Create a tag. qrc:/qml/TagDialog.qml draws it.
class TagDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString target READ target CONSTANT)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
  Q_PROPERTY(QString nameError READ nameError NOTIFY changed)
  Q_PROPERTY(bool force READ force WRITE setForce NOTIFY changed)
  Q_PROPERTY(bool annotated READ isAnnotated WRITE setAnnotated NOTIFY changed)
  Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY changed)
  Q_PROPERTY(bool push READ push WRITE setPush NOTIFY changed)
  Q_PROPERTY(QString pushText READ pushText CONSTANT)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY changed)
  Q_PROPERTY(QStringList existingTags READ existingTags NOTIFY changed)

public:
  TagDialog(const git::Repository &repo, const QString &id,
            const git::Remote &remote = git::Remote(),
            QWidget *parent = nullptr);

  QString target() const { return mTarget; }

  QString name() const { return mName; }
  void setName(const QString &name);
  QString nameError() const;

  bool force() const { return mForce; }
  void setForce(bool force);

  bool isAnnotated() const { return mAnnotated; }
  void setAnnotated(bool annotated);

  // The message of an annotated tag.
  QString message() const { return mAnnotated ? mMessage : QString(); }
  void setMessage(const QString &message);

  bool push() const { return mPush; }
  void setPush(bool push);
  QString pushText() const;

  bool isAcceptable() const;

  // The existing tags that contain the name, newest versions first.
  QStringList existingTags() const;

  // The remote to push the tag to, if any.
  git::Remote remote() const;

signals:
  void changed();

private:
  git::Repository mRepo;
  git::Remote mRemote;
  QString mTarget;
  QString mName;
  QString mMessage;
  bool mForce = false;
  bool mAnnotated = false;
  bool mPush = false;
  QStringList mExistingTags;
};

#endif
