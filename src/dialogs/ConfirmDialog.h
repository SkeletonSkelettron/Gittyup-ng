//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef CONFIRMDIALOG_H
#define CONFIRMDIALOG_H

#include "QmlDialog.h"

// Asks to confirm an action, like QMessageBox, with an optional check box
// and more buttons for alternative actions. qrc:/qml/ConfirmDialog.qml draws
// it.
class ConfirmDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString title READ title NOTIFY changed)
  Q_PROPERTY(QString text READ text NOTIFY changed)
  Q_PROPERTY(QString informativeText READ informativeText NOTIFY changed)
  Q_PROPERTY(QString detailedText READ detailedText NOTIFY changed)
  Q_PROPERTY(QString acceptText READ acceptText NOTIFY changed)
  Q_PROPERTY(QString checkText READ checkText NOTIFY changed)
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY changed)
  Q_PROPERTY(bool danger READ isDanger NOTIFY changed)
  Q_PROPERTY(bool warning READ isWarning NOTIFY changed)
  Q_PROPERTY(bool cancelVisible READ isCancelVisible NOTIFY changed)
  Q_PROPERTY(QStringList buttons READ buttons NOTIFY changed)

public:
  ConfirmDialog(QWidget *parent = nullptr);

  QString title() const { return mTitle; }
  void setTitle(const QString &title);

  QString text() const { return mText; }
  void setText(const QString &text);

  QString informativeText() const { return mInformativeText; }
  void setInformativeText(const QString &text);

  QString detailedText() const { return mDetailedText; }
  void setDetailedText(const QString &text);

  QString acceptText() const { return mAcceptText; }
  void setAcceptText(const QString &text);

  // The text of the check box. It's hidden if the text is empty.
  QString checkText() const { return mCheckText; }
  void setCheckText(const QString &text);

  bool isChecked() const { return mChecked; }
  void setChecked(bool checked);

  // The action destroys something.
  bool isDanger() const { return mDanger; }
  void setDanger(bool danger);

  // Show a warning sign without making the action destructive.
  bool isWarning() const { return mWarning; }
  void setWarning(bool warning);

  // Only show the accept button, to inform about something.
  bool isCancelVisible() const { return mCancelVisible; }
  void setCancelVisible(bool visible);

  // Add a button for an alternative action. It emits buttonClicked() with
  // the index of the button and closes the dialog.
  int addButton(const QString &text);
  QStringList buttons() const { return mButtons; }

  // The index of the button that closed the dialog, or -1.
  int clickedButton() const { return mClicked; }
  Q_INVOKABLE void clickButton(int index);

  // Show a message with an OK button.
  static ConfirmDialog *information(QWidget *parent, const QString &title,
                                    const QString &text,
                                    const QString &detailedText = QString());

  // Show a warning with an OK button and wait for it to close.
  static void warning(QWidget *parent, const QString &title,
                      const QString &text,
                      const QString &informativeText = QString());

signals:
  void changed();
  void buttonClicked(int index);

private:
  QString mTitle;
  QString mText;
  QString mInformativeText;
  QString mDetailedText;
  QString mAcceptText;
  QString mCheckText;
  bool mChecked = false;
  bool mDanger = false;
  bool mWarning = false;
  bool mCancelVisible = true;
  QStringList mButtons;
  int mClicked = -1;
};

#endif
