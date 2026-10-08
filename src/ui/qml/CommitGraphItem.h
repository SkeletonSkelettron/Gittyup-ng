//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef COMMITGRAPHITEM_H
#define COMMITGRAPHITEM_H

#include <QColor>
#include <QQuickPaintedItem>
#include <QVariantList>

// Draws one row of the commit graph: the lanes that pass through the row
// and the node of the commit. Available in QML as Gittyup.CommitGraph.
class CommitGraphItem : public QQuickPaintedItem {
  Q_OBJECT

  Q_PROPERTY(QVariantList columns READ columns WRITE setColumns)
  Q_PROPERTY(QVariantList colors READ colors WRITE setColors)
  Q_PROPERTY(qreal laneWidth READ laneWidth WRITE setLaneWidth)
  Q_PROPERTY(QString initials READ initials WRITE setInitials)
  Q_PROPERTY(bool merge READ merge WRITE setMerge)
  Q_PROPERTY(bool status READ status WRITE setStatus)
  Q_PROPERTY(QColor statusColor READ statusColor WRITE setStatusColor)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)

public:
  CommitGraphItem(QQuickItem *parent = nullptr);

  QVariantList columns() const { return mColumns; }
  QVariantList colors() const { return mColors; }
  qreal laneWidth() const { return mLaneWidth; }
  QString initials() const { return mInitials; }
  bool merge() const { return mMerge; }
  bool status() const { return mStatus; }
  QColor statusColor() const { return mStatusColor; }
  QColor textColor() const { return mTextColor; }

  void setColumns(const QVariantList &columns);
  void setColors(const QVariantList &colors);
  void setLaneWidth(qreal width);
  void setInitials(const QString &initials);
  void setMerge(bool merge);
  void setStatus(bool status);
  void setStatusColor(const QColor &color);
  void setTextColor(const QColor &color);

  void paint(QPainter *painter) override;

private:
  QVariantList mColumns;
  QVariantList mColors;
  qreal mLaneWidth = 18;
  QString mInitials;
  bool mMerge = false;
  bool mStatus = false;
  QColor mStatusColor = Qt::gray;
  QColor mTextColor = Qt::white;
};

#endif
