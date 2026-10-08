//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "DiffLines.h"
#include "git/Patch.h"
#include "git2/diff.h"

namespace DiffLines {

namespace {

bool isEofNewline(char origin) {
  return origin == GIT_DIFF_LINE_CONTEXT_EOFNL ||
         origin == GIT_DIFF_LINE_ADD_EOFNL || origin == GIT_DIFF_LINE_DEL_EOFNL;
}

// Split a line into words, runs of whitespace and single other characters.
QList<QByteArray> tokens(const QByteArray &line) {
  auto isWord = [](char ch) {
    return QChar::isLetterOrNumber(static_cast<uchar>(ch)) || ch == '_' ||
           static_cast<uchar>(ch) >= 0x80;
  };
  auto isSpace = [](char ch) { return ch == ' ' || ch == '\t'; };

  QList<QByteArray> result;
  int pos = 0;
  int length = line.length();
  while (pos < length) {
    int end = pos + 1;
    char ch = line.at(pos);
    if (isWord(ch)) {
      while (end < length && isWord(line.at(end)))
        ++end;
    } else if (isSpace(ch)) {
      while (end < length && isSpace(line.at(end)))
        ++end;
    }

    result.append(line.mid(pos, end - pos));
    pos = end;
  }

  return result;
}

QByteArray trimmed(const QByteArray &line) {
  QByteArray result = line;
  while (result.endsWith('\n') || result.endsWith('\r'))
    result.chop(1);
  return result;
}

} // namespace

QList<Line> lines(const git::Patch &patch, int hunk,
                  const git::Patch &staged) {
  QList<Line> result;
  int patchCount = patch.lineCount(hunk);
  bool conflicted = patch.isConflicted();

  // This follows the matching of the lines of the complete patch (HEAD to
  // workdir) against the staged patch (HEAD to index) of HunkWidget.
  //
  // New file line number change for different line origins
  // | diff HEAD         | diff –cached |   |
  // |-------------------|--------------|---|
  // | no change         | +            | + |
  // | unstaged addition | +            | / |
  // | staged addition   | +            | + |
  // | unstaged deletion | /            | + |
  // | staged deletion   | /            | / |

  int current_staged_index = -1;
  int current_staged_line_idx = 0;

  // Find the first staged hunk which is within this hunk.
  if (!conflicted) {
    const git_diff_hunk *header = patch.header_struct(hunk);
    for (int i = 0; header && i < staged.count(); i++) {
      if (header->old_start > staged.header_struct(i)->old_start +
                                  staged.header_struct(i)->old_lines)
        continue;

      current_staged_index = i;
      break;
    }
  }

  int additions = 0, deletions = 0;
  bool first_staged_patch_match = false;
  for (int lidx = 0; lidx < patchCount; ++lidx) {
    const char lineOrigin = patch.lineOrigin(hunk, lidx);

    if (isEofNewline(lineOrigin)) {
      if (!result.isEmpty())
        result.last().noNewline = true;
    } else {
      Line line;
      line.origin = lineOrigin;
      line.oldLine = patch.lineNumber(hunk, lidx, git::Diff::OldFile);
      line.newLine = patch.lineNumber(hunk, lidx, git::Diff::NewFile);
      line.content = patch.lineContent(hunk, lidx);
      result.append(line);
    }

    // Find matching lines if the first line of the staged patch is somewhere
    // in the middle of the complete patch. Switch to the next staged patch if
    // the line number is higher than the current staged patch contains.
    if (!conflicted && current_staged_index >= 0) {
      const git_diff_hunk *staged_header_struct_next =
          staged.header_struct(current_staged_index + 1);
      if (!first_staged_patch_match) {
        if ((lineOrigin == GIT_DIFF_LINE_CONTEXT ||
             lineOrigin == GIT_DIFF_LINE_DELETION) &&
            patch.lineNumber(hunk, lidx, git::Diff::OldFile) ==
                staged.lineNumber(current_staged_index, 0,
                                  git::Diff::OldFile)) {
          first_staged_patch_match = true;
          current_staged_line_idx = 0;
        } else if (lineOrigin == GIT_DIFF_LINE_ADDITION &&
                   // If the staged line is not an addition, it cannot be
                   // matched.
                   staged.lineOrigin(current_staged_index, 0) ==
                       GIT_DIFF_LINE_ADDITION &&
                   patch.lineNumber(hunk, lidx, git::Diff::NewFile) ==
                       staged.lineNumber(current_staged_index, 0,
                                         git::Diff::NewFile))
          first_staged_patch_match = true;
        current_staged_line_idx = 0;
      } else if (staged_header_struct_next &&
                 patch.lineNumber(hunk, lidx, git::Diff::OldFile) ==
                     staged_header_struct_next->old_start) {
        // Align staged patch with total patch.
        current_staged_index++;
        current_staged_line_idx = 0;
      }
    }

    bool isStaged = false;
    switch (lineOrigin) {
      case GIT_DIFF_LINE_CONTEXT:
        additions = 0;
        deletions = 0;
        current_staged_line_idx++;
        break;

      case GIT_DIFF_LINE_ADDITION: {
        additions++;

        // The heuristic is that matching blocks have the same number of
        // additions as deletions.
        if (lidx + 1 >= patchCount ||
            patch.lineOrigin(hunk, lidx + 1) != GIT_DIFF_LINE_ADDITION) {
          if (additions == deletions) {
            int last = result.size() - 1;
            for (int i = 0; i < additions; ++i) {
              int current = last - i;
              int match = current - additions;
              if (match >= 0) {
                result[current].matchingLine = match;
                result[match].matchingLine = current;
              }
            }
          }

          additions = 0;
          deletions = 0;
        }

        // Check if staged.
        if (!conflicted && staged.count() > 0 && current_staged_index >= 0 &&
            current_staged_line_idx < staged.lineCount(current_staged_index)) {
          char origin =
              staged.lineOrigin(current_staged_index, current_staged_line_idx);
          if (origin == '+' &&
              staged.lineContent(current_staged_index,
                                 current_staged_line_idx) ==
                  patch.lineContent(hunk, lidx)) {
            current_staged_line_idx++;
            isStaged = true;
          }
        }
        break;
      }

      case GIT_DIFF_LINE_DELETION: {
        deletions++;

        // Check if staged.
        if (!conflicted && staged.count() > 0 && current_staged_index >= 0 &&
            current_staged_line_idx < staged.lineCount(current_staged_index)) {
          char origin =
              staged.lineOrigin(current_staged_index, current_staged_line_idx);
          int stagedOld = staged.lineNumber(
              current_staged_index, current_staged_line_idx, git::Diff::OldFile);
          int patchOld = patch.lineNumber(hunk, lidx, git::Diff::OldFile);
          if (origin == '-' && stagedOld == patchOld)
            isStaged = true;
        }

        // Must be incremented in the staged and in the unstaged case.
        current_staged_line_idx++;
        break;
      }

      case GIT_DIFF_LINE_CONTEXT_EOFNL:
      case GIT_DIFF_LINE_ADD_EOFNL:
      case GIT_DIFF_LINE_DEL_EOFNL:
        current_staged_line_idx++;
        continue;
    }

    result.last().staged = isStaged;
  }

  return result;
}

QByteArray stagedContent(const QList<Line> &lines) {
  QByteArray result;
  for (const Line &line : lines) {
    bool include = (line.origin == '+')   ? line.staged
                   : (line.origin == '-') ? !line.staged
                                          : true;
    if (include)
      result.append(line.content);
  }

  return result;
}

QByteArray discardContent(const QList<Line> &lines,
                          const QList<bool> &discard) {
  QByteArray result;
  for (int i = 0; i < lines.size(); ++i) {
    const Line &line = lines.at(i);
    bool discarded = discard.value(i);

    // A discarded addition disappears, a discarded deletion comes back.
    bool include = (line.origin == '+')   ? !discarded
                   : (line.origin == '-') ? discarded
                                          : true;
    if (include)
      result.append(line.content);
  }

  return result;
}

QPair<Ranges, Ranges> changedRanges(const QByteArray &oldLine,
                                    const QByteArray &newLine) {
  QList<QByteArray> oldTokens = tokens(trimmed(oldLine));
  QList<QByteArray> newTokens = tokens(trimmed(newLine));

  // Offsets of the tokens, with a sentinel at the end.
  auto offsets = [](const QList<QByteArray> &tokens) {
    QList<int> result;
    int offset = 0;
    for (const QByteArray &token : tokens) {
      result.append(offset);
      offset += token.length();
    }
    result.append(offset);
    return result;
  };

  QList<int> oldOffsets = offsets(oldTokens);
  QList<int> newOffsets = offsets(newTokens);

  // Diff the tokens line by line.
  QByteArray oldBuffer = QByteArrayList(oldTokens).join('\n') + '\n';
  QByteArray newBuffer = QByteArrayList(newTokens).join('\n') + '\n';
  git::Patch patch = git::Patch::fromBuffers(oldBuffer, newBuffer);

  Ranges removed, added;
  for (int h = 0; h < patch.count(); ++h) {
    for (int l = 0; l < patch.lineCount(h); ++l) {
      char origin = patch.lineOrigin(h, l);
      if (origin == GIT_DIFF_LINE_DELETION) {
        int token = patch.lineNumber(h, l, git::Diff::OldFile) - 1;
        if (token >= 0 && token < oldTokens.size())
          removed.append({oldOffsets.at(token), oldTokens.at(token).length()});
      } else if (origin == GIT_DIFF_LINE_ADDITION) {
        int token = patch.lineNumber(h, l, git::Diff::NewFile) - 1;
        if (token >= 0 && token < newTokens.size())
          added.append({newOffsets.at(token), newTokens.at(token).length()});
      }
    }
  }

  // Merge adjacent ranges.
  auto merge = [](const Ranges &ranges) {
    Ranges result;
    for (const auto &range : ranges) {
      if (!result.isEmpty() &&
          result.last().first + result.last().second == range.first) {
        result.last().second += range.second;
      } else {
        result.append(range);
      }
    }
    return result;
  };

  return {merge(removed), merge(added)};
}

} // namespace DiffLines
