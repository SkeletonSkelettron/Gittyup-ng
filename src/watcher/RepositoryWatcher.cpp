//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RepositoryWatcher.h"

void RepositoryWatcher::init(const git::Repository &repo) {
  // The timer has to run on the main thread.
  mTimer.setInterval(2000);
  mTimer.setSingleShot(true);
  connect(&mTimer, &QTimer::timeout, repo.notifier(),
          &git::RepositoryNotifier::workdirChanged);
}

void RepositoryWatcher::cancelPendingNotification() {
  mTimer.stop();

  // Drop the notifications that are already queued too. They come from
  // changes made while the main thread was busy, mostly by an operation that
  // ran on it, like a merge. The refresh that follows sees them anyway, and
  // another one would restart its status. Queued calls run in order: all of
  // them run before this one.
  ++mDroppedQueues;
  QMetaObject::invokeMethod(
      this, [this] { --mDroppedQueues; }, Qt::QueuedConnection);
}

// Refresh soon after the first change, even while changes keep coming.
void RepositoryWatcher::notifyChanged() {
  if (mDroppedQueues > 0)
    return;

  if (!mTimer.isActive()) {
    mTimer.start();
  }
}
