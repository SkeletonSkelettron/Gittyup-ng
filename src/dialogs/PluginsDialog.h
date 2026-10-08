//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef PLUGINSDIALOG_H
#define PLUGINSDIALOG_H

#include "QmlDialog.h"
#include "git/Repository.h"
#include "plugins/Plugin.h"
#include <QVariantList>

// The plugins of the application or a repository with their options and
// diagnostics. qrc:/qml/PluginsDialog.qml draws it.
class PluginsDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QVariantList plugins READ plugins NOTIFY changed)

public:
  PluginsDialog(const git::Repository &repo, QWidget *parent = nullptr);

  QVariantList plugins() const;

  Q_INVOKABLE void setOption(int plugin, const QString &key,
                             const QVariant &value);
  Q_INVOKABLE void setDiagnosticEnabled(int plugin, const QString &key,
                                        bool enabled);
  Q_INVOKABLE void setDiagnosticKind(int plugin, const QString &key, int kind);

signals:
  void changed();

private:
  QList<PluginRef> mPlugins;
};

#endif
