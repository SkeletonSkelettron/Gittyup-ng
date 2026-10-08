//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef CODEHIGHLIGHTER_H
#define CODEHIGHLIGHTER_H

#include "SyntaxHighlighter.h"
#include <QList>
#include <QSyntaxHighlighter>
#include <QTimer>

// Highlights the code of a text document with the lexers of the text
// editor, and the matches of the find bar. The document is styled again a
// moment after it changes.
class CodeHighlighter : public QSyntaxHighlighter {
  Q_OBJECT

public:
  CodeHighlighter(const QString &path, QTextDocument *document);

  // The path selects the lexer.
  void setPath(const QString &path);

  // Mark the matches of 'text', with the one at 'start' of 'block' as the
  // current one.
  void setFind(const QString &text, int block, int start);

  // Style the whole document now, for example after loading it.
  void restyle();

protected:
  void highlightBlock(const QString &text) override;

private:
  // A range of characters of a block with the same style.
  struct Run {
    int start;
    int length;
    int style;

    bool operator==(const Run &rhs) const {
      return start == rhs.start && length == rhs.length && style == rhs.style;
    }
  };

  SyntaxHighlighter mLexer;
  QString mPath;
  QList<QList<Run>> mRuns;
  QTimer mTimer;

  QString mFindText;
  int mFindBlock = -1;
  int mFindStart = -1;
};

#endif
