//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef REFDROP_H
#define REFDROP_H

#include <QObject>
#include <QPointF>
#include <functional>

class RepoView;

namespace git {
class Reference;
}

// Branches dragged onto other branches or remotes in the references panel
// or the commit graph, like in GitKraken. Dropping shows a menu to merge,
// rebase, fast-forward or push. QML pages see it as 'refDrop'.
class RefDrop : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool active READ isActive NOTIFY changed)
  // The qualified name of the dragged reference.
  Q_PROPERTY(QString source READ source NOTIFY changed)
  Q_PROPERTY(QString label READ label NOTIFY changed)
  // The position of the pointer in the scene.
  Q_PROPERTY(qreal x READ x NOTIFY moved)
  Q_PROPERTY(qreal y READ y NOTIFY moved)

public:
  // An action of dropping a reference onto another.
  struct Choice {
    QString text;
    std::function<void()> run;
  };

  RefDrop(RepoView *view);

  bool isActive() const { return !mSource.isEmpty(); }
  QString source() const { return mSource; }
  QString label() const { return mLabel; }
  qreal x() const { return mPos.x(); }
  qreal y() const { return mPos.y(); }

  // Start dragging the reference 'name' at a position in the scene.
  Q_INVOKABLE void start(const QString &name, qreal x, qreal y);
  Q_INVOKABLE void move(qreal x, qreal y);
  // Drop the reference onto the target under the pointer, if any.
  Q_INVOKABLE void finish();
  Q_INVOKABLE void cancel();

  // Whether dropping the dragged reference onto 'target' does anything.
  // The target is the qualified name of a reference or "remote:<name>".
  Q_INVOKABLE bool accepts(const QString &target) const;
  // Called by the target under the pointer when the reference is dropped.
  Q_INVOKABLE void drop(const QString &target, qreal x, qreal y);

  // What dropping 'source' onto 'target' can do.
  QList<Choice> choices(const QString &source, const QString &target) const;

signals:
  void changed();
  void moved();
  // QML drops the dragged reference onto the target under the pointer.
  void released();

private:
  void addInteractive(QList<Choice> &choices, const QString &source,
                      const git::Reference &target) const;
  void showMenu(const QString &source, const QString &target,
                const QPointF &pos);

  RepoView *mView;
  QString mSource;
  QString mLabel;
  QPointF mPos;

  QString mTarget;
  QPointF mDropPos;
};

#endif
