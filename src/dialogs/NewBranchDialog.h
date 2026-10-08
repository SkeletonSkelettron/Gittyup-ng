//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef NEWBRANCHDIALOG_H
#define NEWBRANCHDIALOG_H

#include "QmlDialog.h"
#include "ReferenceItems.h"
#include "git/Commit.h"
#include "git/Repository.h"

// Create a branch. qrc:/qml/NewBranchDialog.qml draws it.
class NewBranchDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(QString nameError READ nameError NOTIFY nameChanged)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY nameChanged)
  Q_PROPERTY(QString commitText READ commitText CONSTANT)
  Q_PROPERTY(QVariantList startPoints READ startPoints CONSTANT)
  Q_PROPERTY(int startPoint READ startPoint WRITE setStartPoint NOTIFY
                 startPointChanged)
  Q_PROPERTY(QVariantList upstreams READ upstreams CONSTANT)
  Q_PROPERTY(int upstreamIndex READ upstreamIndex WRITE setUpstreamIndex
                 NOTIFY upstreamChanged)
  Q_PROPERTY(bool checkout READ checkout WRITE setCheckout NOTIFY
                 checkoutChanged)
  Q_PROPERTY(bool checkoutVisible READ isCheckoutVisible CONSTANT)

public:
  NewBranchDialog(const git::Repository &repo,
                  const git::Commit &commit = git::Commit(),
                  QWidget *parent = nullptr);

  QString name() const { return mName; }
  void setName(const QString &name);
  QString nameError() const;
  bool isAcceptable() const;

  bool checkout() const { return mCheckout; }
  void setCheckout(bool checkout);
  bool isCheckoutVisible() const { return mCheckoutVisible; }

  // The commit to create the branch on, if it wasn't given.
  QString commitText() const;
  QVariantList startPoints() const { return mStartPoints.items(); }
  int startPoint() const { return mStartPoint; }
  void setStartPoint(int index);

  QVariantList upstreams() const { return mUpstreams.items(); }
  int upstreamIndex() const { return mUpstream; }
  void setUpstreamIndex(int index);

  git::Commit target() const;
  git::Reference upstream() const;

signals:
  void nameChanged();
  void checkoutChanged();
  void startPointChanged();
  void upstreamChanged();

private:
  git::Repository mRepo;
  git::Commit mCommit;
  QString mName;
  bool mCheckout = true;
  bool mCheckoutVisible;
  ReferenceItems mStartPoints;
  ReferenceItems mUpstreams;
  int mStartPoint = -1;
  int mUpstream = 0;
};

#endif
