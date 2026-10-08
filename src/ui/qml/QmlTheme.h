//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef QMLTHEME_H
#define QMLTHEME_H

#include <QColor>
#include <QObject>
#include <QVariantMap>

// Exposes the current theme's colors to QML as the "Theme" singleton of the
// "Gittyup" module. Themes are only applied at startup, so the colors are
// constant.
class QmlTheme : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool dark READ dark CONSTANT)

  Q_PROPERTY(QColor base READ base CONSTANT)
  Q_PROPERTY(QColor alternate READ alternate CONSTANT)
  Q_PROPERTY(QColor panel READ panel CONSTANT)
  Q_PROPERTY(QColor toolbar READ toolbar CONSTANT)
  Q_PROPERTY(QColor sidebar READ sidebar CONSTANT)
  Q_PROPERTY(QColor field READ field CONSTANT)
  Q_PROPERTY(QColor border READ border CONSTANT)

  Q_PROPERTY(QColor text READ text CONSTANT)
  Q_PROPERTY(QColor textMuted READ textMuted CONSTANT)
  Q_PROPERTY(QColor textDisabled READ textDisabled CONSTANT)

  Q_PROPERTY(QColor hover READ hover CONSTANT)
  Q_PROPERTY(QColor pressed READ pressed CONSTANT)
  Q_PROPERTY(QColor selected READ selected CONSTANT)
  Q_PROPERTY(QColor selectedText READ selectedText CONSTANT)

  Q_PROPERTY(QColor accent READ accent CONSTANT)
  Q_PROPERTY(QColor accentText READ accentText CONSTANT)

  Q_PROPERTY(QColor badge READ badge CONSTANT)
  Q_PROPERTY(QColor badgeText READ badgeText CONSTANT)
  Q_PROPERTY(QColor ahead READ ahead CONSTANT)
  Q_PROPERTY(QColor behind READ behind CONSTANT)
  Q_PROPERTY(QColor star READ star CONSTANT)
  Q_PROPERTY(QColor added READ added CONSTANT)
  Q_PROPERTY(QColor modified READ modified CONSTANT)
  Q_PROPERTY(QColor deleted READ deleted CONSTANT)

  Q_PROPERTY(QColor diffAddition READ diffAddition CONSTANT)
  Q_PROPERTY(QColor diffDeletion READ diffDeletion CONSTANT)
  Q_PROPERTY(QColor diffOurs READ diffOurs CONSTANT)
  Q_PROPERTY(QColor diffTheirs READ diffTheirs CONSTANT)
  Q_PROPERTY(QString monoFont READ monoFont CONSTANT)
  // The font of code in diffs and files, from the editor settings.
  Q_PROPERTY(QString codeFont READ codeFont NOTIFY codeFontChanged)
  Q_PROPERTY(int codeFontSize READ codeFontSize NOTIFY codeFontChanged)

  Q_PROPERTY(QColor tooltip READ tooltip CONSTANT)
  Q_PROPERTY(QColor tooltipText READ tooltipText CONSTANT)

public:
  static QmlTheme *instance();

  bool dark() const { return mDark; }

  QColor base() const { return color("base"); }
  QColor alternate() const { return color("alternate"); }
  QColor panel() const { return color("panel"); }
  QColor toolbar() const { return color("toolbar"); }
  QColor sidebar() const { return color("sidebar"); }
  QColor field() const { return color("field"); }
  QColor border() const { return color("border"); }

  QColor text() const { return color("text"); }
  QColor textMuted() const { return color("text_muted"); }
  QColor textDisabled() const { return color("text_disabled"); }

  QColor hover() const { return color("hover"); }
  QColor pressed() const { return color("pressed"); }
  QColor selected() const { return color("selected"); }
  QColor selectedText() const { return color("selected_text"); }

  QColor accent() const { return color("accent"); }
  QColor accentText() const { return color("accent_text"); }

  QColor badge() const { return color("badge"); }
  QColor badgeText() const { return color("badge_text"); }
  QColor ahead() const { return color("ahead"); }
  QColor behind() const { return color("behind"); }
  QColor star() const { return color("star"); }
  QColor added() const { return color("added"); }
  QColor modified() const { return color("modified"); }
  QColor deleted() const { return color("deleted"); }

  QColor diffAddition() const { return color("diff_addition"); }
  QColor diffDeletion() const { return color("diff_deletion"); }
  QColor diffOurs() const { return color("diff_ours"); }
  QColor diffTheirs() const { return color("diff_theirs"); }
  QString monoFont() const { return mMonoFont; }
  QString codeFont() const { return mCodeFont; }
  int codeFontSize() const { return mCodeFontSize; }

  QColor tooltip() const { return color("tooltip"); }
  QColor tooltipText() const { return color("tooltip_text"); }

  // Look up a color by its theme['ui'] key.
  QColor color(const QString &key) const;

signals:
  void codeFontChanged();

private:
  QmlTheme();

  void updateCodeFont();

  bool mDark = false;
  bool mProvisional = false;
  QVariantMap mColors;
  QString mMonoFont;
  QString mCodeFont;
  int mCodeFontSize = 10;
};

#endif
