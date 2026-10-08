//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "ExternalToolsDialog.h"
#include "dialogs/ExternalToolsModel.h"
#include <QFileDialog>

ExternalToolsDialog::ExternalToolsDialog(const QString &type, QWidget *parent)
    : QmlDialog(parent), mType(type),
      mDetected(new ExternalToolsModel(type, true, this)),
      mUserDefined(new ExternalToolsModel(type, false, this)) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Configure External Tools"));
  setModal(false);
  setContent("ExternalToolsDialog");
}

QVariantList ExternalToolsDialog::detected() const {
  return items(mDetected);
}

QVariantList ExternalToolsDialog::userDefined() const {
  return items(mUserDefined);
}

void ExternalToolsDialog::setToolValue(int row, int column,
                                       const QString &value) {
  QModelIndex index = mUserDefined->index(row, column);
  if (!index.isValid() || index.data().toString() == value)
    return;

  mUserDefined->setData(index, value);
  emit changed();
}

void ExternalToolsDialog::addTool() {
  QString path = QFileDialog::getOpenFileName(this, tr("Select Executable"));
  if (path.isEmpty())
    return;

  mUserDefined->add(path);
  mUserDefined->refresh();
  emit changed();
}

void ExternalToolsDialog::removeTool(int row) {
  QModelIndex index = mUserDefined->index(row, ExternalToolsModel::Name);
  if (!index.isValid())
    return;

  mUserDefined->remove(index.data().toString());
  mUserDefined->refresh();
  emit changed();
}

QVariantList ExternalToolsDialog::items(ExternalToolsModel *model) const {
  QVariantList items;
  for (int row = 0; row < model->rowCount(); ++row) {
    QModelIndex name = model->index(row, ExternalToolsModel::Name);
    items.append(QVariantMap{
        {"name", name.data()},
        {"command", model->index(row, ExternalToolsModel::Command).data()},
        {"arguments", model->index(row, ExternalToolsModel::Arguments).data()},
        {"found", model->flags(name) != Qt::ItemFlags()}});
  }

  return items;
}
