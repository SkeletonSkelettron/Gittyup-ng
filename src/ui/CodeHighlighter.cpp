//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "CodeHighlighter.h"
#include <QTextDocument>

namespace {

const int kRestyleDelay = 150;

const QColor kFindColor = QColor(242, 201, 76, 90);
const QColor kCurrentColor = QColor(242, 153, 74, 190);

// The number of bytes of the UTF-8 encoding of the character at 'index',
// and the number of characters that it takes.
int utf8Length(const QString &text, int index, int &chars) {
  char16_t ch = text.at(index).unicode();
  chars = 1;
  if (ch < 0x80)
    return 1;
  if (ch < 0x800)
    return 2;
  if (QChar::isHighSurrogate(ch) && index + 1 < text.size()) {
    chars = 2;
    return 4;
  }
  return 3;
}

int styleAt(const QByteArray &styles, int pos) {
  return pos < styles.size() ? static_cast<uchar>(styles.at(pos)) : 0;
}

} // namespace

CodeHighlighter::CodeHighlighter(const QString &path, QTextDocument *document)
    : QSyntaxHighlighter(document), mPath(path) {
  mTimer.setSingleShot(true);
  mTimer.setInterval(kRestyleDelay);
  connect(&mTimer, &QTimer::timeout, this, &CodeHighlighter::restyle);
  // Highlighting changes the formats of the document but not its text.
  connect(document, &QTextDocument::contentsChange, this,
          [this](int, int removed, int added) {
            if (removed || added)
              mTimer.start();
          });

  restyle();
}

void CodeHighlighter::setPath(const QString &path) {
  if (path == mPath)
    return;

  mPath = path;
  mRuns.clear();
  restyle();
}

void CodeHighlighter::setFind(const QString &text, int block, int start) {
  if (text == mFindText && block == mFindBlock && start == mFindStart)
    return;

  // Only the blocks of the current match change when moving between them.
  int previous = mFindBlock;
  bool all = (text != mFindText);
  mFindText = text;
  mFindBlock = block;
  mFindStart = start;

  if (all) {
    rehighlight();
    return;
  }

  for (int number : {previous, block}) {
    QTextBlock changed = document()->findBlockByNumber(number);
    if (changed.isValid())
      rehighlightBlock(changed);
  }
}

void CodeHighlighter::highlightBlock(const QString &text) {
  int number = currentBlock().blockNumber();
  if (number < mRuns.size()) {
    for (const Run &run : mRuns.at(number)) {
      if (!run.style || run.start >= text.size())
        continue;

      SyntaxHighlighter::Format style = mLexer.format(run.style);
      QTextCharFormat format;
      if (style.color.isValid())
        format.setForeground(style.color);
      if (style.bold)
        format.setFontWeight(QFont::Bold);
      if (style.italic)
        format.setFontItalic(true);
      setFormat(run.start, run.length, format);
    }
  }

  // Mark the matches, keeping the style of the text.
  if (mFindText.isEmpty())
    return;

  int start = text.indexOf(mFindText, 0, Qt::CaseInsensitive);
  while (start >= 0) {
    bool current = (number == mFindBlock && start == mFindStart);
    for (int i = start; i < start + mFindText.length(); ++i) {
      QTextCharFormat format = this->format(i);
      format.setBackground(current ? kCurrentColor : kFindColor);
      setFormat(i, 1, format);
    }

    start = text.indexOf(mFindText, start + mFindText.length(),
                         Qt::CaseInsensitive);
  }
}

void CodeHighlighter::restyle() {
  QTextDocument *document = this->document();
  if (!document)
    return;

  // Style the whole text with the lexer.
  QByteArray text;
  QList<int> offsets;
  for (QTextBlock block = document->begin(); block.isValid();
       block = block.next()) {
    offsets.append(text.size());
    text += block.text().toUtf8();
    text += '\n';
  }

  QByteArray styles = mLexer.style(mPath, text);

  // Find the runs of characters of each block with the same style.
  QList<QList<Run>> runs;
  int number = 0;
  for (QTextBlock block = document->begin(); block.isValid();
       block = block.next(), ++number) {
    QString line = block.text();
    QList<Run> blockRuns;
    int byte = offsets.at(number);
    int index = 0;
    while (index < line.size()) {
      int style = styleAt(styles, byte);
      int start = index;
      while (index < line.size() && styleAt(styles, byte) == style) {
        int chars = 1;
        byte += utf8Length(line, index, chars);
        index += chars;
      }

      blockRuns.append({start, index - start, style});
    }

    runs.append(blockRuns);
  }

  // Highlight the blocks with other styles.
  bool all = (mRuns.size() != runs.size());
  QList<QList<Run>> previous = mRuns;
  mRuns = runs;
  if (all) {
    rehighlight();
    return;
  }

  number = 0;
  for (QTextBlock block = document->begin(); block.isValid();
       block = block.next(), ++number) {
    if (previous.at(number) != runs.at(number))
      rehighlightBlock(block);
  }
}
