//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "QmlSupport.h"
#include "CommitGraphItem.h"
#include "QmlTheme.h"
#include "host/Account.h"
#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QCursor>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QHash>
#include <QImage>
#include <QImageReader>
#include <QMenu>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QPointer>
#include <QQuickWidget>
#include <QToolTip>

namespace {

const QSize kDefaultIconSize(48, 48);

// Serves "image://icons/<name>/<rrggbb>". Monochrome SVG icons from
// qrc:/qml/icons/<name>.svg are drawn with 'currentColor', which is replaced
// by the requested color. "account-<kind>" serves the remote account icons.
class IconProvider : public QQuickImageProvider {
public:
  IconProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

  QImage requestImage(const QString &id, QSize *size,
                      const QSize &requestedSize) override {
    QSize target = requestedSize.isValid() && !requestedSize.isEmpty()
                       ? requestedSize
                       : kDefaultIconSize;

    QString name = id.section('/', 0, 0);
    QString color = id.section('/', 1, 1);

    QImage image;
    if (name.startsWith("account-")) {
      int kind = name.mid(8).toInt();
      QIcon icon = Account::icon(static_cast<Account::Kind>(kind));
      image = icon.pixmap(target).toImage();
    } else {
      QFile file(QString(":/qml/icons/%1.svg").arg(name));
      if (file.open(QIODevice::ReadOnly)) {
        QByteArray svg = file.readAll();
        QColor tint(color.length() == 8 ? "#" + color.right(6) : "#" + color);
        if (tint.isValid())
          svg.replace("currentColor", tint.name(QColor::HexRgb).toUtf8());

        QBuffer buffer(&svg);
        QImageReader reader(&buffer, "svg");
        reader.setScaledSize(target);
        image = reader.read();
      }
    }

    if (size)
      *size = image.size();
    return image;
  }
};

} // namespace

namespace {

// Tool tips aren't shown while a menu is open. Their items still count as
// hovered below the menu.
int sOpenMenus = 0;

class MenuScope {
public:
  MenuScope() { ++sOpenMenus; }
  ~MenuScope() { --sOpenMenus; }
};

} // namespace

QmlHost::QmlHost(QQuickWidget *view) : QObject(view), mView(view) {
  view->installEventFilter(this);
}

bool QmlHost::eventFilter(QObject *watched, QEvent *event) {
  // Clicking or scrolling hides the tool tip, like native tool tips.
  switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::FocusOut:
      hideToolTip();
      break;

    default:
      break;
  }

  return QObject::eventFilter(watched, event);
}

void QmlHost::showToolTip(const QString &text, qreal x, qreal y, qreal width,
                          qreal height) {
  if (sOpenMenus > 0)
    return;

  if (QmlSupport::drawsPopups(mView)) {
    mToolTipText = text;
    mToolTipRect = QRectF(x, y, width, height);
    mToolTipVisible = !text.isEmpty();
    emit toolTipChanged();
    return;
  }

  QRect rect = QRectF(x, y, width, height).toAlignedRect();
  QPoint pos = mView->mapToGlobal(QPoint(rect.left(), rect.bottom() + 4));
  QToolTip::showText(pos, text, mView, rect);
}

void QmlHost::hideToolTip() {
  if (mToolTipVisible) {
    mToolTipVisible = false;
    emit toolTipChanged();
  }

  QToolTip::hideText();
}

void QmlHost::showEditMenu(QQuickItem *field) {
  if (!field)
    return;

  QMenu menu;
  QmlSupport::addEditActions(&menu, field);
  QmlSupport::execMenu(&menu, QCursor::pos());
}

QPoint QmlHost::mapToGlobal(qreal x, qreal y) const {
  return mView->mapToGlobal(QPointF(x, y).toPoint());
}

void QmlHost::popup(QMenu *menu, qreal x, qreal y) const {
  QmlSupport::execMenu(menu, mapToGlobal(x, y));
}

namespace {

// Serves "image://images/<key>" from the images added to the store.
QHash<QString, QImage> sImages;
int sNextImage = 0;

class ImageProvider : public QQuickImageProvider {
public:
  ImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

  QImage requestImage(const QString &id, QSize *size,
                      const QSize &requestedSize) override {
    QImage image = sImages.value(id);
    if (size)
      *size = image.size();
    if (!image.isNull() && requestedSize.isValid() && !requestedSize.isEmpty())
      image = image.scaled(requestedSize, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);
    return image;
  }
};

} // namespace

namespace QmlSupport {

QQuickWidget *createView(const QString &name, const QVariantMap &context,
                         QWidget *parent) {
  static bool registered = false;
  if (!registered) {
    registered = true;
    qmlRegisterSingletonType<QmlTheme>(
        "Gittyup", 1, 0, "Theme", [](QQmlEngine *, QJSEngine *) -> QObject * {
          QmlTheme *theme = QmlTheme::instance();
          QQmlEngine::setObjectOwnership(theme, QQmlEngine::CppOwnership);
          return theme;
        });
    qmlRegisterType<CommitGraphItem>("Gittyup", 1, 0, "CommitGraph");
  }

  QQuickWidget *view = new QQuickWidget(parent);
  view->setResizeMode(QQuickWidget::SizeRootObjectToView);
  view->setClearColor(QmlTheme::instance()->toolbar());
  view->engine()->addImageProvider("icons", new IconProvider);
  view->engine()->addImageProvider("images", new ImageProvider);

  QQmlContext *root = view->rootContext();
  root->setContextProperty("host", new QmlHost(view));
  for (auto it = context.cbegin(); it != context.cend(); ++it)
    root->setContextProperty(it.key(), it.value());

  view->setSource(QUrl(QString("qrc:/qml/%1.qml").arg(name)));
  if (view->status() == QQuickWidget::Error) {
    for (const QQmlError &error : view->errors())
      qWarning("%s", qPrintable(error.toString()));
  }

  return view;
}

QmlHost *host(QQuickWidget *view) { return view->findChild<QmlHost *>(); }

QString addImage(const QImage &image) {
  QString key = QString::number(sNextImage++);
  sImages.insert(key, image);
  return QString("image://images/%1").arg(key);
}

void removeImage(const QString &url) {
  sImages.remove(url.section('/', -1));
}

void setDrawsPopups(QQuickWidget *view, bool draws) {
  view->setProperty("drawsPopups", draws);
}

bool drawsPopups(QQuickWidget *view) {
  return view->property("drawsPopups").toBool();
}

// The view that draws popups at 'pos' on the screen, preferring the active
// window. Windows can be partly outside of the screens.
static QQuickWidget *popupViewAt(const QPoint &pos) {
  QQuickWidget *found = nullptr;
  for (QWidget *window : QApplication::topLevelWidgets()) {
    if (!window->isVisible())
      continue;

    for (QQuickWidget *view : window->findChildren<QQuickWidget *>()) {
      QRect rect(view->mapToGlobal(QPoint()), view->size());
      if (!view->isVisible() || !drawsPopups(view) || !rect.contains(pos))
        continue;

      if (window->isActiveWindow())
        return view;
      if (!found)
        found = view;
    }
  }

  return found;
}

QAction *execMenu(QMenu *menu, const QPoint &pos) {
  QQuickWidget *view = popupViewAt(pos);
  if (!view || !view->rootObject()) {
    QToolTip::hideText();
    MenuScope scope;
    return menu->exec(pos);
  }

  if (QmlHost *host = QmlSupport::host(view))
    host->hideToolTip();

  // Describe the visible actions of the menu and its submenus. Menus that
  // add their actions when they are about to be shown do that now.
  QList<QAction *> actions;
  QList<QMenu *> menus;
  std::function<QVariantList(QMenu *)> describe = [&](QMenu *menu) {
    emit menu->aboutToShow();
    menus.append(menu);

    QVariantList items;
    for (QAction *action : menu->actions()) {
      if (!action->isVisible())
        continue;

      if (action->isSeparator()) {
        // Skip leading and repeated separators, like native menus.
        if (!items.isEmpty() && !items.last().toMap().value("separator").toBool())
          items.append(QVariantMap{{"separator", true}});
        continue;
      }

      // Remove the markers of mnemonics.
      QString text = action->text();
      text.replace("&&", QChar(0x1));
      text.remove('&');
      text.replace(QChar(0x1), '&');

      QVariantMap item{
          {"text", text},
          {"shortcut", action->shortcut().toString(QKeySequence::NativeText)},
          {"enabled", action->isEnabled()},
          {"checkable", action->isCheckable()},
          {"checked", action->isChecked()}};
      if (QMenu *submenu = action->menu()) {
        item.insert("submenu", describe(submenu));
      } else {
        item.insert("id", actions.size());
        actions.append(action);
      }

      items.append(item);
    }

    if (!items.isEmpty() && items.last().toMap().value("separator").toBool())
      items.removeLast();

    return items;
  };

  QVariantList items = describe(menu);
  if (items.isEmpty())
    return nullptr;

  QQmlComponent component(view->engine(),
                          QUrl("qrc:/qml/ContextMenu.qml"));
  QPoint local = view->mapFromGlobal(pos);
  QObject *popup = component.createWithInitialProperties(
      {{"items", items},
       {"parent", QVariant::fromValue(view->rootObject())},
       {"x", local.x()},
       {"y", local.y()}},
      view->rootContext());
  if (!popup) {
    for (const QQmlError &error : component.errors())
      qWarning("%s", qPrintable(error.toString()));
    MenuScope scope;
    return menu->exec(pos);
  }

  // The item that was clicked to show the menu keeps the mouse until its
  // click handler returns, after the menu closes. Release it, or the
  // entries of the menu aren't hovered until the menu is clicked.
  if (QQuickItem *grabber = view->quickWindow()->mouseGrabberItem())
    grabber->ungrabMouse();

  // Wait until the menu closes, like a native menu.
  QEventLoop loop;
  QObject::connect(popup, SIGNAL(closed()), &loop, SLOT(quit()));
  MenuScope scope;
  QMetaObject::invokeMethod(popup, "open");
  loop.exec();
  int chosen = popup->property("chosen").toInt();
  popup->deleteLater();

  for (QMenu *shown : menus)
    emit shown->aboutToHide();

  if (chosen < 0 || chosen >= actions.size())
    return nullptr;

  // Trigger the action like the menu would. QMenu emits triggered() for it
  // and its parent menus.
  QAction *action = actions.at(chosen);
  action->activate(QAction::Trigger);

  return action;
}

void addEditActions(QMenu *menu, QQuickItem *field) {
  // The menu takes the focus, and the field loses its selection with it.
  // Give both back before acting on the field.
  QPointer<QQuickItem> target(field);
  int start = field->property("selectionStart").toInt();
  int end = field->property("selectionEnd").toInt();
  bool selected = start != end;
  bool editable = !field->property("readOnly").toBool();
  // Passwords aren't copied. TextEdit has no echo mode.
  bool visible = field->property("echoMode").toInt() == 0;

  auto add = [menu, target, start,
              end](const QString &text, QKeySequence::StandardKey key,
                   bool enabled, std::function<void(QQuickItem *)> act) {
    QAction *action = menu->addAction(text);
    action->setShortcut(key);
    action->setEnabled(enabled);
    QObject::connect(action, &QAction::triggered, [target, start, end, act] {
      if (!target)
        return;

      target->forceActiveFocus();
      QMetaObject::invokeMethod(target, "select", Q_ARG(int, start),
                                Q_ARG(int, end));
      act(target);
    });
  };

  auto invoke = [](const char *method) {
    return
        [method](QQuickItem *item) { QMetaObject::invokeMethod(item, method); };
  };

  add(QMenu::tr("Undo"), QKeySequence::Undo,
      editable && field->property("canUndo").toBool(), invoke("undo"));
  add(QMenu::tr("Redo"), QKeySequence::Redo,
      editable && field->property("canRedo").toBool(), invoke("redo"));
  menu->addSeparator();
  add(QMenu::tr("Cut"), QKeySequence::Cut, editable && selected && visible,
      invoke("cut"));
  add(QMenu::tr("Copy"), QKeySequence::Copy, selected && visible,
      invoke("copy"));
  add(QMenu::tr("Paste"), QKeySequence::Paste,
      editable && field->property("canPaste").toBool(), invoke("paste"));
  add(QMenu::tr("Delete"), QKeySequence::Delete, editable && selected,
      [start, end](QQuickItem *item) {
        QMetaObject::invokeMethod(item, "remove", Q_ARG(int, start),
                                  Q_ARG(int, end));
      });
  menu->addSeparator();
  add(QMenu::tr("Select All"), QKeySequence::SelectAll,
      !field->property("text").toString().isEmpty(), invoke("selectAll"));
}

} // namespace QmlSupport
