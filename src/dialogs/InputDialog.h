//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include "QmlDialog.h"
#include <QStringList>
#include <QVariantList>

// Ask for one or more lines of text, like a user name and a password.
// qrc:/qml/InputDialog.qml draws it.
class InputDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString title READ title CONSTANT)
  Q_PROPERTY(QString text READ text CONSTANT)
  Q_PROPERTY(QString acceptText READ acceptText CONSTANT)
  Q_PROPERTY(QVariantList fields READ fields CONSTANT)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY valuesChanged)

public:
  struct Field {
    QString label;
    QString value;
    // Hide the text as it's typed.
    bool password = false;
    // The dialog can't be accepted while the field is empty.
    bool required = true;
    // Shown while the field is empty.
    QString placeholder;
  };

  InputDialog(const QString &title, const QString &text,
              const QList<Field> &fields, QWidget *parent = nullptr,
              const QString &acceptText = QString());

  QString title() const { return mTitle; }
  QString text() const { return mText; }
  QString acceptText() const { return mAcceptText; }
  QVariantList fields() const;

  QString value(int index) const { return mValues.value(index); }
  QStringList values() const { return mValues; }
  Q_INVOKABLE void setValue(int index, const QString &value);

  bool isAcceptable() const;

signals:
  void valuesChanged();

private:
  QString mTitle;
  QString mText;
  QString mAcceptText;
  QList<Field> mFields;
  QStringList mValues;
};

#endif
