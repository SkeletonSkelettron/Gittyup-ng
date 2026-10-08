//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "SyntaxHighlighter.h"
#include "conf/Settings.h"
#include "editor/TextEditor.h"
#include "qml/QmlTheme.h"

namespace {

QColor fromScintilla(sptr_t colour) {
  return QColor(colour & 0xff, (colour >> 8) & 0xff, (colour >> 16) & 0xff);
}

// Append 'text' to 'html', escaped, with tabs expanded and spaces kept.
// Visible whitespace is drawn with 'whitespace', a span for a character.
void appendText(QString &html, const QString &text, int &column,
                int tabWidth, const QString &whitespace) {
  for (QChar ch : text) {
    if (ch == '\t') {
      int spaces = tabWidth - (column % tabWidth);
      for (int i = 0; i < spaces; ++i)
        html += (i == 0 && !whitespace.isEmpty())
                    ? whitespace.arg(QChar(0x2192))
                    : QString("&nbsp;");
      column += spaces;
      continue;
    }

    switch (ch.unicode()) {
      case ' ':
        html += whitespace.isEmpty() ? QString("&nbsp;")
                                     : whitespace.arg(QChar(0x00b7));
        break;
      case '<':
        html += "&lt;";
        break;
      case '>':
        html += "&gt;";
        break;
      case '&':
        html += "&amp;";
        break;
      default:
        html += ch;
        break;
    }

    ++column;
  }
}

} // namespace

// The editor is never shown. It only runs the lexer.
SyntaxHighlighter::SyntaxHighlighter() : mEditor(new TextEditor) {}

SyntaxHighlighter::~SyntaxHighlighter() { delete mEditor; }

QByteArray SyntaxHighlighter::style(const QString &path,
                                    const QByteArray &text) {
  mFormats.clear();
  mEditor->setLexer(path);
  mEditor->setText(text.constData());
  mEditor->colourise(0, -1);

  int length = qMin(static_cast<int>(mEditor->length()),
                    static_cast<int>(text.size()));
  QByteArray styles(text.size(), 0);
  for (int i = 0; i < length; ++i)
    styles[i] = static_cast<char>(mEditor->styleAt(i));

  // Free the memory of the text.
  mEditor->setText("");
  return styles;
}

SyntaxHighlighter::Format SyntaxHighlighter::format(int style) const {
  auto it = mFormats.constFind(style);
  if (it != mFormats.constEnd())
    return it.value();

  Format format;
  if (style != 0) {
    sptr_t fore = mEditor->styleFore(style);
    if (fore != mEditor->styleFore(STYLE_DEFAULT))
      format.color = fromScintilla(fore);
    format.bold = mEditor->styleBold(style);
    format.italic = mEditor->styleItalic(style);
  }

  mFormats.insert(style, format);
  return format;
}

QString SyntaxHighlighter::html(
    const QByteArray &line, const QByteArray &styles,
    const std::function<QString(const QByteArray &)> &decode,
    const QVector<bool> &marked, const QColor &markColor) const {
  auto styleAt = [&styles](int pos) {
    return pos < styles.size() ? static_cast<uchar>(styles.at(pos)) : 0;
  };
  auto markedAt = [&marked](int pos) {
    return pos < marked.size() && marked.at(pos);
  };

  int tabWidth = SyntaxHighlighter::tabWidth();
  QString whitespace;
  if (Settings::instance()->value(Setting::Id::ShowWhitespaceInEditor).toBool())
    whitespace = QString("<span style='color:%1'>%2</span>")
                     .arg(QmlTheme::instance()->textDisabled().name(), "%1");

  // Emit runs of bytes with the same style and mark.
  QString html;
  int column = 0;
  int pos = 0;
  while (pos < line.size()) {
    bool mark = markedAt(pos);
    int style = styleAt(pos);
    int end = pos + 1;
    while (end < line.size() && markedAt(end) == mark &&
           styleAt(end) == style)
      ++end;

    QString css;
    if (mark)
      css += QString("background-color:%1;").arg(markColor.name());
    if (style) {
      Format format = this->format(style);
      if (format.color.isValid())
        css += QString("color:%1;").arg(format.color.name());
      if (format.bold)
        css += "font-weight:bold;";
      if (format.italic)
        css += "font-style:italic;";
    }

    if (!css.isEmpty())
      html += QString("<span style='%1'>").arg(css);
    appendText(html, decode(line.mid(pos, end - pos)), column, tabWidth,
               whitespace);
    if (!css.isEmpty())
      html += "</span>";

    pos = end;
  }

  return html;
}

int SyntaxHighlighter::tabWidth() {
  int width = Settings::instance()->value(Setting::Id::TabWidth).toInt();
  return width > 0 ? width : 4;
}

QString SyntaxHighlighter::expandTabs(const QString &text) {
  if (!text.contains('\t'))
    return text;

  int tabWidth = SyntaxHighlighter::tabWidth();
  QString result;
  for (QChar ch : text) {
    if (ch == '\t') {
      result += QString(tabWidth - (result.size() % tabWidth), ' ');
      continue;
    }

    result += ch;
  }

  return result;
}
