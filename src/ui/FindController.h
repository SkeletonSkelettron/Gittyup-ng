//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef FINDCONTROLLER_H
#define FINDCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <functional>

// Rows of text that can be searched, like the lines of a diff.
class FindTarget {
public:
  virtual ~FindTarget() {}

  virtual int findRowCount() const = 0;

  // The text of 'row' as it's shown, with tabs expanded. Empty for rows
  // that aren't searched.
  virtual QString findRowText(int row) const = 0;

  // Remember what is found to draw the matches. 'row' and 'start' are the
  // current match, or -1.
  virtual void setFindState(const QString &text, int row, int start) = 0;

protected:
  // The matches of 'text' in 'row', with the current one marked, for the
  // matches role of the models.
  QVariantList findMatches(const QString &text, int row, int currentRow,
                           int currentStart) const;
};

// Finds text in the target that is shown, like the diff of the selected
// file. qrc:/qml/FindBar.qml draws it as 'finder'.
class FindController : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
  Q_PROPERTY(QString searchText READ searchText NOTIFY searchTextChanged)
  Q_PROPERTY(QString hitsText READ hitsText NOTIFY hitsChanged)
  Q_PROPERTY(bool hasMatches READ hasMatches NOTIFY hitsChanged)

public:
  using TargetFunc = std::function<FindTarget *()>;

  FindController(const TargetFunc &target, QObject *parent = nullptr);

  // The text to find, shared by all find bars.
  static QString text() { return sText; }
  static void setText(const QString &text) { sText = text; }

  bool isVisible() const { return mVisible; }
  QString searchText() const;
  QString hitsText() const;
  bool hasMatches() const { return !mMatches.isEmpty(); }

  // Show the bar and focus its field.
  Q_INVOKABLE void show();
  Q_INVOKABLE void hide();

  Q_INVOKABLE void search(const QString &text);
  Q_INVOKABLE void next();
  Q_INVOKABLE void previous();

  // Find again after the target or its content changed.
  void refresh();

signals:
  void visibleChanged();
  void searchTextChanged();
  void hitsChanged();
  void focusRequested();

  // Scroll to the current match.
  void currentChanged(int row, int start, int length);

private:
  struct Match {
    int row;
    int start;
  };

  void update();
  void setCurrent(int index);

  TargetFunc mTarget;
  FindTarget *mLastTarget = nullptr;
  bool mVisible = false;

  QList<Match> mMatches;
  int mCurrent = -1;

  static QString sText;
};

#endif
