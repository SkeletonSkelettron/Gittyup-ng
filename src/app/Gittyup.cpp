//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Application.h"
#include "git/Config.h"
#include "ui/MainWindow.h"
#include "ui/qml/QmlTheme.h"
#include "dialogs/ConfirmDialog.h"
#include <QEventLoop>
#include <QQmlContext>
#include <QQuickView>
#include <QScreen>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

// Shows a small animated window while the main windows load.
QQuickView *showSplash() {
  QQuickView *splash = new QQuickView;
  splash->setFlags(Qt::SplashScreen | Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint);
  splash->setColor(QmlTheme::instance()->panel());
  splash->rootContext()->setContextProperty("theme", QmlTheme::instance());
  splash->setSource(QUrl("qrc:/qml/Splash.qml"));
  if (splash->status() != QQuickView::Ready) {
    delete splash;
    return nullptr;
  }

  QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
  splash->setPosition(screen.center() -
                      QPoint(splash->width() / 2, splash->height() / 2));
  splash->show();

  // The main thread is busy from now on: let the first frame show. The render
  // thread animates it afterwards.
  QEventLoop loop;
  QObject::connect(splash, &QQuickWindow::frameSwapped, &loop,
                   &QEventLoop::quit);
  QTimer::singleShot(1000, &loop, &QEventLoop::quit);
  loop.exec();

  return splash;
}

} // namespace

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  // Windows replaces the windows of a busy main thread with frozen copies
  // after a few seconds, the splash screen too.
  DisableProcessWindowsGhosting();
#endif

  Application app(argc, argv, true);

  // Check if only one running instance is allowed and already running
  if (app.runSingleInstance())
    return 0;

  if (!git::Config::global().isValid()) {
    ConfirmDialog::warning(
        nullptr, GITTYUP_NAME,
        QObject::tr("Your global GIT configuration is invalid, %1 won't "
                    "run properly until this is fixed")
            .arg(GITTYUP_NAME));
  }

  QQuickView *splash = showSplash();

  // Restore windows before checking for updates so that
  // the update dialog pops up on top of the other windows.
  if (!app.restoreWindows())
    MainWindow::open();

  // Close the splash screen once the main windows have drawn.
  if (splash)
    QTimer::singleShot(0, splash, &QObject::deleteLater);

  // Check for updates.
  app.autoUpdate();

  return app.exec();
}
