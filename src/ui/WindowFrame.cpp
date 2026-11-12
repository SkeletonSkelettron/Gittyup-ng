//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Claude Opus 5.5, an AI model by Anthropic
//

#include "WindowFrame.h"
#include "conf/Settings.h"
#include <QEvent>
#include <QWidget>
#include <QWindow>

WindowFrame::WindowFrame(QWidget *window) : QObject(window), mWindow(window) {
  window->installEventFilter(this);
  connect(window, &QWidget::windowTitleChanged, this,
          &WindowFrame::titleChanged);
  connect(Settings::instance(), &Settings::settingsChanged, this,
          &WindowFrame::update);
  update();
}

bool WindowFrame::isMaximized() const { return mWindow->isMaximized(); }

bool WindowFrame::isActive() const { return mWindow->isActiveWindow(); }

QString WindowFrame::title() const { return mWindow->windowTitle(); }

void WindowFrame::startMove() {
  if (QWindow *window = mWindow->windowHandle())
    window->startSystemMove();
}

void WindowFrame::startResize(int edges) {
  if (QWindow *window = mWindow->windowHandle())
    window->startSystemResize(Qt::Edges(edges));
}

void WindowFrame::minimize() { mWindow->showMinimized(); }

void WindowFrame::toggleMaximized() {
  if (mWindow->isMaximized()) {
    mWindow->showNormal();
  } else {
    mWindow->showMaximized();
  }
}

void WindowFrame::close() { mWindow->close(); }

bool WindowFrame::eventFilter(QObject *watched, QEvent *event) {
  switch (event->type()) {
    case QEvent::WindowStateChange:
      emit maximizedChanged();
      break;

    case QEvent::ActivationChange:
      emit activeChanged();
      break;

    default:
      break;
  }

  return QObject::eventFilter(watched, event);
}

void WindowFrame::update() {
#ifdef Q_OS_MACOS
  // The menus are in the menu bar of the system, so the title bar stays.
  bool custom = false;
#else
  bool custom =
      !Settings::instance()->value(Setting::Id::SystemTitleBar).toBool();
#endif

  if (custom == mCustom &&
      mWindow->windowFlags().testFlag(Qt::FramelessWindowHint) == custom)
    return;

  // Changing the flags of a window hides it.
  bool visible = mWindow->isVisible();
  mWindow->setWindowFlag(Qt::FramelessWindowHint, custom);
  if (visible)
    mWindow->show();

  mCustom = custom;
  emit customChanged();
}
