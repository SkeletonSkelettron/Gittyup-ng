//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef COMMITMESSAGE_H
#define COMMITMESSAGE_H

#include <QString>
#include <QStringList>

// Helpers to generate commit messages.
namespace CommitMessage {

// "a", "a and b", "a, b, and c" or "a, b, and 3 more files".
QString fileList(const QStringList &files, int maxFiles);

struct Result {
  QString text;
  int cursorPosition;
};

// Expand a message template. ${files:N} is replaced by the list of files
// and %| marks the position of the cursor.
Result applyTemplate(const QString &text, const QStringList &files);

} // namespace CommitMessage

#endif
