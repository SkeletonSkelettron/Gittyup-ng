//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef COMMITDIALOG_H
#define COMMITDIALOG_H

#include "QmlDialog.h"
#include "conf/Settings.h"

// Edit the message of a merge, stash, revert or cherry-pick commit.
// qrc:/qml/CommitDialog.qml draws it.
class CommitDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString title READ title CONSTANT)
  Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY
                 messageChanged)
  Q_PROPERTY(QString acceptText READ acceptText CONSTANT)
  Q_PROPERTY(QString rejectText READ rejectText CONSTANT)
  Q_PROPERTY(QString promptText READ promptText CONSTANT)
  Q_PROPERTY(bool prompt READ prompt WRITE setPrompt NOTIFY promptChanged)

public:
  CommitDialog(const QString &message, Prompt::Kind kind,
               QWidget *parent = nullptr);

  QString title() const { return mTitle; }
  QString message() const { return mMessage; }
  void setMessage(const QString &message);

  QString acceptText() const { return mAcceptText; }
  QString rejectText() const { return mRejectText; }

  // Whether to ask for the message the next time.
  QString promptText() const;
  bool prompt() const;
  void setPrompt(bool prompt);

signals:
  void messageChanged();
  void promptChanged();

private:
  Prompt::Kind mKind;
  QString mTitle;
  QString mMessage;
  QString mAcceptText;
  QString mRejectText;
};

#endif
