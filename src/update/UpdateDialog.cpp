//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "UpdateDialog.h"
#include "DownloadDialog.h"
#include "Updater.h"
#include "conf/Settings.h"
#include "ui/MenuBar.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>

#if ((!defined(Q_OS_LINUX) || defined(FLATPAK) || defined(DEBUG_FLATPAK)) &&   \
     defined(ENABLE_UPDATE_OVER_GUI))
#define ENABLE_UPDATE 1
#else
#define ENABLE_UPDATE 0
#endif

UpdateDialog::UpdateDialog(const QString &platform, const QString &version,
                           const QString &changelog, const QString &link,
                           QWidget *parent)
    : QmlDialog(parent), mVersion(version), mChangelog(changelog) {
  QString appName = QCoreApplication::applicationName();
  QString appVersion = QCoreApplication::applicationVersion();

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Update %1").arg(appName));

#if !ENABLE_UPDATE
  mText = tr("%1 %2 is now available - you have %3. The new version will be "
             "soon available in your package manager. Just update your "
             "system.")
              .arg(appName, version, appVersion);
#elif !defined(Q_OS_LINUX)
  mText = tr("%1 %2 is now available - you have %3. Would you like to "
             "download it now?")
              .arg(appName, version, appVersion);
#elif defined(FLATPAK) || defined(DEBUG_FLATPAK)
  mText = tr("%1 %2 is now available - you have %3. If you installed the "
             "flatpak package from a package manager or from flathub.org, "
             "the new version will be available within the next days during "
             "your system update: flatpak update")
              .arg(appName, version, appVersion);
#else
  mText = tr("%1 %2 is now available - you have %3. The new version will be "
             "soon available in your package manager. Just update your "
             "system.")
              .arg(appName, version, appVersion);
#endif

#if ENABLE_UPDATE
  connect(this, &UpdateDialog::accepted, [link] {
    // Start download.
    if (Updater::DownloadRef download = Updater::instance()->download(link)) {
      DownloadDialog *dialog = new DownloadDialog(download);
      dialog->show();
    }
  });
#else
  Q_UNUSED(link)

  // Skip the version automatically, because the user can't update.
  skipVersion();
#endif

  setContent("UpdateDialog");
}

QString UpdateDialog::title() const {
  return tr("A new version of %1 is available!")
      .arg(QCoreApplication::applicationName());
}

bool UpdateDialog::isInstallable() const { return ENABLE_UPDATE; }

bool UpdateDialog::installAutomatically() const {
  return Settings::instance()
      ->value(Setting::Id::InstallUpdatesAutomatically)
      .toBool();
}

void UpdateDialog::setInstallAutomatically(bool install) {
  Settings::instance()->setValue(Setting::Id::InstallUpdatesAutomatically,
                                 install);
  emit installAutomaticallyChanged();
}

void UpdateDialog::skip() {
  skipVersion();
  reject();
}

void UpdateDialog::donate() {
  QDesktopServices::openUrl(QUrl(MenuBar::donationUrlLiberapay));
}

void UpdateDialog::skipVersion() {
  Settings *settings = Settings::instance();
  QStringList skipped =
      settings->value(Setting::Id::SkippedUpdates).toStringList();
  if (!skipped.contains(mVersion))
    settings->setValue(Setting::Id::SkippedUpdates, skipped << mVersion);
}
