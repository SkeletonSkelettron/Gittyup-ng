//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include "git/Id.h"
#include <QAbstractListModel>
#include <QPointer>

class MainWindow;
class QAction;
class QMenu;

// The command palette of a window, like GitKraken's fuzzy finder. It finds
// the commands of the menu bar, the branches, tags and files of the current
// repository and the recent repositories by fuzzy matching. '>' limits the
// search to commands and '@' to branches and tags.
// qrc:/qml/CommandPalette.qml draws it as 'commandPalette'.
class CommandPalette : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

public:
  enum Kind { Command, Branch, Tag, File, Repository };

  enum Role {
    TitleRole = Qt::UserRole,
    // The title with the matched characters in bold.
    HtmlRole,
    DetailRole,
    ShortcutRole,
    KindRole,
    IconRole
  };

  CommandPalette(MainWindow *window);

  bool isVisible() const { return mVisible; }
  QString query() const { return mQuery; }
  void setQuery(const QString &query);

  Q_INVOKABLE void open(const QString &query = QString());
  Q_INVOKABLE void close();
  // Run the item of a row. The palette closes first.
  Q_INVOKABLE void activate(int row);

  // How well 'pattern' matches 'text' as a subsequence, ignoring case, or -1
  // when it doesn't. The matched positions of 'text' are added to
  // 'positions'.
  static int score(const QString &pattern, const QString &text,
                   QList<int> *positions = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

signals:
  void visibleChanged();
  void queryChanged();

private:
  struct Item {
    Kind kind;
    QString title;
    QString detail;
    QString shortcut;
    // The text that's matched; the title is at its end.
    QString key;
    QPointer<QAction> action;
    QPointer<QMenu> menu;
    // The qualified name of a reference, the path of a file or repository.
    QString data;
  };

  struct Match {
    int item;
    int score;
    QList<int> positions;
  };

  void collect();
  void addCommands(QMenu *menu, const QString &path);
  void filter();

  MainWindow *mWindow;
  bool mVisible = false;
  QString mQuery;
  QList<Item> mItems;
  QList<Match> mMatches;

  // The files of HEAD are listed again when it changes.
  git::Id mFilesTree;
  QString mFilesRepo;
  QList<Item> mFiles;
};

#endif
