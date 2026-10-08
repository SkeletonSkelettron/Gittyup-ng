//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "SearchField.h"
#include "RepoView.h"
#include "MainWindow.h"
#include "ToolBar.h"
#include "index/Query.h"
#include <QDate>
#include <QRegularExpression>
#include <QtConcurrent>

namespace {

const int kMaxCompletions = 200;

const QString kParenFmt = "(%1)";
const QString kFieldFmt = "%1:%2";

const QRegularExpression kWsRe("\\s");

// Find the word at 'index' and move 'index' to its start.
QString word(const QString &text, int &index) {
  index = text.lastIndexOf(kWsRe, qMax(0, index - 1)) + 1;
  return text.mid(index, text.indexOf(kWsRe, index) - index);
}

struct AdvancedField {
  Index::Field field;
  const char *label;
  const char *tip;
  bool date;
  int group;
};

const AdvancedField kAdvancedFields[] = {
    {Index::Any, QT_TRANSLATE_NOOP("SearchField", "Words"),
     QT_TRANSLATE_NOOP("SearchField", "Anywhere in the commit"), false, -1},
    {Index::Author, QT_TRANSLATE_NOOP("SearchField", "Author"),
     QT_TRANSLATE_NOOP("SearchField", "Author name"), false, 0},
    {Index::Email, QT_TRANSLATE_NOOP("SearchField", "Email"),
     QT_TRANSLATE_NOOP("SearchField", "Author email"), false, 0},
    {Index::Message, QT_TRANSLATE_NOOP("SearchField", "Message"),
     QT_TRANSLATE_NOOP("SearchField", "Commit message"), false, 0},
    {Index::Date, QT_TRANSLATE_NOOP("SearchField", "Date"),
     QT_TRANSLATE_NOOP("SearchField", "Specific commit date"), true, 1},
    {Index::After, QT_TRANSLATE_NOOP("SearchField", "After"),
     QT_TRANSLATE_NOOP("SearchField", "Commits after date"), true, 1},
    {Index::Before, QT_TRANSLATE_NOOP("SearchField", "Before"),
     QT_TRANSLATE_NOOP("SearchField", "Commits before date"), true, 1},
    {Index::File, QT_TRANSLATE_NOOP("SearchField", "File"),
     QT_TRANSLATE_NOOP("SearchField", "File name"), false, 2},
    {Index::Path, QT_TRANSLATE_NOOP("SearchField", "Path"),
     QT_TRANSLATE_NOOP("SearchField", "File path"), false, 2},
    {Index::Scope, QT_TRANSLATE_NOOP("SearchField", "Scope"),
     QT_TRANSLATE_NOOP("SearchField", "Hunk header text"), false, 2},
    {Index::Context, QT_TRANSLATE_NOOP("SearchField", "Context"),
     QT_TRANSLATE_NOOP("SearchField", "Diff context (white)"), false, 3},
    {Index::Addition, QT_TRANSLATE_NOOP("SearchField", "Addition"),
     QT_TRANSLATE_NOOP("SearchField", "Diff addition (green)"), false, 3},
    {Index::Deletion, QT_TRANSLATE_NOOP("SearchField", "Deletion"),
     QT_TRANSLATE_NOOP("SearchField", "Diff deletion (red)"), false, 3},
    {Index::Comment, QT_TRANSLATE_NOOP("SearchField", "Comment"),
     QT_TRANSLATE_NOOP("SearchField", "Source code comment"), false, 4},
    {Index::String, QT_TRANSLATE_NOOP("SearchField", "String"),
     QT_TRANSLATE_NOOP("SearchField", "Source code string literal"), false,
     4},
    {Index::Identifier, QT_TRANSLATE_NOOP("SearchField", "Identifier"),
     QT_TRANSLATE_NOOP("SearchField", "Source code identifier"), false, 4}};

const int kAdvancedFieldCount =
    sizeof(kAdvancedFields) / sizeof(kAdvancedFields[0]);

} // namespace

SearchField::SearchField(ToolBar *parent)
    : QObject(parent), mToolBar(parent), mPlaceholderText(tr("Search")) {
  for (int i = 0; i < kAdvancedFieldCount; ++i)
    mAdvancedValues.append(QString());

  connect(&mFieldMapWatcher,
          &QFutureWatcher<QMap<Index::Field, QStringList>>::finished, this,
          [this] { mFieldMap = mFieldMapWatcher.result(); });
}

SearchField::~SearchField() { mFieldMapWatcher.waitForFinished(); }

void SearchField::setText(const QString &text) {
  if (text == mText)
    return;

  mText = text;
  emit textChanged(text);
}

void SearchField::setPlaceholderText(const QString &text) {
  if (text == mPlaceholderText)
    return;

  mPlaceholderText = text;
  emit placeholderTextChanged();
}

void SearchField::setEnabled(bool enabled) {
  if (enabled == mEnabled)
    return;

  mEnabled = enabled;
  emit enabledChanged();

  if (!enabled) {
    hideCompletions();
    hideAdvanced();
  }
}

void SearchField::edit(const QString &text, int cursor) {
  setText(text);
  updateCompletions(cursor);
}

bool SearchField::moveCompletion(int delta) {
  if (!mCompletionsVisible)
    return false;

  // Moving before the first completion goes back to the text.
  int index = mCompletionIndex + delta;
  setCompletionIndex(qBound(-1, index, mCompletions.size() - 1));
  return true;
}

bool SearchField::acceptCompletion() {
  if (!mCompletionsVisible)
    return false;

  if (mCompletionIndex < 0) {
    hideCompletions();
    return false;
  }

  applyCompletion(mCompletionIndex);
  return true;
}

bool SearchField::hideCompletions() {
  if (!mCompletionsVisible)
    return false;

  setCompletionsVisible(false);
  return true;
}

void SearchField::applyCompletion(int index) {
  if (index < 0 || index >= mCompletions.size())
    return;

  QString completion = mCompletions.at(index);
  QString text = mText;
  text.replace(mWordStart, mWordLength, completion);
  hideCompletions();
  setText(text);
  emit cursorRequested(mWordStart + completion.length());
}

QVariantList SearchField::advancedFields() const {
  QVariantList fields;
  for (int i = 0; i < kAdvancedFieldCount; ++i) {
    const AdvancedField &field = kAdvancedFields[i];
    bool group = (i > 0 && kAdvancedFields[i - 1].group != field.group);
    fields.append(QVariantMap{{"label", tr(field.label)},
                              {"tip", tr(field.tip)},
                              {"date", field.date},
                              {"group", group},
                              {"value", mAdvancedValues.at(i)}});
  }

  return fields;
}

void SearchField::showAdvanced() {
  Index *index = this->index();
  if (!index)
    return;

  hideCompletions();

  // Load the values from the query.
  // FIXME: Phrase queries are lossy.
  QMap<Index::Field, QStringList> map;
  if (QueryRef query = Query::parseQuery(mText)) {
    for (const Index::Term &term : query->terms())
      map[term.field].append(term.text);
  }

  for (int i = 0; i < kAdvancedFieldCount; ++i)
    mAdvancedValues[i] = map.take(kAdvancedFields[i].field).join(' ');
  emit advancedFieldsChanged();

  // Keep the terms of other fields, like 'is:starred'.
  mOtherTerms.clear();
  for (auto it = map.cbegin(); it != map.cend(); ++it) {
    for (const QString &text : it.value())
      mOtherTerms.append(term(it.key(), text));
  }

  // Load the completions in the background.
  mFieldMap.clear();
  if (!mFieldMapWatcher.isRunning())
    mFieldMapWatcher.setFuture(
        QtConcurrent::run([index] { return index->fieldMap(); }));

  if (!mAdvancedVisible) {
    mAdvancedVisible = true;
    emit advancedVisibleChanged();
  }
}

void SearchField::hideAdvanced() {
  if (!mAdvancedVisible)
    return;

  mAdvancedVisible = false;
  mFieldMap.clear();
  emit advancedVisibleChanged();
}

void SearchField::setAdvancedValue(int index, const QString &value) {
  if (index >= 0 && index < mAdvancedValues.size())
    mAdvancedValues[index] = value;
}

QStringList SearchField::advancedCompletions(int index,
                                             const QString &prefix) const {
  if (index < 0 || index >= kAdvancedFieldCount || prefix.isEmpty())
    return QStringList();

  QStringList completions;
  for (const QString &value : mFieldMap.value(kAdvancedFields[index].field)) {
    if (value.startsWith(prefix, Qt::CaseInsensitive) &&
        value.compare(prefix, Qt::CaseInsensitive) != 0 &&
        !completions.contains(value)) {
      completions.append(value);
      if (completions.size() >= kMaxCompletions)
        break;
    }
  }

  return completions;
}

QString SearchField::formatDate(int year, int month, int day) const {
  return QDate(year, month, day).toString(Index::dateFormat());
}

void SearchField::acceptAdvanced() {
  QStringList terms;
  for (int i = 0; i < kAdvancedFieldCount; ++i) {
    QString text = mAdvancedValues.at(i).trimmed();
    if (!text.isEmpty())
      terms.append(term(kAdvancedFields[i].field, text));
  }

  hideAdvanced();
  setText((terms + mOtherTerms).join(' '));
}

QString SearchField::term(Index::Field field, const QString &text) {
  // Words without a field match anywhere.
  if (field == Index::Any)
    return text;

  // Enclose queries with embedded spaces in parentheses.
  QString value = text;
  if (value.contains(' ') && (!value.startsWith('"') || !value.endsWith('"')))
    value = kParenFmt.arg(value);

  return kFieldFmt.arg(Index::fieldName(field), value);
}

Index *SearchField::index() const {
  RepoView *view = mToolBar->currentView();
  return view ? view->index() : nullptr;
}

void SearchField::updateCompletions(int cursor) {
  QStringList completions;
  int start = cursor;
  QString term = word(mText, start);
  Index *index = this->index();
  if (index && !term.isEmpty()) {
    // The words of the dictionary are sorted and in lower case.
    QByteArray key = term.toLower().toUtf8();
    const Index::Dictionary &dict = index->dict();
    auto it = std::lower_bound(dict.cbegin(), dict.cend(), Index::Word(key));
    for (; it != dict.cend() && it->key.startsWith(key); ++it) {
      completions.append(QString::fromUtf8(it->key));
      if (completions.size() >= kMaxCompletions)
        break;
    }
  }

  // Don't offer the word that is already there.
  if (completions.size() == 1 &&
      completions.first().compare(term, Qt::CaseInsensitive) == 0)
    completions.clear();

  mWordStart = start;
  mWordLength = term.length();
  mCompletions = completions;
  emit completionsChanged();
  setCompletionIndex(-1);

  setCompletionsVisible(!completions.isEmpty());
}

void SearchField::setCompletionIndex(int index) {
  if (index == mCompletionIndex)
    return;

  mCompletionIndex = index;
  emit completionIndexChanged();
}

void SearchField::setCompletionsVisible(bool visible) {
  if (visible == mCompletionsVisible)
    return;

  mCompletionsVisible = visible;
  emit completionsVisibleChanged();
}
