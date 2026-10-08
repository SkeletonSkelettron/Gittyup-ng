//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "FindController.h"
#include <QVariantMap>

QVariantList FindTarget::findMatches(const QString &text, int row,
                                     int currentRow, int currentStart) const {
  QVariantList matches;
  if (text.isEmpty())
    return matches;

  QString line = findRowText(row);
  int start = line.indexOf(text, 0, Qt::CaseInsensitive);
  while (start >= 0) {
    bool current = (row == currentRow && start == currentStart);
    matches.append(QVariantMap{
        {"start", start}, {"length", text.length()}, {"current", current}});
    start = line.indexOf(text, start + text.length(), Qt::CaseInsensitive);
  }

  return matches;
}

QString FindController::sText;

FindController::FindController(const TargetFunc &target, QObject *parent)
    : QObject(parent), mTarget(target) {}

QString FindController::searchText() const { return FindController::text(); }

QString FindController::hitsText() const {
  if (searchText().isEmpty())
    return QString();

  if (mMatches.isEmpty())
    return tr("Not found");

  return tr("%1 of %2").arg(mCurrent + 1).arg(mMatches.size());
}

void FindController::show() {
  if (!mVisible) {
    mVisible = true;
    emit visibleChanged();
  }

  emit searchTextChanged();
  update();
  emit focusRequested();
}

void FindController::hide() {
  if (!mVisible)
    return;

  mVisible = false;
  emit visibleChanged();

  // Remove the matches from the target.
  mMatches.clear();
  mCurrent = -1;
  if (FindTarget *target = mTarget())
    target->setFindState(QString(), -1, -1);
  emit hitsChanged();
}

void FindController::search(const QString &text) {
  if (text == FindController::text())
    return;

  FindController::setText(text);
  emit searchTextChanged();
  update();
}

void FindController::next() {
  // Show the matches of the text used last, like the text selected for
  // finding.
  if (!mVisible) {
    show();
    return;
  }

  if (!mMatches.isEmpty())
    setCurrent((mCurrent + 1) % mMatches.size());
}

void FindController::previous() {
  if (!mVisible) {
    show();
    return;
  }

  if (!mMatches.isEmpty())
    setCurrent((mCurrent - 1 + mMatches.size()) % mMatches.size());
}

void FindController::refresh() {
  if (mVisible)
    update();
}

void FindController::update() {
  // Keep the position of the current match when the text changes.
  Match previous = mCurrent >= 0 ? mMatches.at(mCurrent) : Match{0, 0};

  FindTarget *target = mTarget();
  if (mLastTarget && mLastTarget != target)
    mLastTarget->setFindState(QString(), -1, -1);
  mLastTarget = target;

  mMatches.clear();
  mCurrent = -1;
  QString text = searchText();
  if (target && !text.isEmpty()) {
    int count = target->findRowCount();
    for (int row = 0; row < count; ++row) {
      QString line = target->findRowText(row);
      int start = line.indexOf(text, 0, Qt::CaseInsensitive);
      while (start >= 0) {
        mMatches.append({row, start});
        start = line.indexOf(text, start + text.length(), Qt::CaseInsensitive);
      }
    }
  }

  // Start at the first match after the previous one.
  int current = -1;
  for (int i = 0; i < mMatches.size(); ++i) {
    const Match &match = mMatches.at(i);
    if (match.row > previous.row ||
        (match.row == previous.row && match.start >= previous.start)) {
      current = i;
      break;
    }
  }

  if (current < 0 && !mMatches.isEmpty())
    current = 0;

  if (current >= 0) {
    setCurrent(current);
    return;
  }

  if (target)
    target->setFindState(text, -1, -1);
  emit hitsChanged();
}

void FindController::setCurrent(int index) {
  mCurrent = index;
  const Match &match = mMatches.at(index);
  if (FindTarget *target = mTarget())
    target->setFindState(searchText(), match.row, match.start);

  emit hitsChanged();
  emit currentChanged(match.row, match.start, searchText().length());
}
