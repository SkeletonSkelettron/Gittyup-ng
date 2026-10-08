//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef UNDOHISTORY_H
#define UNDOHISTORY_H

#include "git/RefState.h"
#include <QObject>
#include <QPointer>
#include <QTimer>

class LogEntry;
class RepoView;

// The actions of a repository that can be undone and redone, like in
// GitKraken. The branches, tags and HEAD are recorded after each action that
// changes them, and restored to undo or redo it.
class UndoHistory : public QObject {
  Q_OBJECT

public:
  // How the index and the working directory follow HEAD when an action is
  // undone or redone.
  enum Mode {
    // Checkout the commit of HEAD without overwriting local changes.
    Checkout,
    // Keep the index and the working directory, like 'reset --soft'.
    Soft,
    // Reset the index but keep the working directory, like 'reset --mixed'.
    Mixed
  };

  UndoHistory(RepoView *view);

  // Start an action that the log describes with 'entry'. Changes before are
  // recorded as the previous action.
  void begin(LogEntry *entry);
  // Set how the current action changes the working directory.
  void setMode(Mode mode);

  bool canUndo() const;
  bool canRedo() const;
  QString undoText() const;
  QString redoText() const;

  bool undo();
  bool redo();

  // Record the changes since the last action now.
  void check();

signals:
  void changed();

private:
  struct Action {
    QString text;
    git::RefState before;
    git::RefState after;
    Mode mode = Checkout;
    // The message of a commit whose undo leaves its changes in the index.
    QString message;
  };

  bool apply(const Action &action, bool undo);
  QString describe(const git::RefState &before,
                   const git::RefState &after) const;

  RepoView *mView;
  git::RefState mState;
  QList<Action> mUndo;
  QList<Action> mRedo;

  QPointer<LogEntry> mEntry;
  Mode mMode = Checkout;
  bool mApplying = false;
  QTimer mTimer;
};

#endif
