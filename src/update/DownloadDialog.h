//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DOWNLOADDIALOG_H
#define DOWNLOADDIALOG_H

#include "Updater.h"
#include "dialogs/QmlDialog.h"

// The progress of downloading an update. qrc:/qml/DownloadDialog.qml draws
// it.
class DownloadDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString text READ text NOTIFY changed)
  Q_PROPERTY(qreal progress READ progress NOTIFY changed)
  Q_PROPERTY(bool complete READ isComplete NOTIFY changed)

public:
  DownloadDialog(const Updater::DownloadRef &download,
                 QWidget *parent = nullptr);

  QString text() const { return mText; }
  qreal progress() const { return mProgress; }
  bool isComplete() const { return mComplete; }

signals:
  void changed();

private:
  QString mText;
  qreal mProgress = 0;
  bool mComplete = false;
};

#endif
