//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include <QObject>
#include <QVariantList>

class MainWindow;
class QWidget;

// Controller of the page shown when no repository is open or a new tab is
// requested. qrc:/qml/WelcomePage.qml draws it.
class WelcomePage : public QObject {
  Q_OBJECT

  Q_PROPERTY(QVariantList recent READ recent NOTIFY recentChanged)
  Q_PROPERTY(QVariantList accounts READ accounts CONSTANT)
  Q_PROPERTY(bool closable READ isClosable NOTIFY closableChanged)

public:
  // Dialogs are opened on 'widget', and repositories in its main window.
  WelcomePage(QWidget *widget);

  QVariantList recent() const { return mRecent; }
  QVariantList accounts() const;

  bool isClosable() const { return mClosable; }
  void setClosable(bool closable);

  Q_INVOKABLE void openRepository();
  Q_INVOKABLE void cloneRepository();
  Q_INVOKABLE void initRepository();
  Q_INVOKABLE void openRecent(const QString &path);
  Q_INVOKABLE void removeRecent(const QString &path);
  Q_INVOKABLE void addAccount(int kind);
  Q_INVOKABLE void openSupport();
  Q_INVOKABLE void close();

signals:
  void recentChanged();
  void closableChanged();
  void closeRequested();

private:
  void updateRecent();
  void open(const QString &path, const QString &message = QString(),
            const QString &title = QString());

  QWidget *mWidget;
  QVariantList mRecent;
  bool mClosable = false;
};

#endif
