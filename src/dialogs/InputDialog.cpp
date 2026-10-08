//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "InputDialog.h"

InputDialog::InputDialog(const QString &title, const QString &text,
                         const QList<Field> &fields, QWidget *parent,
                         const QString &acceptText)
    : QmlDialog(parent), mTitle(title), mText(text),
      mAcceptText(acceptText.isEmpty() ? tr("OK") : acceptText),
      mFields(fields) {
  setWindowTitle(title);
  for (const Field &field : fields)
    mValues.append(field.value);

  setContent("InputDialog");
}

QVariantList InputDialog::fields() const {
  QVariantList fields;
  for (const Field &field : mFields) {
    fields.append(QVariantMap{{"label", field.label},
                              {"value", field.value},
                              {"password", field.password},
                              {"placeholder", field.placeholder}});
  }

  return fields;
}

void InputDialog::setValue(int index, const QString &value) {
  if (index < 0 || index >= mValues.size() || mValues.at(index) == value)
    return;

  mValues[index] = value;
  emit valuesChanged();
}

bool InputDialog::isAcceptable() const {
  for (int i = 0; i < mFields.size(); ++i) {
    if (mFields.at(i).required && mValues.at(i).isEmpty())
      return false;
  }

  return true;
}
