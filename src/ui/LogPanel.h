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
#include <QPointer>

class LogEntry;
class LogModel;
class QAbstractItemModel;
class QPoint;

// Controller of the activity log at the bottom of the repository page and
// of the clone progress. qrc:/qml/LogPanel.qml draws it.
class LogPanel : public QObject {
  Q_OBJECT

  Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)
  Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
  Q_PROPERTY(bool collapseEnabled READ isCollapseEnabled NOTIFY
                 collapseEnabledChanged)
  Q_PROPERTY(QString alert READ alert NOTIFY alertChanged)
  Q_PROPERTY(int alertCount READ alertCount NOTIFY alertChanged)

public:
  LogPanel(LogEntry *root, QObject *parent = nullptr);

  QAbstractItemModel *model() const;

  bool isVisible() const { return mVisible; }
  void setVisible(bool visible);

  // Collapse the previous entry when a new one is added.
  bool isCollapseEnabled() const { return mCollapse; }
  void setCollapseEnabled(bool collapse);

  void setEntryExpanded(LogEntry *entry, bool expanded);

  // Expand the entries down to 'entry' and scroll to it.
  void reveal(LogEntry *entry);

  // The last error that wasn't acknowledged, for the banner of the page,
  // and how many errors weren't. Showing, dismissing or hiding the log
  // acknowledges them.
  QString alert() const { return mAlert; }
  int alertCount() const { return mAlertCount; }
  void addAlert(LogEntry *error);
  Q_INVOKABLE void showAlert();
  Q_INVOKABLE void dismissAlert();

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
  void revealRequested(const QModelIndex &index);
  void closeRequested();
  void showRequested();
  void alertChanged();

private:
  void showMenu(const QPersistentModelIndex &index, const QPoint &pos);
  void copyIndexes(const QModelIndexList &indexes);

  LogModel *mModel;
  bool mVisible = false;
  bool mCollapse = true;

  QString mAlert;
  int mAlertCount = 0;
  QPointer<LogEntry> mAlertEntry;
};

#endif
