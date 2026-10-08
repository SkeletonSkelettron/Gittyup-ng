//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DownloadDialog.h"
#include <QCoreApplication>
#include <QNetworkReply>

DownloadDialog::DownloadDialog(const Updater::DownloadRef &download,
                               QWidget *parent)
    : QmlDialog(parent),
      mText(tr("Downloading %1...").arg(download->name())) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Update %1").arg(QCoreApplication::applicationName()));

  QNetworkReply *reply = download->reply();
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    // The download was aborted.
    if (reply->error() != QNetworkReply::NoError || !reply->isOpen()) {
      close();
      return;
    }

    // Adjust interface for installation.
    mText = tr("Download Complete!");
    mProgress = 1;
    mComplete = true;
    emit changed();

    // Now connect reject to the cancel signal.
    connect(this, &DownloadDialog::rejected, Updater::instance(),
            &Updater::updateCanceled);
  });

  // Handle reject and accept.
  connect(this, &DownloadDialog::rejected, reply, &QNetworkReply::abort);
  connect(this, &DownloadDialog::accepted,
          [download] { Updater::instance()->install(download); });

  // Show download progress.
  connect(reply, &QNetworkReply::downloadProgress, this,
          [this](qint64 current, qint64 total) {
            mProgress = total > 0 ? qreal(current) / total : 0;
            emit changed();
          });

  setContent("DownloadDialog");
}
