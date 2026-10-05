//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "MergeModel.h"
#include "CodeHighlighter.h"
#include "RepoView.h"
#include "SyntaxHighlighter.h"
#include "dialogs/ConfirmDialog.h"
#include "git/Blob.h"
#include "git/Index.h"
#include <QDir>
#include <QFile>
#include <QQuickTextDocument>
#include <QSaveFile>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextStream>

namespace {

// The markers of a conflict start with seven characters.
bool isMarker(const QString &line, QChar ch) {
  if (line.size() < 7)
    return false;

  for (int i = 0; i < 7; ++i) {
    if (line.at(i) != ch)
      return false;
  }

  return line.size() == 7 || line.at(7) == ' ';
}

QString label(const QString &marker) { return marker.mid(8).trimmed(); }

} // namespace

MergeSideModel::MergeSideModel(MergeModel *merge, int side)
    : QAbstractListModel(merge), mMerge(merge), mSide(side) {}

MergeSideModel::~MergeSideModel() {}

int MergeSideModel::conflictRow(int conflict) const {
  for (int row = 0; row < mRows.size(); ++row) {
    if (mRows.at(row).kind == ConflictRow && mRows.at(row).conflict == conflict)
      return row;
  }

  return -1;
}

void MergeSideModel::reset() {
  beginResetModel();
  mRows.clear();
  mStyles.clear();

  // The whole file of this side, for the lexer.
  QString text;
  QList<int> textRows;
  int number = 0;
  for (const MergeModel::Segment &segment : mMerge->segments()) {
    if (segment.conflict < 0) {
      for (const QString &line : segment.common) {
        mRows.append({CommonRow, -1, -1, ++number, line});
        textRows.append(mRows.size() - 1);
        text += line + '\n';
      }
      continue;
    }

    const MergeModel::Conflict &conflict =
        mMerge->conflicts().at(segment.conflict);
    mRows.append({ConflictRow, segment.conflict, -1, 0, QString()});
    const QStringList &lines = conflict.lines[mSide];
    if (lines.isEmpty())
      mRows.append({EmptyRow, segment.conflict, -1, 0, QString()});
    for (int i = 0; i < lines.size(); ++i) {
      mRows.append({LineRow, segment.conflict, i, ++number, lines.at(i)});
      textRows.append(mRows.size() - 1);
      text += lines.at(i) + '\n';
    }
  }

  // Style the lines with the lexer of the file.
  if (!mRows.isEmpty()) {
    if (!mHighlighter)
      mHighlighter.reset(new SyntaxHighlighter);

    QByteArray bytes = text.toUtf8();
    QByteArray styles = mHighlighter->style(mMerge->path(), bytes);
    for (int i = 0; i < mRows.size(); ++i)
      mStyles.append(QByteArray());

    int offset = 0;
    for (int row : textRows) {
      int length = mRows.at(row).text.toUtf8().size();
      mStyles[row] = styles.mid(offset, length);
      offset += length + 1;
    }
  }

  endResetModel();
}

void MergeSideModel::updateChecks(int conflict) {
  for (int row = 0; row < mRows.size(); ++row) {
    if (mRows.at(row).conflict == conflict)
      emit dataChanged(index(row), index(row), {CheckedRole, CheckStateRole});
  }
}

int MergeSideModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : mRows.size();
}

QVariant MergeSideModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= mRows.size())
    return QVariant();

  const Row &row = mRows.at(index.row());
  switch (role) {
    case KindRole:
      return row.kind;
    case ConflictRole:
      return row.conflict;
    case LineRole:
      return row.line;
    case NumberRole:
      return row.number;
    case HtmlRole:
      if (!mHighlighter || row.kind == ConflictRow || row.kind == EmptyRow)
        return QString();
      return mHighlighter->html(
          row.text.toUtf8(), mStyles.value(index.row()),
          [](const QByteArray &bytes) { return QString::fromUtf8(bytes); });
    case CheckedRole:
      if (row.kind != LineRow)
        return false;
      return mMerge->conflicts().at(row.conflict).checked[mSide].value(row.line);
    case CheckStateRole: {
      if (row.conflict < 0)
        return 0;
      const QList<bool> &checked =
          mMerge->conflicts().at(row.conflict).checked[mSide];
      int count = checked.count(true);
      return count == 0 ? 0 : count == checked.size() ? 2 : 1;
    }
  }

  return QVariant();
}

QHash<int, QByteArray> MergeSideModel::roleNames() const {
  return {{KindRole, "kind"},       {ConflictRole, "conflict"},
          {LineRole, "line"},       {NumberRole, "number"},
          {HtmlRole, "html"},       {CheckedRole, "checked"},
          {CheckStateRole, "checkState"}};
}

MergeModel::MergeModel(RepoView *view, QObject *parent)
    : QObject(parent), mView(view) {
  mSides[0] = new MergeSideModel(this, 0);
  mSides[1] = new MergeSideModel(this, 1);

  // Follow edits of the output.
  mRegionsTimer.setSingleShot(true);
  mRegionsTimer.setInterval(0);
  connect(&mRegionsTimer, &QTimer::timeout, this,
          &MergeModel::regionsChanged);
}

MergeModel::~MergeModel() {}

void MergeModel::load(const QString &path) {
  git::Repository repo = mView->repo();
  QFile file(repo.workdir().filePath(path));
  QByteArray content = file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();

  // Keep the lines that were taken while the file is the same.
  if (path == mPath && content == mContent)
    return;

  mPath = path;
  mContent = content;
  mNotice.clear();
  mLabels[0] = tr("Ours");
  mLabels[1] = tr("Theirs");
  mSegments.clear();
  mConflicts.clear();
  mRegions.clear();

  mEncoding = repo.encoding(content);
  if (git::Blob::isBinary(content)) {
    mNotice = tr("Binary files can't be merged here.");
  } else if (!parse(repo.decode(content))) {
    mNotice = tr("This file has no conflict markers.");
    mSegments.clear();
    mConflicts.clear();
  }

  mSides[0]->reset();
  mSides[1]->reset();
  setOutput();

  emit loaded();
  emit selectionChanged();
  emit regionsChanged();
}

void MergeModel::clear() {
  if (mPath.isEmpty())
    return;

  mPath.clear();
  mContent.clear();
  mNotice.clear();
  mSegments.clear();
  mConflicts.clear();
  mRegions.clear();
  mSides[0]->reset();
  mSides[1]->reset();
  setOutput();

  emit loaded();
  emit selectionChanged();
  emit regionsChanged();
}

int MergeModel::unresolvedCount() const {
  int count = 0;
  for (const Conflict &conflict : mConflicts) {
    if (!conflict.touched)
      ++count;
  }

  return count;
}

int MergeModel::lineNumberWidth() const {
  int lines = 0;
  for (const Segment &segment : mSegments) {
    if (segment.conflict < 0) {
      lines += segment.common.size();
    } else {
      const Conflict &conflict = mConflicts.at(segment.conflict);
      lines += qMax(conflict.lines[0].size(), conflict.lines[1].size());
    }
  }

  return qMax(2, static_cast<int>(QString::number(lines).size()));
}

QObject *MergeModel::ours() const { return mSides[0]; }

QObject *MergeModel::theirs() const { return mSides[1]; }

QVariantList MergeModel::regions() const {
  QVariantList regions;
  for (int i = 0; i < mRegions.size() && i < mConflicts.size(); ++i) {
    const Region &region = mRegions.at(i);
    regions.append(QVariantMap{{"start", region.start.position()},
                               {"middle", region.middle.position()},
                               {"end", region.end.position()},
                               {"first", mConflicts.at(i).first},
                               {"touched", mConflicts.at(i).touched}});
  }

  return regions;
}

QVariantList MergeModel::layout() const {
  QVariantList layout;
  for (const Segment &segment : mSegments) {
    if (segment.conflict < 0) {
      int lines = segment.common.size();
      layout.append(
          QVariantMap{{"conflict", -1}, {"lines", QVariantList{lines, lines}}});
      continue;
    }

    const Conflict &conflict = mConflicts.at(segment.conflict);
    layout.append(
        QVariantMap{{"conflict", segment.conflict},
                    {"lines", QVariantList{conflict.lines[0].size(),
                                           conflict.lines[1].size()}}});
  }

  return layout;
}

void MergeModel::setDocument(QQuickTextDocument *document) {
  if (document)
    setTextDocument(document->textDocument());
}

void MergeModel::setTextDocument(QTextDocument *document) {
  if (!document || mDocument == document)
    return;

  mDocument = document;
  mHighlighter = new CodeHighlighter(mPath, mDocument);
  connect(mDocument, &QTextDocument::contentsChange, this,
          [this](int, int removed, int added) {
            if (removed || added)
              mRegionsTimer.start();
          });

  setOutput();
}

void MergeModel::setLineChecked(int side, int conflict, int line,
                                bool checked) {
  if (side < 0 || side > 1 || conflict < 0 || conflict >= mConflicts.size())
    return;

  Conflict &current = mConflicts[conflict];
  if (line < 0 || line >= current.checked[side].size() ||
      current.checked[side].at(line) == checked)
    return;

  // The side that is taken first comes first in the output.
  int other = 1 - side;
  if (checked && !current.checked[side].contains(true) &&
      !current.checked[other].contains(true))
    current.first = side;

  current.checked[side][line] = checked;
  current.touched = true;
  updateRegion(conflict);
  mSides[side]->updateChecks(conflict);
  emit selectionChanged();
}

void MergeModel::setConflictChecked(int side, int conflict, bool checked) {
  if (side < 0 || side > 1 || conflict < 0 || conflict >= mConflicts.size())
    return;

  Conflict &current = mConflicts[conflict];
  int other = 1 - side;
  if (checked && !current.checked[side].contains(true) &&
      !current.checked[other].contains(true))
    current.first = side;

  for (bool &value : current.checked[side])
    value = checked;
  current.touched = true;
  updateRegion(conflict);
  mSides[side]->updateChecks(conflict);
  emit selectionChanged();
}

void MergeModel::takeAll(int side) {
  if (side < 0 || side > 1)
    return;

  for (int i = 0; i < mConflicts.size(); ++i) {
    Conflict &conflict = mConflicts[i];
    for (bool &value : conflict.checked[side])
      value = true;
    for (bool &value : conflict.checked[1 - side])
      value = false;
    conflict.first = side;
    conflict.touched = true;
    updateRegion(i);
    mSides[0]->updateChecks(i);
    mSides[1]->updateChecks(i);
  }

  emit selectionChanged();
}

int MergeModel::conflictRow(int side, int conflict) const {
  return (side == 0 || side == 1) ? mSides[side]->conflictRow(conflict) : -1;
}

int MergeModel::regionStart(int conflict) const {
  return conflict >= 0 && conflict < mRegions.size()
             ? mRegions.at(conflict).start.position()
             : 0;
}

int MergeModel::outputRow(int position) const {
  if (!mDocument)
    return 0;

  QTextBlock block = mDocument->findBlock(position);
  return block.isValid() ? block.blockNumber()
                         : qMax(0, mDocument->blockCount() - 1);
}

int MergeModel::outputPosition(int row) const {
  if (!mDocument)
    return 0;

  QTextBlock block = mDocument->findBlockByNumber(row);
  return block.isValid() ? block.position() : mDocument->characterCount() - 1;
}

void MergeModel::save() {
  if (!mDocument || mPath.isEmpty() || !mNotice.isEmpty())
    return;

  // Conflicts without a choice are removed from the file.
  int unresolved = unresolvedCount();
  if (unresolved > 0) {
    ConfirmDialog dialog(mView);
    dialog.setTitle(tr("Unresolved Conflicts"));
    dialog.setText(unresolved == 1
                       ? tr("One conflict has no lines taken from either side.")
                       : tr("%1 conflicts have no lines taken from either side.")
                             .arg(unresolved));
    dialog.setInformativeText(
        tr("The lines of these conflicts won't be in the file."));
    dialog.setWarning(true);
    dialog.setAcceptText(tr("Save Anyway"));
    if (dialog.exec() != QDialog::Accepted)
      return;
  }

  QString text = mDocument->toPlainText();
  if (!mTrailingNewline && text.endsWith('\n'))
    text.chop(1);
  else if (mTrailingNewline && !text.isEmpty() && !text.endsWith('\n'))
    text += '\n';
  if (mCrlf)
    text.replace("\n", "\r\n");

  git::Repository repo = mView->repo();
  QSaveFile file(repo.workdir().filePath(mPath));
  if (!file.open(QFile::WriteOnly))
    return;

  QTextStream out(&file);
  out.setEncoding(mEncoding);
  out << text;
  out.flush();
  if (!file.commit())
    return;

  // Staging the file marks it as resolved.
  repo.index().setStaged({mPath}, true);
  mView->refresh();
}

bool MergeModel::parse(const QString &text) {
  mCrlf = text.contains("\r\n");
  mTrailingNewline = text.endsWith('\n');

  QStringList lines = text.split('\n');
  if (mTrailingNewline)
    lines.removeLast();

  enum State { Common, Ours, Base, Theirs };
  State state = Common;
  Segment common;
  Conflict conflict;
  for (QString line : lines) {
    if (line.endsWith('\r'))
      line.chop(1);

    if (state == Common && isMarker(line, '<')) {
      if (!common.common.isEmpty())
        mSegments.append(common);
      common = Segment();
      conflict = Conflict();
      if (mConflicts.isEmpty() && !label(line).isEmpty())
        mLabels[0] = label(line);
      state = Ours;
    } else if (state == Ours && isMarker(line, '|')) {
      state = Base;
    } else if ((state == Ours || state == Base) && isMarker(line, '=')) {
      state = Theirs;
    } else if (state == Theirs && isMarker(line, '>')) {
      if (mConflicts.isEmpty() && !label(line).isEmpty())
        mLabels[1] = label(line);

      for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < conflict.lines[side].size(); ++i)
          conflict.checked[side].append(false);
      }

      Segment segment;
      segment.conflict = mConflicts.size();
      mConflicts.append(conflict);
      mSegments.append(segment);
      state = Common;
    } else if (state == Common) {
      common.common.append(line);
    } else if (state == Ours) {
      conflict.lines[0].append(line);
    } else if (state == Base) {
      conflict.base.append(line);
    } else {
      conflict.lines[1].append(line);
    }
  }

  if (!common.common.isEmpty())
    mSegments.append(common);

  // An unfinished conflict isn't a conflict.
  return state == Common && !mConflicts.isEmpty();
}

void MergeModel::setOutput() {
  if (!mDocument)
    return;

  // The common lines, and nothing yet for the conflicts. The text is set at
  // once, since cursors at the end would move with text added after them.
  QString text;
  QList<int> positions;
  for (const Segment &segment : mSegments) {
    if (segment.conflict < 0) {
      for (const QString &line : segment.common)
        text += line + '\n';
    } else {
      positions.append(text.size());
    }
  }

  mDocument->setUndoRedoEnabled(false);
  mDocument->setPlainText(text);
  mRegions.clear();

  // The start stays before the lines that are taken.
  for (int position : positions) {
    Region region;
    region.start = QTextCursor(mDocument);
    region.start.setPosition(position);
    region.start.setKeepPositionOnInsert(true);
    region.middle = QTextCursor(mDocument);
    region.middle.setPosition(position);
    region.end = QTextCursor(mDocument);
    region.end.setPosition(position);
    mRegions.append(region);
  }

  mDocument->setUndoRedoEnabled(true);
  mDocument->setModified(false);

  // Conflicts that were already chosen.
  for (int i = 0; i < mConflicts.size(); ++i) {
    if (mConflicts.at(i).checked[0].contains(true) ||
        mConflicts.at(i).checked[1].contains(true))
      updateRegion(i);
  }

  if (mHighlighter) {
    mHighlighter->setPath(mPath);
    mHighlighter->restyle();
  }

  emit regionsChanged();
}

void MergeModel::updateRegion(int index) {
  if (!mDocument || index < 0 || index >= mRegions.size())
    return;

  // The taken lines of the first side, then of the other one.
  const Conflict &conflict = mConflicts.at(index);
  QString parts[2];
  for (int side = 0; side < 2; ++side) {
    for (int i = 0; i < conflict.lines[side].size(); ++i) {
      if (conflict.checked[side].at(i))
        parts[side] += conflict.lines[side].at(i) + '\n';
    }
  }

  QString first = parts[conflict.first];
  QString second = parts[1 - conflict.first];

  Region &region = mRegions[index];
  QTextCursor cursor(mDocument);
  cursor.setPosition(region.start.position());
  cursor.setPosition(region.end.position(), QTextCursor::KeepAnchor);
  cursor.beginEditBlock();
  cursor.insertText(first);
  int middle = cursor.position();
  cursor.insertText(second);
  cursor.endEditBlock();

  region.middle.setPosition(middle);
  region.end.setPosition(cursor.position());
  emit regionsChanged();
}
