//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SEARCHFIELD_H
#define SEARCHFIELD_H

#include "index/Index.h"
#include <QFutureWatcher>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariantList>

class ToolBar;

// The search field of the tool bar, drawn by qrc:/qml/SearchField.qml as
// 'search'. It completes the words of the index of the current repository
// and has a panel to build advanced queries. Both drop down from the field.
class SearchField : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
  Q_PROPERTY(QString placeholderText READ placeholderText NOTIFY
                 placeholderTextChanged)
  Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
  Q_PROPERTY(QStringList completions READ completions NOTIFY
                 completionsChanged)
  Q_PROPERTY(bool completionsVisible READ completionsVisible NOTIFY
                 completionsVisibleChanged)
  Q_PROPERTY(bool advancedVisible READ advancedVisible NOTIFY
                 advancedVisibleChanged)
  Q_PROPERTY(int completionIndex READ completionIndex NOTIFY
                 completionIndexChanged)
  Q_PROPERTY(QVariantList advancedFields READ advancedFields NOTIFY
                 advancedFieldsChanged)

public:
  SearchField(ToolBar *parent);
  ~SearchField() override;

  QString text() const { return mText; }
  void setText(const QString &text);

  QString placeholderText() const { return mPlaceholderText; }
  void setPlaceholderText(const QString &text);

  bool isEnabled() const { return mEnabled; }
  void setEnabled(bool enabled);

  // Called by QML when the text is edited.
  Q_INVOKABLE void edit(const QString &text, int cursor);

  QStringList completions() const { return mCompletions; }
  bool completionsVisible() const { return mCompletionsVisible; }
  bool advancedVisible() const { return mAdvancedVisible; }
  int completionIndex() const { return mCompletionIndex; }

  // These return true if the list of completions handled the key.
  Q_INVOKABLE bool moveCompletion(int delta);
  Q_INVOKABLE bool acceptCompletion();
  Q_INVOKABLE bool hideCompletions();
  Q_INVOKABLE void applyCompletion(int index);

  // The fields of the advanced search, each with a 'label', a 'tip', a
  // 'value' and whether it's a 'date' or starts a new 'group'.
  QVariantList advancedFields() const;

  Q_INVOKABLE void showAdvanced();
  Q_INVOKABLE void hideAdvanced();
  Q_INVOKABLE void setAdvancedValue(int index, const QString &value);
  Q_INVOKABLE QStringList advancedCompletions(int index,
                                              const QString &prefix) const;
  Q_INVOKABLE QString formatDate(int year, int month, int day) const;
  // Replace the text with the query of the advanced fields.
  Q_INVOKABLE void acceptAdvanced();

signals:
  void textChanged(const QString &text);
  void placeholderTextChanged();
  void enabledChanged();
  void completionsChanged();
  void completionIndexChanged();
  void advancedFieldsChanged();
  void completionsVisibleChanged();
  void advancedVisibleChanged();

  // Move the cursor of the field to 'position'.
  void cursorRequested(int position);

private:
  // A term of a query that matches 'text' in 'field'.
  static QString term(Index::Field field, const QString &text);

  Index *index() const;
  void updateCompletions(int cursor);
  void setCompletionIndex(int index);
  void setCompletionsVisible(bool visible);

  ToolBar *mToolBar;
  QString mText;
  QString mPlaceholderText;
  bool mEnabled = false;

  bool mCompletionsVisible = false;
  QStringList mCompletions;
  int mCompletionIndex = -1;
  // The word that the completions replace.
  int mWordStart = 0;
  int mWordLength = 0;

  bool mAdvancedVisible = false;
  QStringList mAdvancedValues;
  QStringList mOtherTerms;
  QMap<Index::Field, QStringList> mFieldMap;
  QFutureWatcher<QMap<Index::Field, QStringList>> mFieldMapWatcher;
};

#endif
