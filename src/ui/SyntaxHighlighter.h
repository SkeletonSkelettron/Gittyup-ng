//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QString>
#include <QVector>
#include <functional>

class TextEditor;

// Styles source code with the lexers and the theme of the text editor, so
// views that aren't text editors can highlight code like it.
class SyntaxHighlighter {
public:
  struct Format {
    // Invalid for the default text color.
    QColor color;
    bool bold = false;
    bool italic = false;
  };

  SyntaxHighlighter();
  ~SyntaxHighlighter();

  // Get the style of each byte of 'text', which is the content of 'path'.
  QByteArray style(const QString &path, const QByteArray &text);

  // The format of a style returned by the last call to style().
  Format format(int style) const;

  // The HTML of a line of text and its styles from the last call to
  // style(), with tabs expanded and spaces kept. Bytes that are 'marked'
  // get the background 'markColor'. 'decode' converts bytes to text.
  QString html(const QByteArray &line, const QByteArray &styles,
               const std::function<QString(const QByteArray &)> &decode,
               const QVector<bool> &marked = QVector<bool>(),
               const QColor &markColor = QColor()) const;

  // 'text' as html() shows it, with tabs expanded.
  static QString expandTabs(const QString &text);

  // The width of tabs from the editor settings.
  static int tabWidth();

  // The hidden editor, for example to run plugins on text.
  TextEditor *editor() const { return mEditor; }

private:
  TextEditor *mEditor;
  mutable QHash<int, Format> mFormats;
};

#endif
