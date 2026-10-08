//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "CommitMessage.h"
#include "CommitTemplates.h"
#include <QCoreApplication>
#include <QRegularExpression>

namespace CommitMessage {

namespace {

QString tr(const char *text) {
  return QCoreApplication::translate("CommitMessage", text);
}

} // namespace

QString fileList(const QStringList &list, int maxFiles) {
  const int numberFiles = list.size();
  if (numberFiles == 0 || maxFiles == 0)
    return QString();

  if (numberFiles == 1) {
    return list.first();
  } else if (numberFiles == 2 && maxFiles >= 2) {
    return tr("%1 and %2").arg(list.first(), list.last());
  } else if (numberFiles == 3 && maxFiles >= 3) {
    return tr("%1, %2, and %3").arg(list.at(0), list.at(1), list.at(2));
  }

  // numberFiles > 3 || maxFiles < numberFiles
  QString msg;
  const int s = qMin(numberFiles, maxFiles) - 1;
  for (int i = 0; i < s; i++)
    msg += list.at(i) + QStringLiteral(", ");

  if (numberFiles > s + 1) {
    msg += list.at(s) + QStringLiteral(", ");
    const int remainingFiles = numberFiles - s - 1;
    msg += QStringLiteral("and %1 more file").arg(remainingFiles);
    if (remainingFiles > 1)
      msg += QStringLiteral("s");
  } else {
    msg += QStringLiteral("and %1").arg(list.at(s));
  }

  return msg;
}

Result applyTemplate(const QString &text, const QStringList &files) {
  QString templ = text;

  QString pattern = CommitTemplates::filesPosition;
  pattern.replace("{", "\\{");
  pattern.replace("}", "\\}");
  pattern.replace("$", "\\$");
  QRegularExpressionMatch match = QRegularExpression(pattern).match(templ);

  int start = -1;
  int offset = 0;
  if (match.hasMatch()) {
    start = match.capturedStart(0);
    int origLength = match.capturedLength(0);
    const QString matchComplete = match.captured(0);
    bool ok;
    const int number = match.captured(1).toInt(&ok);
    if (ok) {
      const QString filesStr = fileList(files, number);
      templ.replace(matchComplete, filesStr);
      offset = filesStr.length() - origLength;
    }
  }

  int index = text.indexOf(CommitTemplates::cursorPositionString);
  if (index < 0) {
    index = templ.length();
  } else if (start > 0 && index > start) {
    // The list of files has a different length than its placeholder.
    index += offset;
  }

  templ.replace(CommitTemplates::cursorPositionString, "");
  return {templ, qMin(index, static_cast<int>(templ.length()))};
}

} // namespace CommitMessage
