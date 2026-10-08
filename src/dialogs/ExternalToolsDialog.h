//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef EXTERNALTOOLSDIALOG_H
#define EXTERNALTOOLSDIALOG_H

#include "QmlDialog.h"
#include <QVariantList>

class ExternalToolsModel;

// The detected and the user defined external diff or merge tools.
// qrc:/qml/ExternalToolsDialog.qml draws it.
class ExternalToolsDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QString type READ type CONSTANT)
  Q_PROPERTY(QVariantList detected READ detected NOTIFY changed)
  Q_PROPERTY(QVariantList userDefined READ userDefined NOTIFY changed)

public:
  ExternalToolsDialog(const QString &type, QWidget *parent = nullptr);

  QString type() const { return mType; }
  QVariantList detected() const;
  QVariantList userDefined() const;

  // Column 0 is the name, 1 the command and 2 the arguments.
  Q_INVOKABLE void setToolValue(int row, int column, const QString &value);
  Q_INVOKABLE void addTool();
  Q_INVOKABLE void removeTool(int row);

signals:
  void changed();

private:
  QVariantList items(ExternalToolsModel *model) const;

  QString mType;
  ExternalToolsModel *mDetected;
  ExternalToolsModel *mUserDefined;
};

#endif
