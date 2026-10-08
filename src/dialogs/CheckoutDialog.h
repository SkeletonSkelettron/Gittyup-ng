//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CHECKOUTDIALOG_H
#define CHECKOUTDIALOG_H

#include "QmlDialog.h"
#include "ReferenceItems.h"

namespace git {
class Repository;
} // namespace git

// Check out a reference. qrc:/qml/CheckoutDialog.qml draws it.
class CheckoutDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QVariantList refs READ refs CONSTANT)
  Q_PROPERTY(int refIndex READ refIndex WRITE setRefIndex NOTIFY changed)
  Q_PROPERTY(bool detachEnabled READ isDetachEnabled NOTIFY changed)
  Q_PROPERTY(bool detachChecked READ isDetachChecked NOTIFY changed)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY changed)

public:
  CheckoutDialog(const git::Repository &repo, const git::Reference &ref,
                 QWidget *parent = nullptr);

  git::Reference reference() const;
  bool detach() const { return mDetach; }
  Q_INVOKABLE void setDetach(bool detach);

  QVariantList refs() const { return mRefs.items(); }
  int refIndex() const { return mIndex; }
  void setRefIndex(int index);

  // Only local branches can be checked out without detaching HEAD.
  bool isDetachEnabled() const;
  bool isDetachChecked() const;
  bool isAcceptable() const;

signals:
  void changed();

private:
  ReferenceItems mRefs;
  int mIndex = -1;
  bool mDetach = false;
};

#endif
