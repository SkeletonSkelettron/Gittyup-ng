//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef THEMEDIALOG_H
#define THEMEDIALOG_H

#include "QmlDialog.h"
#include <QVariantList>

// Pick the theme when Gittyup starts for the first time.
// qrc:/qml/ThemeDialog.qml draws it.
class ThemeDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QVariantList themes READ themes CONSTANT)

public:
  ThemeDialog(QWidget *parent = nullptr);

  QVariantList themes() const;
  Q_INVOKABLE void choose(const QString &name);
};

#endif
