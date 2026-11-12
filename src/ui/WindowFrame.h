//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Claude Opus 5.5, an AI model by Anthropic
//

#ifndef WINDOWFRAME_H
#define WINDOWFRAME_H

#include <QObject>

class QWidget;

// The frame of a window that its view draws, in the colors of the theme:
// the window has no frame of the system, and the view moves and resizes it
// through the window system. Setting::Id::SystemTitleBar brings back the
// frame of the system. The view reads it as 'windowFrame'.
class WindowFrame : public QObject {
  Q_OBJECT

  // The view draws the title bar and the edges of the window.
  Q_PROPERTY(bool custom READ isCustom NOTIFY customChanged)
  Q_PROPERTY(bool maximized READ isMaximized NOTIFY maximizedChanged)
  Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
  Q_PROPERTY(QString title READ title NOTIFY titleChanged)

public:
  explicit WindowFrame(QWidget *window);

  bool isCustom() const { return mCustom; }
  bool isMaximized() const;
  bool isActive() const;
  QString title() const;

  // Move or resize the window with the window system, while a mouse button
  // is pressed. 'edges' are Qt::Edges.
  Q_INVOKABLE void startMove();
  Q_INVOKABLE void startResize(int edges);

  Q_INVOKABLE void minimize();
  Q_INVOKABLE void toggleMaximized();
  Q_INVOKABLE void close();

signals:
  void customChanged();
  void maximizedChanged();
  void activeChanged();
  void titleChanged();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  void update();

  QWidget *mWindow;
  bool mCustom = false;
};

#endif
