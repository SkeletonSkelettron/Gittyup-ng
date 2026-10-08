//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "LogPanel.h"
#include "qml/QmlSupport.h"
#include "log/LogEntry.h"
#include "log/LogModel.h"
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QMenu>
#include <QMetaMethod>
#include <QMimeData>
#include <QRegularExpression>

namespace {

// The index and all of its descendants.
void collect(const QAbstractItemModel *model, const QModelIndex &index,
             QModelIndexList &indexes) {
  indexes.append(index);
  for (int i = 0; i < model->rowCount(index); ++i)
    collect(model, model->index(i, 0, index), indexes);
}

} // namespace

LogPanel::LogPanel(LogEntry *root, QObject *parent)
    : QObject(parent),
      mModel(new LogModel(root, QApplication::style(), this)) {}

QAbstractItemModel *LogPanel::model() const { return mModel; }

void LogPanel::setVisible(bool visible) {
  if (visible == mVisible)
    return;

  mVisible = visible;
  emit visibleChanged();
}

void LogPanel::setCollapseEnabled(bool collapse) {
  if (collapse == mCollapse)
    return;

  mCollapse = collapse;
  emit collapseEnabledChanged();
}

void LogPanel::setEntryExpanded(LogEntry *entry, bool expanded) {
  emit expandRequested(mModel->index(entry), expanded);
}

bool LogPanel::hasProblems(const QModelIndex &index) const {
  if (!index.isValid())
    return false;

  int kind = index.data(LogModel::KindRole).toInt();
  if (kind == LogEntry::Warning || kind == LogEntry::Error)
    return true;

  for (int i = 0; i < mModel->rowCount(index); ++i) {
    if (hasProblems(mModel->index(i, 0, index)))
      return true;
  }

  return false;
}

void LogPanel::activateLink(const QString &link) { emit linkActivated(link); }

void LogPanel::cancel(const QModelIndex &index) {
  if (index.isValid() && index.data(LogModel::ProgressRole).toInt() >= 0)
    emit operationCanceled(index);
}

void LogPanel::copy(const QModelIndex &index) {
  if (!index.isValid())
    return;

  QModelIndexList indexes;
  collect(mModel, index, indexes);
  copyIndexes(indexes);
}

void LogPanel::copyAll() {
  QModelIndexList indexes;
  for (int i = 0; i < mModel->rowCount(); ++i)
    collect(mModel, mModel->index(i, 0), indexes);
  copyIndexes(indexes);
}

void LogPanel::showMenu(const QModelIndex &index, qreal x, qreal y) {
  Q_UNUSED(x)
  Q_UNUSED(y)

  QMenu menu;
  QAction *copyAction =
      menu.addAction(tr("Copy"), this, [this, index] { copy(index); });
  copyAction->setEnabled(index.isValid());
  menu.addAction(tr("Copy All"), this, &LogPanel::copyAll);
  if (isSignalConnected(QMetaMethod::fromSignal(&LogPanel::closeRequested))) {
    menu.addSeparator();
    menu.addAction(tr("Hide Log"), this, &LogPanel::close);
  }
  QmlSupport::execMenu(&menu, QCursor::pos());
}

void LogPanel::close() { emit closeRequested(); }

void LogPanel::copyIndexes(const QModelIndexList &indexes) {
  QString plainText;
  QString richText;
  for (const QModelIndex &index : indexes) {
    // Indent child indexes.
    QString prefix;
    for (QModelIndex parent = index.parent(); parent.isValid();
         parent = parent.parent())
      prefix += QString(4, ' ');

    QString text = index.data().toString();
    plainText += prefix + QString(text).remove(QRegularExpression("<[^>]*>")) +
                 '\n';
    richText += prefix + text + "<br>";
  }

  QMimeData *mimeData = new QMimeData;
  mimeData->setText(plainText);
  mimeData->setHtml(QString("<pre>%1</pre>").arg(richText));
  QApplication::clipboard()->setMimeData(mimeData);
}
