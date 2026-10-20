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

// Called when a status starts. It reads the workdir after the changes that
// were notified so far, so they don't need another refresh.
void RepositoryWatcher::cancelPendingNotification() {
  mTimer.stop();

  // Drop the notifications that are already queued too. They come from
  // changes made while the main thread was busy, mostly by an operation that
  // ran on it, like a merge, and another refresh would restart the status.
  // Queued calls run in order: all of them run before this one. Changes made
  // while the status runs are notified after it, and refresh again.
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
