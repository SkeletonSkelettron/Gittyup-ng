//
//          Copyright (c) 2022, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "TemplateDialog.h"
#include <QFile>
#include <QFileDialog>
#include <QTextStream>

namespace {

const QString kTemplateFileExtension =
    QStringLiteral(".GittyupCommitMessageTemplate");

} // namespace

TemplateDialog::TemplateDialog(QList<CommitTemplates::Template> &templates,
                               QWidget *parent)
    : QmlDialog(parent), mTemplates(templates), mNew(templates) {
  setWindowTitle(tr("Commit Message Templates"));
  if (!mNew.isEmpty())
    showTemplate(0);

  setContent("TemplateDialog");
}

QStringList TemplateDialog::names() const {
  QStringList names;
  for (const CommitTemplates::Template &tmpl : mNew)
    names.append(tmpl.name);
  return names;
}

void TemplateDialog::setCurrent(int index) {
  if (index < 0 || index >= mNew.size() || index == mCurrent)
    return;

  showTemplate(index);
}

void TemplateDialog::setName(const QString &name) {
  if (name == mName)
    return;

  mName = name;
  emit editChanged();
}

void TemplateDialog::setTemplateText(const QString &text) {
  if (text == mTemplate)
    return;

  mTemplate = text;
  emit editChanged();
}

QString TemplateDialog::cursorHint() const {
  return tr("Use %1 to place the cursor and ${files:x} to add the names of "
            "up to x changed files.")
      .arg(CommitTemplates::cursorPositionString);
}

void TemplateDialog::addTemplate() {
  if (mName.isEmpty())
    return;

  // Replace the value of a template with the same name.
  for (int i = 0; i < mNew.count(); i++) {
    if (mNew[i].name == mName) {
      mNew[i].value = mTemplate;
      mCurrent = i;
      emit templatesChanged();
      return;
    }
  }

  CommitTemplates::Template tmpl;
  tmpl.name = mName;
  tmpl.value = mTemplate;
  mNew.append(tmpl);
  mCurrent = mNew.count() - 1;

  emit templatesChanged();
  emit editChanged();
}

void TemplateDialog::removeTemplate() {
  if (mCurrent < 0 || mCurrent >= mNew.count())
    return;

  mNew.removeAt(mCurrent);
  int index = qMin(mCurrent, static_cast<int>(mNew.count()) - 1);
  mCurrent = -1;
  emit templatesChanged();

  if (index >= 0)
    showTemplate(index);
}

void TemplateDialog::moveTemplateUp() { moveTemplate(-1); }

void TemplateDialog::moveTemplateDown() { moveTemplate(1); }

void TemplateDialog::moveTemplate(int offset) {
  int to = mCurrent + offset;
  if (mCurrent < 0 || to < 0 || to >= mNew.count())
    return;

  mNew.move(mCurrent, to);
  mCurrent = to;
  emit templatesChanged();
}

void TemplateDialog::importTemplates(QString filename) {
  if (filename.isEmpty()) {
    filename = QFileDialog::getOpenFileName(
        this, tr("Open File"), "/home",
        tr("Gittyup Templates (*%1)").arg(kTemplateFileExtension));
  }

  mNew.clear();
  mCurrent = -1;

  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    while (!file.atEnd()) {
      QString line = file.readLine();
      line.remove(line.length() - 1, 1);
      const int index = line.indexOf(QStringLiteral(":"));
      if (index == -1)
        continue;

      const QString name = line.sliced(0, index);
      if (index + 1 >= line.length())
        continue;
      QString value = line.sliced(index + 1);
      value = value.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
      value = value.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
      CommitTemplates::Template t;
      t.name = name;
      t.value = value;
      mNew.append(t);
    }
  }

  emit templatesChanged();
  if (!mNew.isEmpty()) {
    showTemplate(0);
  } else {
    setName(QString());
    setTemplateText(QString());
  }
}

void TemplateDialog::exportTemplates(QString filename) {
  if (filename.isEmpty()) {
    filename = QFileDialog::getSaveFileName(
        this, tr("Save Templates"),
        QStringLiteral("/home/%1%2")
            .arg("GittyupTemplates", kTemplateFileExtension),
        tr("Gittyup Templates (*%1)").arg(kTemplateFileExtension));
  }

  QString templatesStr;
  for (const auto &tmpl : mNew) {
    QString name = tmpl.name;
    QString value = tmpl.value;
    value = value.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    value = value.replace(QStringLiteral("\t"), QStringLiteral("\\t"));
    templatesStr += QStringLiteral("%1:%2\n").arg(name, value);
  }

  QFile file(filename);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream stream(&file);
    stream << templatesStr;
  }
}

void TemplateDialog::applyTemplates() {
  mTemplates = mNew;
  accept();
}

void TemplateDialog::showTemplate(int index) {
  if (index < 0 || index >= mNew.count())
    return;

  mCurrent = index;
  mName = mNew.at(index).name;
  mTemplate = mNew.at(index).value;
  emit templatesChanged();
  emit editChanged();
}

bool TemplateDialog::uniqueName(const QString &name) const {
  for (const auto &templ : mNew) {
    if (templ.name == name)
      return false;
  }
  return true;
}
