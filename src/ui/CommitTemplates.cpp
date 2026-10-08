#include "CommitTemplates.h"
#include "qml/QmlSupport.h"

#include "TemplateDialog.h"

#include <QMenu>
#include <QSettings>

namespace {
const QString kTemplatesKey = "templates";
} // namespace

const QString CommitTemplates::cursorPositionString = QStringLiteral("%|");
// Showing the position where the files shall be placed
// The number regex dertermines how many filenames shall be shown and if
// there are more it will be replaced by ...
const QString CommitTemplates::filesPosition =
    QStringLiteral("${files:([0-9]*)}");

CommitTemplates::CommitTemplates(QObject *parent) : QObject(parent) {
  mTemplates = loadTemplates();
}

void CommitTemplates::showMenu(const QPoint &pos, QWidget *parent) {
  QMenu menu(parent);
  for (const Template &templ : mTemplates) {
    QString value = templ.value;
    menu.addAction(templ.name, this,
                   [this, value] { emit templateChanged(value); });
  }

  if (!mTemplates.isEmpty())
    menu.addSeparator();
  menu.addAction(tr("Configure Templates..."), this, [this, parent] {
    TemplateDialog dialog(mTemplates, parent);
    if (dialog.exec())
      storeTemplates();
  });

  QmlSupport::execMenu(&menu, pos);
}

void CommitTemplates::storeTemplates() {
  QSettings settings;
  settings.beginGroup(kTemplatesKey);

  // delete old templates
  QList<CommitTemplates::Template> old = loadTemplates();
  for (auto templ : old)
    settings.remove(templ.name);

  for (auto templ : mTemplates) {
    QString value = templ.value;
    value = value.replace("\n", "\\n");
    value = value.replace("\t", "\\t");
    settings.setValue(templ.name, value);
  }
  settings.endGroup();
}

QList<CommitTemplates::Template> CommitTemplates::loadTemplates() {
  QSettings settings;
  settings.beginGroup(kTemplatesKey);

  QStringList list = settings.allKeys();
  QList<CommitTemplates::Template> templates;

  for (auto templateName : list) {
    QString value = settings.value(templateName).toString();
    value = value.replace("\\n", "\n");
    value = value.replace("\\t", "\t");
    Template t;
    t.name = templateName;
    t.value = value;
    templates.append(t);
  }

  settings.endGroup();
  return templates;
}
