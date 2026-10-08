//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "CommandPalette.h"
#include "MainWindow.h"
#include "MenuBar.h"
#include "RepoView.h"
#include "conf/RecentRepositories.h"
#include "conf/RecentRepository.h"
#include "git/Commit.h"
#include "git/Reference.h"
#include "git/Tree.h"
#include "Location.h"
#include <QAction>
#include <QFileInfo>
#include <QMenu>
#include <QTimer>

namespace {

// Results are limited to this many.
const int kMaxResults = 100;
// Repositories with more files only list this many.
const int kMaxFiles = 200000;

const QString kSeparator = QString::fromUtf8(" › ");

bool isBoundary(const QString &text, int pos) {
  if (pos == 0)
    return true;

  QChar prev = text.at(pos - 1);
  QChar ch = text.at(pos);
  return prev.isSpace() || prev == '/' || prev == '_' || prev == '-' ||
         prev == '.' || prev == ':' || prev == QChar(0x203A) ||
         (prev.isLower() && ch.isUpper());
}

// Match the rest of the pattern greedily from 'start'.
int matchFrom(const QString &pattern, const QString &text, int start,
              QList<int> *positions) {
  int score = 0;
  int prev = -1;
  int pos = start;
  for (int i = 0; i < pattern.size(); ++i) {
    QChar ch = pattern.at(i);
    while (pos < text.size() && text.at(pos).toLower() != ch)
      ++pos;
    if (pos >= text.size())
      return -1;

    score += 1;
    if (isBoundary(text, pos))
      score += 8;
    if (prev >= 0) {
      if (pos == prev + 1) {
        score += 6;
      } else {
        score -= qMin(pos - prev - 1, 6);
      }
    }

    positions->append(pos);
    prev = pos;
    ++pos;
  }

  return score;
}

QString icon(CommandPalette::Kind kind) {
  switch (kind) {
    case CommandPalette::Command:
      return "keyboard";
    case CommandPalette::Branch:
      return "branch";
    case CommandPalette::Tag:
      return "tag";
    case CommandPalette::File:
      return "file";
    case CommandPalette::Repository:
      return "repo";
  }

  return QString();
}

} // namespace

CommandPalette::CommandPalette(MainWindow *window)
    : QAbstractListModel(window), mWindow(window) {}

void CommandPalette::setQuery(const QString &query) {
  if (mQuery == query)
    return;

  mQuery = query;
  emit queryChanged();
  filter();
}

void CommandPalette::open(const QString &query) {
  collect();
  mQuery = query;
  emit queryChanged();
  filter();

  if (!mVisible) {
    mVisible = true;
    emit visibleChanged();
  }
}

void CommandPalette::close() {
  if (!mVisible)
    return;

  mVisible = false;
  emit visibleChanged();
}

void CommandPalette::activate(int row) {
  if (row < 0 || row >= mMatches.size())
    return;

  Item item = mItems.at(mMatches.at(row).item);
  close();

  // Run it when the palette is gone, so that dialogs get the focus.
  MainWindow *window = mWindow;
  QTimer::singleShot(0, window, [window, item] {
    RepoView *view = window->currentView();
    switch (item.kind) {
      case Command:
        if (item.action && item.action->isEnabled())
          item.action->trigger();
        break;

      case Branch: {
        if (!view)
          break;
        git::Reference ref = view->repo().lookupRef(item.data);
        if (!ref.isValid())
          break;
        if (ref.isHead()) {
          view->selectReference(ref);
        } else {
          view->checkout(ref);
        }
        break;
      }

      case Tag:
        if (view) {
          git::Reference ref = view->repo().lookupRef(item.data);
          if (ref.isValid())
            view->selectReference(ref);
        }
        break;

      case File:
        if (view) {
          // Show the file of HEAD with its blame.
          git::Reference head = view->repo().head();
          if (head.isValid())
            view->setLocation(Location(RepoView::Tree, head.qualifiedName(),
                                       head.target().id().toString(),
                                       item.data));
        }
        break;

      case Repository:
        MainWindow::open(item.data);
        break;
    }
  });
}

int CommandPalette::score(const QString &pattern, const QString &text,
                          QList<int> *positions) {
  QString needle;
  for (QChar ch : pattern) {
    if (!ch.isSpace())
      needle.append(ch.toLower());
  }

  if (needle.isEmpty())
    return 0;

  // Try each place where the first character matches, and keep the best.
  int best = -1;
  QList<int> bestPositions;
  QChar first = needle.at(0);
  for (int start = 0; start < text.size(); ++start) {
    if (text.at(start).toLower() != first)
      continue;

    QList<int> current;
    int score = matchFrom(needle, text, start, &current);
    if (score < 0)
      break;

    if (score > best) {
      best = score;
      bestPositions = current;
    }
  }

  if (best < 0)
    return -1;

  // Prefer text that contains the pattern, and more so at its start.
  int index = text.indexOf(pattern.trimmed(), 0, Qt::CaseInsensitive);
  if (index == 0) {
    best += 15;
  } else if (index > 0) {
    best += 10;
  }

  if (positions)
    *positions = bestPositions;
  return best;
}

int CommandPalette::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : mMatches.size();
}

QVariant CommandPalette::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= mMatches.size())
    return QVariant();

  const Match &match = mMatches.at(index.row());
  const Item &item = mItems.at(match.item);
  switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
      return item.title;

    case HtmlRole: {
      // The title is at the end of the matched text.
      int offset = item.key.length() - item.title.length();
      QString html;
      for (int i = 0; i < item.title.length(); ++i) {
        QString ch = QString(item.title.at(i)).toHtmlEscaped();
        html += match.positions.contains(offset + i) ? "<b>" + ch + "</b>"
                                                      : ch;
      }
      return html;
    }

    case DetailRole:
      return item.detail;
    case ShortcutRole:
      return item.shortcut;
    case KindRole:
      return item.kind;
    case IconRole:
      return icon(item.kind);
  }

  return QVariant();
}

QHash<int, QByteArray> CommandPalette::roleNames() const {
  return {{TitleRole, "title"},       {HtmlRole, "html"},
          {DetailRole, "detail"},     {ShortcutRole, "shortcut"},
          {KindRole, "kind"},         {IconRole, "icon"}};
}

void CommandPalette::collect() {
  mItems.clear();

  // The commands of the menu bar that can be run now.
  if (MenuBar *menuBar = MenuBar::instance(mWindow)) {
    menuBar->update();
    for (QMenu *menu : menuBar->menus())
      addCommands(menu, menu->title().remove('&'));
  }

  // Branches and tags.
  RepoView *view = mWindow->currentView();
  git::Repository repo = view ? view->repo() : git::Repository();
  if (repo.isValid()) {
    for (const git::Reference &ref : repo.refs()) {
      Item item;
      if (ref.isLocalBranch()) {
        item.kind = Branch;
        item.detail = ref.isHead() ? tr("Current branch") : tr("Checkout");
      } else if (ref.isRemoteBranch() && !ref.name().endsWith("/HEAD")) {
        item.kind = Branch;
        item.detail = tr("Checkout remote branch");
      } else if (ref.isTag()) {
        item.kind = Tag;
        item.detail = tr("Show tag");
      } else {
        continue;
      }

      item.title = ref.name();
      item.key = item.title;
      item.data = ref.qualifiedName();
      mItems.append(item);
    }
  }

  // The files of HEAD, listed again when it changes.
  git::Reference head = repo.isValid() ? repo.head() : git::Reference();
  git::Commit commit = head.isValid() ? head.target() : git::Commit();
  git::Tree tree = commit.isValid() ? commit.tree() : git::Tree();
  QString repoPath = repo.isValid() ? repo.dir().path() : QString();
  git::Id treeId = commit.isValid() ? commit.id() : git::Id();
  if (treeId != mFilesTree || repoPath != mFilesRepo) {
    mFiles.clear();
    mFilesTree = treeId;
    mFilesRepo = repoPath;

    std::function<void(const git::Tree &, const QString &)> walk;
    walk = [this, &walk](const git::Tree &tree, const QString &prefix) {
      for (int i = 0; i < tree.count() && mFiles.size() < kMaxFiles; ++i) {
        git::Object object = tree.object(i);
        QString name = tree.name(i);
        if (!object.isValid())
          continue;

        if (object.type() == GIT_OBJECT_TREE) {
          walk(git::Tree(object), prefix + name + "/");
        } else if (object.type() == GIT_OBJECT_BLOB) {
          Item item;
          item.kind = File;
          item.title = name;
          item.detail = prefix.isEmpty() ? QString("/") : prefix.chopped(1);
          item.key = prefix + name;
          item.data = item.key;
          mFiles.append(item);
        }
      }
    };

    if (tree.isValid())
      walk(tree, QString());
  }

  mItems.append(mFiles);

  // Recent repositories.
  RecentRepositories *recent = RecentRepositories::instance();
  for (int i = 0; i < recent->count(); ++i) {
    RecentRepository *repository = recent->repository(i);
    Item item;
    item.kind = Repository;
    item.title = repository->name();
    item.detail = repository->gitpath();
    item.key = item.title;
    item.data = repository->gitpath();
    mItems.append(item);
  }
}

void CommandPalette::addCommands(QMenu *menu, const QString &path) {
  // Menus fill themselves when they are about to show.
  emit menu->aboutToShow();

  for (QAction *action : menu->actions()) {
    if (action->isSeparator() || !action->isVisible())
      continue;

    QString text = action->text().remove('&');
    if (QMenu *submenu = action->menu()) {
      if (submenu->objectName() != "openRecent")
        addCommands(submenu, path + kSeparator + text);
      continue;
    }

    if (!action->isEnabled() || text.isEmpty() ||
        action->objectName() == "commandPalette")
      continue;

    Item item;
    item.kind = Command;
    item.title = text;
    item.detail = path;
    item.shortcut = action->shortcut().toString(QKeySequence::NativeText);
    item.key = path + kSeparator + text;
    item.action = action;
    item.menu = menu;
    mItems.append(item);
  }
}

void CommandPalette::filter() {
  QString query = mQuery.trimmed();
  QList<Kind> kinds;
  if (query.startsWith('>')) {
    kinds = {Command};
    query = query.mid(1).trimmed();
  } else if (query.startsWith('@')) {
    kinds = {Branch, Tag};
    query = query.mid(1).trimmed();
  }

  QList<Match> matches;
  for (int i = 0; i < mItems.size(); ++i) {
    const Item &item = mItems.at(i);
    if (!kinds.isEmpty() && !kinds.contains(item.kind))
      continue;

    // Without a query, suggest the commands and branches.
    if (query.isEmpty()) {
      if (item.kind == Command || item.kind == Branch)
        matches.append({i, 0, {}});
      continue;
    }

    QList<int> positions;
    int score = CommandPalette::score(query, item.key, &positions);
    if (score < 0)
      continue;

    // Prefer matches in the title, like the name of a file.
    int offset = item.key.length() - item.title.length();
    if (!positions.isEmpty() && positions.first() >= offset)
      score += 12;

    matches.append({i, score, positions});
  }

  // Suggestions keep the order of the menus.
  if (!query.isEmpty()) {
    std::stable_sort(matches.begin(), matches.end(),
                     [this](const Match &lhs, const Match &rhs) {
                       if (lhs.score != rhs.score)
                         return lhs.score > rhs.score;
                       const Item &l = mItems.at(lhs.item);
                       const Item &r = mItems.at(rhs.item);
                       if (l.kind != r.kind)
                         return l.kind < r.kind;
                       return l.key.length() < r.key.length();
                     });
  }

  if (matches.size() > kMaxResults)
    matches.erase(matches.begin() + kMaxResults, matches.end());

  beginResetModel();
  mMatches = matches;
  endResetModel();
}
