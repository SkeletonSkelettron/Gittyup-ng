//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "qtsupport.h"
#include "RemoteCallbacks.h"
#include "git/Id.h"
#include "git/RevWalk.h"
#include "log/LogEntry.h"
#include <QRegularExpression>
#include <QTextStream>
#include <QUrl>
#include <QUrlQuery>

namespace {

const int kKb = 1 << 10;
const int kMb = 1 << 20;
const int kGb = 1 << 30;

const QString kSizeFmt = "%1 %2";
const QString kRangeFmt = "%1..%2";
const QString kFromToFmt = "%1 -> %2";
const QString kUpdateFmt = " %1 %2 %3";
const QString kLinkFmt = "<a href='%1'>%2</a>";

QString size(size_t bytes) {
  if (bytes < kKb)
    return kSizeFmt.arg(bytes).arg("bytes");

  int i = 0;
  QString s;
  if (bytes < kMb) {
    i = kKb;
    s = "KiB";
  } else if (bytes < kGb) {
    i = kMb;
    s = "MiB";
  } else {
    i = kGb;
    s = "GiB";
  }

  return kSizeFmt.arg(static_cast<float>(bytes) / i, 0, 'f', 2).arg(s);
}

QString range(const QString &name, const QString &a, const QString &b) {
  QUrl url;
  url.setScheme("id");
  url.setPath(kRangeFmt.arg(a, b));

  QUrlQuery query;
  query.addQueryItem("ref", name);
  url.setQuery(query);

  return kLinkFmt.arg(url.toString(), kRangeFmt.arg(a.left(7), b.left(7)));
}

void setText(const QString &text, LogEntry *log, LogEntry *&entry) {
  if (entry) {
    entry->setText(text);
    return;
  }

  entry = log->addEntry(text);
}

} // namespace

RemoteCallbacks::RemoteCallbacks(Kind kind, LogEntry *log, const QString &url,
                                 const QString &name, QObject *parent,
                                 const git::Repository &repo)
    : QObject(parent), git::Remote::Callbacks(url, repo), mKind(kind),
      mLog(log), mName(name) {
  QObject::connect(this, &RemoteCallbacks::queueSideband, this,
                   &RemoteCallbacks::sidebandImpl);
  QObject::connect(this, &RemoteCallbacks::queueTransfer, this,
                   &RemoteCallbacks::transferImpl);
  QObject::connect(this, &RemoteCallbacks::queueUpdate, this,
                   &RemoteCallbacks::updateImpl);
  QObject::connect(this, &RemoteCallbacks::queueRejected, this,
                   &RemoteCallbacks::rejectedImpl);

  mTimer.start();
}

void RemoteCallbacks::setCanceled(bool canceled) {
  mCanceled = canceled;
}

void RemoteCallbacks::sideband(const QString &text) {
  emit queueSideband(text, tr("remote: %1"));
}

bool RemoteCallbacks::transfer(int total, int current, size_t bytes) {
  int elapsed = mTimer.elapsed();
  if (current == 0 || current == total || elapsed > 100) {
    emit queueTransfer(total, current, bytes, elapsed);
    mTimer.restart();
  }

  return !mCanceled;
}

void RemoteCallbacks::update(const QString &name, const git::Id &a,
                             const git::Id &b) {
  emit queueUpdate(name, a, b);
}

void RemoteCallbacks::rejected(const QString &name, const QString &status) {
  emit queueRejected(name, status);
}

void RemoteCallbacks::sidebandImpl(const QString &text, const QString &fmt) {
  // Build up sideband text until carriage return or newline.
  // Carriage return overwrites existing entry. Newline emits new.
  mSideband.append(text);

  QRegularExpression re("\\r|\\n");
  int index = mSideband.indexOf(re);
  while (index >= 0) {
    QString line = mSideband.left(index);
    QString text = fmt.isEmpty() ? line : fmt.arg(line);
    if (mSidebandItem) {
      mSidebandItem->setText(text);
    } else {
      // Insert before the transfer item.
      const QList<LogEntry *> &entries = mLog->entries();
      mSidebandItem = !mTransferItem
                          ? mLog->addEntry(text)
                          : mLog->insertEntry(entries.indexOf(mTransferItem),
                                              LogEntry::Entry, text);
    }

    // Start new item.
    if (mSideband.at(index) == '\n')
      mSidebandItem = nullptr;

    mSideband.remove(0, index + 1);
    index = mSideband.indexOf(re);
  }
}

void RemoteCallbacks::transferImpl(int total, int current, size_t bytes,
                                   int elapsed) {
  // Change state to resolve.
  if (current == total)
    mState = Resolve;

  if (total == 0)
    return;

  // Calculate percent.
  int percent = 100 * (static_cast<float>(current) / total);

  // Calculate the transfer rate.
  float seconds = static_cast<float>(qMax(elapsed, 1)) / 1000;
  auto transferRate = (bytes - mBytesReceived) / seconds;
  mBytesReceived = bytes;

  // Write text.
  QString text;
  QTextStream stream(&text);
  stream << ((mKind == Receive) ? "Receiving" : "Writing")
         << " objects: " << percent << "% (" << current << "/" << total << "), "
         << size(bytes) << " | " << size(transferRate) << "/s"
         << ((mState != Transfer) ? ", done" : QString()) << ".";

  // Update the list item.
  setText(text, mLog, mTransferItem);
}

void RemoteCallbacks::updateImpl(const QString &name, const git::Id &a,
                                 const git::Id &b) {
  if (mState == Resolve) {
    // Write header.
    QString fmt = (mKind == Receive) ? tr("From %1") : tr("To %1");
    mLog->addEntry(fmt.arg(mUrl));
    mState = Update;
  }

  // Signal attached views.
  emit referenceUpdated(name);

  QChar flag(' ');
  QString astr = a.toString();
  QString bstr = b.toString();
  QString summary = range(name, astr, bstr);
  QString remoteName = name.section('/', 2);
  QString localName = remoteName.section('/', 1);
  QString fromTo = kFromToFmt.arg(localName, remoteName);

  QRegularExpression re("[^0]");
  if (!astr.contains(re)) {
    flag = '*';
    bool tag = (name.section('/', 1, 1) == "tags");
    summary = QString("[new %1]").arg(tag ? "tag" : "branch");
  } else if (!bstr.contains(re)) {
    flag = '-';
    summary = "[deleted]";
    fromTo = localName;
  } else if (mRepo.isValid()) {
    git::Commit lhs = mRepo.lookupCommit(a);
    git::Commit rhs = mRepo.lookupCommit(b);
    if (lhs.isValid() && rhs.isValid()) {
      git::Commit base = mRepo.mergeBase(lhs, rhs);
      if (!base.isValid() || base != lhs)
        fromTo.append(" (forced update)");
    }
  }

  // Write text.
  mLog->addEntry(kUpdateFmt.arg(flag, summary, fromTo));
}

void RemoteCallbacks::rejectedImpl(const QString &name, const QString &status) {
  if (mState == Resolve) {
    // Write header.
    mLog->addEntry(tr("To %1").arg(mUrl));
    mState = Update;
  }

  QString relativeName = name.section('/', 2);
  QString fromTo = kFromToFmt.arg(relativeName, relativeName);
  QString text = QString("[remote rejected] %1 (%2)").arg(fromTo, status);
  mLog->addEntry(LogEntry::Error, text);
}
