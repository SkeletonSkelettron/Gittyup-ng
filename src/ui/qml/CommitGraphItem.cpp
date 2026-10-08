//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "CommitGraphItem.h"
#include <QPainter>
#include <QPainterPath>

namespace {

// Keep in sync with GraphSegment in CommitList.cpp.
namespace Segment {
enum GraphSegment {
  Dot,
  Top,
  Middle,
  Bottom,
  Cross,
  LeftIn,
  LeftOut,
  RightIn,
  RightOut
};
} // namespace Segment

// Lines to the uncommitted changes row are drawn with this color.
const QColor kTaintedColor = Qt::gray;

} // namespace

CommitGraphItem::CommitGraphItem(QQuickItem *parent)
    : QQuickPaintedItem(parent) {
  setAntialiasing(true);
}

void CommitGraphItem::setColumns(const QVariantList &columns) {
  mColumns = columns;
  update();
}

void CommitGraphItem::setColors(const QVariantList &colors) {
  mColors = colors;
  update();
}

void CommitGraphItem::setLaneWidth(qreal width) {
  mLaneWidth = width;
  update();
}

void CommitGraphItem::setInitials(const QString &initials) {
  mInitials = initials;
  update();
}

void CommitGraphItem::setMerge(bool merge) {
  mMerge = merge;
  update();
}

void CommitGraphItem::setStatus(bool status) {
  mStatus = status;
  update();
}

void CommitGraphItem::setStatusColor(const QColor &color) {
  mStatusColor = color;
  update();
}

void CommitGraphItem::setTextColor(const QColor &color) {
  mTextColor = color;
  update();
}

void CommitGraphItem::paint(QPainter *painter) {
  painter->setRenderHint(QPainter::Antialiasing);

  qreal w = mLaneWidth;
  qreal h = height();
  qreal cy = h / 2;
  qreal curve = h / 4;

  // Node radius. Merges get a small dot, commits a circle with initials.
  qreal r = mMerge ? 4 : qMin(w, h) / 2 - 1;
  if (mStatus)
    r = qMin(w, h) / 2 - 3;

  QPointF node;
  QColor nodeColor;
  bool hasNode = false;

  for (int i = 0; i < mColumns.size(); ++i) {
    qreal x = i * w;
    qreal x1 = x + w / 2;
    qreal x2 = x + w;
    qreal y1 = cy - r;
    qreal y3 = cy + r;
    qreal y4 = cy + curve;

    QVariantList segments = mColumns.at(i).toList();
    QVariantList colors = mColors.value(i).toList();
    for (int j = 0; j < segments.size(); ++j) {
      QColor color = colors.value(j).value<QColor>();
      QPen pen(color, 2, Qt::SolidLine, Qt::RoundCap);
      if (color == kTaintedColor) {
        pen.setColor(mStatusColor);
        pen.setStyle(Qt::DashLine);
        pen.setDashPattern({2, 2});
      }

      painter->setPen(pen);
      painter->setBrush(Qt::NoBrush);
      switch (segments.at(j).toInt()) {
        case Segment::Dot:
          node = QPointF(x1, cy);
          nodeColor = color;
          hasNode = true;
          break;

        case Segment::Top:
          painter->drawLine(QPointF(x1, 0), QPointF(x1, y1));
          break;

        case Segment::Middle:
          painter->drawLine(QPointF(x1, y1), QPointF(x1, y3));
          break;

        case Segment::Bottom:
          painter->drawLine(QPointF(x1, y3), QPointF(x1, h));
          break;

        case Segment::Cross:
          painter->drawLine(QPointF(x, y4), QPointF(x2, y4));
          break;

        case Segment::RightOut: {
          QPainterPath path;
          path.moveTo(x1, y3);
          path.quadTo(x1, y4, x2, y4);
          painter->drawPath(path);
          break;
        }

        case Segment::LeftOut: {
          QPainterPath path;
          path.moveTo(x1, y3);
          path.quadTo(x1, y4, x, y4);
          painter->drawPath(path);
          break;
        }

        case Segment::RightIn: {
          QPainterPath path;
          path.moveTo(x1, h);
          path.quadTo(x1, y4, x2, y4);
          painter->drawPath(path);
          break;
        }

        case Segment::LeftIn: {
          QPainterPath path;
          path.moveTo(x1, h);
          path.quadTo(x1, y4, x, y4);
          painter->drawPath(path);
          break;
        }
      }
    }
  }

  if (!hasNode)
    return;

  // Draw the node on top of the lanes.
  if (mStatus || !nodeColor.isValid()) {
    QPen pen(mStatusColor, 2);
    pen.setStyle(Qt::DashLine);
    pen.setDashPattern({2, 1.5});
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(node, r, r);
    return;
  }

  painter->setPen(QPen(nodeColor, 2));
  painter->setBrush(mMerge ? nodeColor : nodeColor.darker(135));
  painter->drawEllipse(node, r, r);

  if (mMerge || mInitials.isEmpty())
    return;

  QFont font = painter->font();
  font.setPixelSize(qMax(7, qRound(r * 0.85)));
  font.setBold(true);
  painter->setFont(font);
  painter->setPen(mTextColor);
  QRectF rect(node.x() - r, node.y() - r, 2 * r, 2 * r);
  painter->drawText(rect, Qt::AlignCenter, mInitials);
}
