//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef DIFFLINES_H
#define DIFFLINES_H

#include <QByteArray>
#include <QList>
#include <QPair>

namespace git {
class Patch;
}

// The lines of a diff hunk and the bookkeeping needed to stage, unstage and
// discard single lines. This is independent of any view.
namespace DiffLines {

struct Line {
  char origin = ' ';
  int oldLine = -1;
  int newLine = -1;

  // The raw content of the line, including the line ending.
  QByteArray content;

  // The line isn't followed by a newline at the end of the file.
  bool noNewline = false;

  bool staged = false;

  // Index of the corresponding deletion/addition in the hunk, if the
  // change looks like a modification of a single line.
  int matchingLine = -1;

  bool isChange() const { return origin == '+' || origin == '-'; }
};

// Changed character ranges within a line: (start, length) in bytes.
using Ranges = QList<QPair<int, int>>;

// Get the lines of hunk 'hunk' of 'patch'. 'staged' is the diff between
// HEAD and the index for the same file; it determines which lines are
// staged. Lines marking a missing newline at the end of the file are not
// returned, the preceding line has noNewline set instead.
QList<Line> lines(const git::Patch &patch, int hunk, const git::Patch &staged);

// The content of a hunk in the index: context, staged additions and
// unstaged deletions.
QByteArray stagedContent(const QList<Line> &lines);

// The content of a hunk in the working directory after discarding the
// changes of the lines for which 'discard' is true.
QByteArray discardContent(const QList<Line> &lines,
                          const QList<bool> &discard);

// Find the changed parts of a modified line: the ranges of 'oldLine' that
// were removed and of 'newLine' that were added.
QPair<Ranges, Ranges> changedRanges(const QByteArray &oldLine,
                                    const QByteArray &newLine);

} // namespace DiffLines

#endif
