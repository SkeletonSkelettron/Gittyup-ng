//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "AboutDialog.h"
#include "conf/Settings.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QLocale>
#include <QPointer>

namespace {

const QString kIssueTracker =
    QStringLiteral("https://github.com/Murmele/Gittyup/issues");

const QString kUrl =
    "https://stackoverflow.com/questions/tagged/gittyup?sort=frequent";

const QString kUrlMatrix = "https://matrix.to/#/#Gittyup:matrix.org";

const QStringList kDocuments = {"changelog.html", "acknowledgments.html",
                                "privacy.html"};

} // namespace

AboutDialog::AboutDialog(QWidget *parent) : QmlDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("About %1").arg(name()));
  setContent("AboutDialog");
}

void AboutDialog::openSharedInstance(Index index) {
  static QPointer<AboutDialog> dialog;
  if (dialog) {
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
  } else {
    dialog = new AboutDialog;
    dialog->show();
  }

  dialog->setIndex(index);
}

QString AboutDialog::name() const {
  return QCoreApplication::applicationName();
}

QString AboutDialog::version() const {
  return QCoreApplication::applicationVersion();
}

QString AboutDialog::build() const {
  QDateTime dateTime = QDateTime::fromString(GITTYUP_BUILD_DATE, Qt::ISODate);
  QString date =
      dateTime.date().toString(QLocale().dateFormat(QLocale::LongFormat));
  return QString("%1 · %2").arg(date, GITTYUP_BUILD_REVISION);
}

QString AboutDialog::copyright() const {
  return tr("Copyright © 2021-%1 Gittyup contributors\n"
            "Copyright © 2016-2020 Scientific Toolworks, Inc. and "
            "contributors")
      .arg(CURR_YEAR);
}

QString AboutDialog::support() const {
  return tr("If you have a question that might benefit the community, "
            "consider asking it on <a href='%1'>Stack Overflow</a> by "
            "including 'gittyup' in the tags. Otherwise, contact us at "
            "<a href='%2'>GitHub</a> or ask in the "
            "<a href='%3'>matrix channel</a>.")
      .arg(kUrl, kIssueTracker, kUrlMatrix);
}

void AboutDialog::setIndex(int index) {
  if (index == mIndex || index < 0 || index >= kDocuments.size())
    return;

  mIndex = index;
  emit indexChanged();
}

QString AboutDialog::document() const {
  QFile file(Settings::docDir().filePath(kDocuments.value(mIndex)));
  if (!file.open(QIODevice::ReadOnly))
    return QString();

  return QString::fromUtf8(file.readAll());
}
