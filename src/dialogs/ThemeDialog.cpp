//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "ThemeDialog.h"
#include "conf/Settings.h"
#include <QCoreApplication>

ThemeDialog::ThemeDialog(QWidget *parent) : QmlDialog(parent) {
  setWindowTitle(
      tr("Pick a theme for %1").arg(QCoreApplication::applicationName()));
  setContent("ThemeDialog");
}

QVariantList ThemeDialog::themes() const {
  auto theme = [](const QString &name, const QString &title,
                  const QString &description, const QString &image) {
    return QVariantMap{{"name", name},
                       {"title", title},
                       {"description", description},
                       {"image", QString("qrc:/%1").arg(image)}};
  };

  return {theme("Kraken Dark", tr("Kraken Dark"),
                tr("A flat dark theme with a teal accent"), "kraken_dark.png"),
          theme("Kraken Light", tr("Kraken Light"),
                tr("A flat light theme with a teal accent"),
                "kraken_light.png"),
          theme("Mocha", tr("Catppuccin Mocha"),
                tr("A more modern dark theme"), "mocha.png"),
          theme("Dark", tr("Dark"),
                tr("A consistent look optimal for reducing eye strain"),
                "dark.png"),
          theme("Default", tr("Default"), tr("A consistent bright theme"),
                "native.png"),
          theme("System", tr("System"),
                tr("A flexible look matching system colors"), "system.png")};
}

void ThemeDialog::choose(const QString &name) {
  Settings::instance()->setValue(Setting::Id::ColorTheme, name);
  accept();
}
