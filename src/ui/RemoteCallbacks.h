//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REMOTECALLBACKS_H
#define REMOTECALLBACKS_H

#include "git/Remote.h"
#include "git/Repository.h"
#include "host/Account.h"
#include <QElapsedTimer>
#include <QObject>
#include <QSet>
#include <atomic>

class LogEntry;

class RemoteCallbacks : public QObject, public git::Remote::Callbacks {
  Q_OBJECT

public:
  enum Kind { Send, Receive };

  RemoteCallbacks(Kind kind, LogEntry *log, const QString &url,
                  const QString &name = QString(), QObject *parent = nullptr,
                  const git::Repository &repo = git::Repository());

  bool isCanceled() const override { return mCanceled; }
  void setCanceled(bool canceled);

  void sideband(const QString &text) override;
  bool transfer(int total, int current, size_t bytes) override;
  void update(const QString &name, const git::Id &a, const git::Id &b) override;
  void rejected(const QString &name, const QString &status) override;

signals:
  void referenceUpdated(const QString &name);

  // These are implementation details.
  void queueSideband(const QString &text, const QString &fmt = QString());
  void queueTransfer(int total, int current, size_t bytes, int elapsed);
  void queueUpdate(const QString &name, const git::Id &a, const git::Id &b);
  void queueRejected(const QString &name, const QString &status);

private:
  void sidebandImpl(const QString &text, const QString &fmt = QString());
  void transferImpl(int total, int current, size_t bytes, int elapsed);
  void updateImpl(const QString &name, const git::Id &a, const git::Id &b);
  void rejectedImpl(const QString &name, const QString &status);

  Kind mKind;
  LogEntry *mLog;
  QString mName;

  QElapsedTimer mTimer;
  QString mSideband;
  size_t mBytesReceived = 0;
  std::atomic<bool> mCanceled{false};

  LogEntry *mSidebandItem = nullptr;
  LogEntry *mTransferItem = nullptr;
};

#endif
