//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "QmlDialog.h"

// Information about Gittyup. qrc:/qml/AboutDialog.qml draws it.
class AboutDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QString version READ version CONSTANT)
  Q_PROPERTY(QString build READ build CONSTANT)
  Q_PROPERTY(QString copyright READ copyright CONSTANT)
  Q_PROPERTY(QString support READ support CONSTANT)
  Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
  Q_PROPERTY(QString document READ document NOTIFY indexChanged)

public:
  // Keep in sync with AboutDialog.qml.
  enum Index { Changelog, Acknowledgments, Privacy };

  AboutDialog(QWidget *parent = nullptr);

  static void openSharedInstance(Index index = Changelog);

  QString name() const;
  QString version() const;
  QString build() const;
  QString copyright() const;
  QString support() const;

  int index() const { return mIndex; }
  void setIndex(int index);

  // The HTML of the current tab.
  QString document() const;

signals:
  void indexChanged();

private:
  int mIndex = Changelog;
};

#endif
