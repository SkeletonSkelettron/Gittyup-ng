//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef QMLSUPPORT_H
#define QMLSUPPORT_H

#include <QObject>
#include <QPoint>
#include <QRectF>
#include <QString>
#include <QVariantMap>

class QAction;
class QImage;
class QMenu;
class QQuickWidget;
class QWidget;

// Published to every QML view as "host". Views that draw popups show tool
// tips and menus themselves, see setDrawsPopups(). Other views show native
// ones.
class QmlHost : public QObject {
  Q_OBJECT

  // The tool tip that a view that draws popups shows below 'toolTipRect'.
  Q_PROPERTY(QString toolTipText READ toolTipText NOTIFY toolTipChanged)
  Q_PROPERTY(QRectF toolTipRect READ toolTipRect NOTIFY toolTipChanged)
  Q_PROPERTY(bool toolTipVisible READ toolTipVisible NOTIFY toolTipChanged)

public:
  QmlHost(QQuickWidget *view);

  // Coordinates are in the scene coordinates of the view's root item.
  Q_INVOKABLE void showToolTip(const QString &text, qreal x, qreal y,
                               qreal width, qreal height);
  Q_INVOKABLE void hideToolTip();

  QString toolTipText() const { return mToolTipText; }
  QRectF toolTipRect() const { return mToolTipRect; }
  bool toolTipVisible() const { return mToolTipVisible; }

  QPoint mapToGlobal(qreal x, qreal y) const;
  // Show 'menu' at a point of the scene and wait until it closes.
  void popup(QMenu *menu, qreal x, qreal y) const;

signals:
  void toolTipChanged();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  QQuickWidget *mView;
  QString mToolTipText;
  QRectF mToolTipRect;
  bool mToolTipVisible = false;
};

namespace QmlSupport {

// Create a view showing qrc:/qml/<name>.qml. Each entry of 'context' is
// published to QML as a context property, along with "host". The view
// uses the current theme and can load tinted icons from
// "image://icons/<name>/<rrggbb>".
QQuickWidget *createView(const QString &name, const QVariantMap &context,
                         QWidget *parent);

// Get the host object of a view created with createView().
QmlHost *host(QQuickWidget *view);

// Views that are big enough for popups, like the view of the main window,
// draw menus with qrc:/qml/ContextMenu.qml and tool tips from the host
// instead of showing native ones.
void setDrawsPopups(QQuickWidget *view, bool draws);
bool drawsPopups(QQuickWidget *view);

// Show the actions of 'menu' at 'pos' on the screen and wait until the
// menu closes, like QMenu::exec(). The menu is drawn by the view at 'pos'
// if it draws popups. Returns the triggered action.
QAction *execMenu(QMenu *menu, const QPoint &pos);

// Images for QML at "image://images/<key>". Remove them when they're no
// longer shown.
QString addImage(const QImage &image);
void removeImage(const QString &url);

} // namespace QmlSupport

#endif
