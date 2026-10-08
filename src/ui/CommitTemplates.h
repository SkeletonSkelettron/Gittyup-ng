#ifndef COMMITTEMPLATES_H
#define COMMITTEMPLATES_H

#include <QList>
#include <QObject>
#include <QPoint>

class QWidget;

// The commit message templates, saved in the settings.
class CommitTemplates : public QObject {
  Q_OBJECT
public:
  struct Template {
    QString name{""};
    QString value{""};
  };
  static const QString cursorPositionString;
  static const QString filesPosition;

  CommitTemplates(QObject *parent = nullptr);

  const QList<Template> &templates() const { return mTemplates; }

  // Show the menu of templates and the action to configure them at 'pos'.
  void showMenu(const QPoint &pos, QWidget *parent);

signals:
  void templateChanged(const QString &str);

private:
  void storeTemplates();
  QList<Template> loadTemplates();

  QList<Template> mTemplates;
};

#endif
