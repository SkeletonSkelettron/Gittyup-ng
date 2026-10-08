//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "QmlTheme.h"
#include "app/Application.h"
#include "app/Theme.h"
#include "conf/Settings.h"
#include <QFontDatabase>
#include <QPalette>

namespace {

QColor mix(const QColor &a, const QColor &b, qreal t) {
  return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                          a.greenF() + (b.greenF() - a.greenF()) * t,
                          a.blueF() + (b.blueF() - a.blueF()) * t);
}

} // namespace

QmlTheme *QmlTheme::instance() {
  // Colors that are read while the theme is being chosen come from the
  // palette only, so read them again once the theme exists. The previous
  // instance is kept for the views that still use it.
  static QmlTheme *instance = nullptr;
  if (!instance || (instance->mProvisional && Application::theme()))
    instance = new QmlTheme;
  return instance;
}

QmlTheme::QmlTheme() : mProvisional(!Application::theme()) {
  QPalette palette;
  QColor window = palette.color(QPalette::Window);
  QColor base = palette.color(QPalette::Base);
  QColor text = palette.color(QPalette::WindowText);
  QColor highlight = palette.color(QPalette::Highlight);
  mDark = (text.lightnessF() > window.lightnessF());

  // Derive every color from the widget palette so that themes without a
  // theme['ui'] section still get a consistent QML interface.
  mColors = {
      {"base", base},
      {"alternate", palette.color(QPalette::AlternateBase)},
      {"panel", window},
      {"toolbar", window},
      {"sidebar", base},
      {"field", base},
      {"border", mix(window, text, 0.15)},
      {"text", text},
      {"text_muted", mix(text, window, 0.4)},
      // Many themes don't define disabled colors, so blend instead.
      {"text_disabled", mix(text, window, 0.65)},
      {"hover", mix(window, text, 0.08)},
      {"pressed", mix(window, text, 0.16)},
      {"selected", highlight},
      {"selected_text", palette.color(QPalette::HighlightedText)},
      {"accent", highlight},
      {"accent_text", palette.color(QPalette::HighlightedText)},
      {"badge", highlight},
      {"badge_text", palette.color(QPalette::HighlightedText)},
      {"ahead", highlight},
      {"behind", highlight},
      {"star", QColor("#FFCE6D")},
      {"added", QColor("#3FB950")},
      {"modified", QColor("#D29922")},
      {"deleted", QColor("#F85149")},
      {"diff_addition", QColor(mDark ? "#1C3A2A" : "#E3F6E8")},
      {"diff_deletion", QColor(mDark ? "#45252A" : "#FCE8EA")},
      {"diff_ours", QColor(mDark ? "#1D2E4A" : "#E0F0FF")},
      {"diff_theirs", QColor(mDark ? "#3B2248" : "#F5E6FF")},
      {"tooltip", palette.color(QPalette::ToolTipBase)},
      {"tooltip_text", palette.color(QPalette::ToolTipText)},
  };

  if (Theme *theme = Application::theme()) {
    QColor notification = theme->badge(Theme::BadgeRole::Background,
                                       Theme::BadgeState::Notification);
    if (notification.isValid()) {
      mColors.insert("badge", notification);
      mColors.insert("ahead", notification);
      mColors.insert("behind", notification);
      mColors.insert("badge_text",
                     theme->badge(Theme::BadgeRole::Foreground,
                                  Theme::BadgeState::Notification));
    }

    QColor plus = theme->diff(Theme::Diff::Plus);
    if (plus.isValid())
      mColors.insert("added", plus);
    QColor minus = theme->diff(Theme::Diff::Minus);
    if (minus.isValid())
      mColors.insert("deleted", minus);

    auto setDiff = [this, theme](const char *key, Theme::Diff role) {
      QColor color = theme->diff(role);
      if (color.isValid())
        mColors.insert(key, color);
    };
    setDiff("diff_addition", Theme::Diff::Addition);
    setDiff("diff_deletion", Theme::Diff::Deletion);
    setDiff("diff_ours", Theme::Diff::Ours);
    setDiff("diff_theirs", Theme::Diff::Theirs);

    QColor star = theme->star();
    if (star.isValid())
      mColors.insert("star", star);

    // Explicit theme['ui'] entries win.
    QVariantMap ui = theme->ui();
    for (auto it = ui.cbegin(); it != ui.cend(); ++it) {
      QColor color(it.value().toString());
      if (color.isValid())
        mColors.insert(it.key(), color);
    }

    if (ui.contains("dark"))
      mDark = ui.value("dark").toBool();
  }

  mMonoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
  updateCodeFont();
  connect(Settings::instance(), &Settings::settingsChanged, this,
          &QmlTheme::updateCodeFont);
}

void QmlTheme::updateCodeFont() {
  Settings *settings = Settings::instance();
  QString family = settings->value(Setting::Id::FontFamily).toString();
  int size = settings->value(Setting::Id::FontSize).toInt();
  if (family.isEmpty())
    family = mMonoFont;
  if (size <= 0)
    size = 10;

  if (family == mCodeFont && size == mCodeFontSize)
    return;

  mCodeFont = family;
  mCodeFontSize = size;
  emit codeFontChanged();
}

QColor QmlTheme::color(const QString &key) const {
  return mColors.value(key).value<QColor>();
}
