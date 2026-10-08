//
//          Copyright (c) 2022, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef IGNOREDIALOG_H
#define IGNOREDIALOG_H

#include "dialogs/QmlDialog.h"

// Edit the patterns to add to .gitignore. qrc:/qml/IgnoreDialog.qml draws
// it.
class IgnoreDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString pattern READ ignoreText WRITE setIgnoreText NOTIFY
                 patternChanged)

public:
  IgnoreDialog(const QString &ignore, QWidget *parent = nullptr);

  QString ignoreText() const { return mIgnore; }
  void setIgnoreText(const QString &text);

signals:
  void patternChanged();

private:
  QString mIgnore;
};

#endif
