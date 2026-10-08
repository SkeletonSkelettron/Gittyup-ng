//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CloneDialog.h"
#include "git/Remote.h"
#include "git/Repository.h"
#include "git/Result.h"
#include "log/LogEntry.h"
#include "ui/LogPanel.h"
#include "ui/RemoteCallbacks.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>
#include <QtConcurrent>

namespace {

const QString kPathKey = "repo/path";

} // namespace

CloneDialog::CloneDialog(Kind kind, QWidget *parent, Repository *repo)
    : QmlDialog(parent), mInit(kind == Init), mRepo(repo) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(mInit ? tr("Initialize Repository") : tr("Clone Repository"));

  mDirectory = QSettings().value(kPathKey, QDir::homePath()).toString();
  if (mRepo)
    mUrl = mRepo->url(Repository::Https);
  if (mInit)
    mStep = LocationStep;

  mLogRoot = new LogEntry(this);
  mLogPanel = new LogPanel(mLogRoot, this);

  setContent("CloneDialog",
             {{"logPanel", QVariant::fromValue<QObject *>(mLogPanel)}});
}

CloneDialog::~CloneDialog() { cancel(); }

void CloneDialog::accept() {
  QString path = this->path();
  if (git::Repository::open(path).isValid() ||
      git::Repository::init(path, mBare).isValid()) {
    QSettings().setValue(kPathKey, mDirectory);
    QDialog::accept();
  }

  // FIXME: Report error.
}

void CloneDialog::reject() {
  cancel();
  QDialog::reject();
}

QString CloneDialog::path() const {
  return QDir(mDirectory.trimmed()).filePath(mName.trimmed());
}

QString CloneDialog::message() const {
  return mUrl.isEmpty()
             ? tr("Initialized empty repository into '%1'").arg(path())
             : tr("Cloned repository from '%1' into '%2'").arg(mUrl, path());
}

QString CloneDialog::messageTitle() const {
  return mUrl.isEmpty() ? tr("Initialize") : tr("Clone");
}

QVariant CloneDialog::field(const QString &name) const {
  if (name == "url")
    return mUrl;
  if (name == "name")
    return mName;
  if (name == "path")
    return mDirectory;
  if (name == "bare")
    return mBare;
  return QVariant();
}

void CloneDialog::setField(const QString &name, const QVariant &value) {
  if (name == "url")
    setUrl(value.toString());
  else if (name == "name")
    setName(value.toString());
  else if (name == "path")
    setDirectory(value.toString());
  else if (name == "bare")
    setBare(value.toString() == "true" || value.toBool());
}

void CloneDialog::setUrl(const QString &url) {
  if (url == mUrl)
    return;

  mUrl = url;
  emit changed();
}

void CloneDialog::setProtocol(int protocol) {
  if (!mRepo || protocol == mProtocol)
    return;

  mProtocol = protocol;
  setUrl(mRepo->url(static_cast<Repository::Protocol>(protocol)));
  emit changed();
}

void CloneDialog::setName(const QString &name) {
  if (name == mName)
    return;

  mName = name;
  emit changed();
}

void CloneDialog::setDirectory(const QString &directory) {
  if (directory == mDirectory)
    return;

  mDirectory = directory;
  emit changed();
}

void CloneDialog::setBare(bool bare) {
  if (bare == mBare)
    return;

  mBare = bare;
  emit changed();
}

bool CloneDialog::canContinue() const {
  switch (mStep) {
    case RemoteStep:
      return !mUrl.trimmed().isEmpty();
    case LocationStep:
      return !mName.trimmed().isEmpty() && !mDirectory.trimmed().isEmpty() &&
             QDir(mDirectory.trimmed()).exists();
    default:
      return false;
  }
}

void CloneDialog::next() {
  if (!canContinue())
    return;

  if (mStep == RemoteStep) {
    // Suggest the name from the URL.
    QString name = QFileInfo(QUrl(mUrl.trimmed()).path()).fileName();
    setName(name.endsWith(".git") ? name.chopped(4) : name);
    setStep(LocationStep);
  } else if (mStep == LocationStep) {
    if (mInit) {
      accept();
    } else {
      setStep(ProgressStep);
      startClone();
    }
  }
}

void CloneDialog::back() {
  if (mStep == ProgressStep) {
    cancel();
    setStep(LocationStep);
  } else if (mStep == LocationStep && !mInit) {
    setStep(RemoteStep);
  }
}

void CloneDialog::browseUrl() {
  QFileDialog *dialog =
      new QFileDialog(this, tr("Choose Directory"), mUrl, QString());
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setFileMode(QFileDialog::Directory);
  dialog->setOption(QFileDialog::ShowDirsOnly);
  connect(dialog, &QFileDialog::fileSelected, this, &CloneDialog::setUrl);
  dialog->open();
}

void CloneDialog::browseDirectory() {
  QFileDialog *dialog =
      new QFileDialog(this, tr("Choose Directory"), mDirectory, QString());
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setFileMode(QFileDialog::Directory);
  dialog->setOption(QFileDialog::ShowDirsOnly);
  connect(dialog, &QFileDialog::fileSelected, this,
          &CloneDialog::setDirectory);
  dialog->open();
}

void CloneDialog::startClone() {
  if (mWatcher)
    return;

  QString url = mUrl.trimmed();
  QString path = this->path();
  LogEntry *entry = mLogRoot->addEntry(url, tr("Clone"));

  mFailed = false;
  mWatcher = new QFutureWatcher<git::Result>(this);
  connect(mWatcher, &QFutureWatcher<git::Result>::finished, mWatcher,
          [this, path, entry] {
            entry->setBusy(false);

            git::Result result = mWatcher->result();
            bool success = false;
            if (mCallbacks->isCanceled()) {
              error(entry, tr("clone"), path, tr("Clone canceled."));
            } else if (!result) {
              error(entry, tr("clone"), path, result.errorString());
            } else {
              mCallbacks->storeDeferredCredentials();
              success = true;
            }

            mWatcher->deleteLater();
            mWatcher = nullptr;
            mCallbacks = nullptr;
            mFailed = !success;
            emit busyChanged();

            if (success)
              accept();
          });

  mCallbacks = new RemoteCallbacks(RemoteCallbacks::Receive, entry, url,
                                   "origin", mWatcher);

  entry->setBusy(true);
  mWatcher->setFuture(
      QtConcurrent::run(&git::Remote::clone, mCallbacks, url, path, mBare));
  emit busyChanged();
}

void CloneDialog::setStep(int step) {
  if (step == mStep)
    return;

  mStep = step;
  emit stepChanged();
  emit changed();
}

void CloneDialog::cancel() {
  // Signal the asynchronous transfer to cancel itself and wait for it.
  if (mWatcher && mWatcher->isRunning()) {
    mCallbacks->setCanceled(true);
    mWatcher->waitForFinished();
  }
}

void CloneDialog::error(LogEntry *entry, const QString &action,
                        const QString &name, const QString &defaultError) {
  QString text = tr("Failed to %1 into '%2' - %3");
  QString detail = git::Repository::lastError(defaultError);
  entry->addEntry(LogEntry::Error, text.arg(action, name, detail));
}
