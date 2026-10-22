//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Claude Opus 5.5, an AI model by Anthropic
//

#include "Test.h"
#include "app/Theme.h"
#include "conf/Settings.h"
#include <QScopedPointer>

class TestTheme : public QObject {
  Q_OBJECT

private slots:
  void renamed_data();
  void renamed();
};

void TestTheme::renamed_data() {
  QTest::addColumn<QString>("saved");
  QTest::addColumn<QString>("name");

  QTest::newRow("dark") << "Kraken Dark"
                        << "Teal Dark";
  QTest::newRow("light") << "Kraken Light"
                         << "Teal Light";
}

// A theme that was chosen before it was renamed is still used, by its new name.
void TestTheme::renamed() {
  QFETCH(QString, saved);
  QFETCH(QString, name);

  Settings *settings = Settings::instance();
  settings->setValue(Setting::Id::ColorTheme, saved);

  QScopedPointer<Theme> theme(Theme::create());
  QCOMPARE(theme->name(), name);
  QCOMPARE(settings->value(Setting::Id::ColorTheme).toString(), name);
}

TEST_MAIN(TestTheme)

#include "theme.moc"
