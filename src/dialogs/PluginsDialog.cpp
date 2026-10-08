//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "PluginsDialog.h"

PluginsDialog::PluginsDialog(const git::Repository &repo, QWidget *parent)
    : QmlDialog(parent), mPlugins(Plugin::plugins(repo)) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Plugins"));
  setContent("PluginsDialog");
}

QVariantList PluginsDialog::plugins() const {
  QVariantList plugins;
  for (const PluginRef &plugin : mPlugins) {
    QVariantList options;
    QVariantList diagnostics;
    if (plugin->isValid()) {
      for (const QString &key : plugin->optionKeys()) {
        options.append(QVariantMap{{"key", key},
                                   {"text", plugin->optionText(key)},
                                   {"kind", plugin->optionKind(key)},
                                   {"value", plugin->optionValue(key)},
                                   {"opts", plugin->optionOpts(key)}});
      }

      for (const QString &key : plugin->diagnosticKeys()) {
        diagnostics.append(
            QVariantMap{{"key", key},
                        {"name", plugin->diagnosticName(key)},
                        {"description", plugin->diagnosticDescription(key)},
                        {"enabled", plugin->isEnabled(key)},
                        {"kind", plugin->diagnosticKind(key)}});
      }
    }

    plugins.append(QVariantMap{{"name", plugin->name()},
                               {"valid", plugin->isValid()},
                               {"error", plugin->errorString()},
                               {"options", options},
                               {"diagnostics", diagnostics}});
  }

  return plugins;
}

void PluginsDialog::setOption(int plugin, const QString &key,
                              const QVariant &value) {
  if (plugin < 0 || plugin >= mPlugins.size())
    return;

  mPlugins.at(plugin)->setOptionValue(key, value);
}

void PluginsDialog::setDiagnosticEnabled(int plugin, const QString &key,
                                         bool enabled) {
  if (plugin < 0 || plugin >= mPlugins.size())
    return;

  mPlugins.at(plugin)->setEnabled(key, enabled);
}

void PluginsDialog::setDiagnosticKind(int plugin, const QString &key,
                                      int kind) {
  if (plugin < 0 || plugin >= mPlugins.size())
    return;

  mPlugins.at(plugin)->setDiagnosticKind(
      key, static_cast<Plugin::DiagnosticKind>(kind));
}
