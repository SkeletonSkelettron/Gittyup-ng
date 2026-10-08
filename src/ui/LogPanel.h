//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QModelIndex>
#include <QObject>

class LogEntry;
class LogModel;
class QAbstractItemModel;

// Controller of the activity log at the bottom of the repository page and
// of the clone progress. qrc:/qml/LogPanel.qml draws it.
class LogPanel : public QObject {
  Q_OBJECT

  Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)
  Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
  Q_PROPERTY(bool collapseEnabled READ isCollapseEnabled NOTIFY
                 collapseEnabledChanged)

public:
  LogPanel(LogEntry *root, QObject *parent = nullptr);

  QAbstractItemModel *model() const;

  bool isVisible() const { return mVisible; }
  void setVisible(bool visible);

  // Collapse the previous entry when a new one is added.
  bool isCollapseEnabled() const { return mCollapse; }
  void setCollapseEnabled(bool collapse);

  void setEntryExpanded(LogEntry *entry, bool expanded);

  // Whether the entry or one of its descendants is a warning or an error.
  Q_INVOKABLE bool hasProblems(const QModelIndex &index) const;

  Q_INVOKABLE void activateLink(const QString &link);
  Q_INVOKABLE void cancel(const QModelIndex &index);
  Q_INVOKABLE void copy(const QModelIndex &index);
  Q_INVOKABLE void copyAll();
  Q_INVOKABLE void showMenu(const QModelIndex &index, qreal x, qreal y);
  Q_INVOKABLE void close();

signals:
  void visibleChanged();
  void collapseEnabledChanged();
  void linkActivated(const QString &link);
  void operationCanceled(const QModelIndex &index);
  void expandRequested(const QModelIndex &index, bool expanded);
  void closeRequested();

private:
  void copyIndexes(const QModelIndexList &indexes);

  LogModel *mModel;
  bool mVisible = false;
  bool mCollapse = true;
};

#endif
