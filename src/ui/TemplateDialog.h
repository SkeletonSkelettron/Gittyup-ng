//
//          Copyright (c) 2022, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef TEMPLATEDIALOG_H
#define TEMPLATEDIALOG_H

#include "CommitTemplates.h"
#include "dialogs/QmlDialog.h"

// Edit the commit message templates. qrc:/qml/TemplateDialog.qml draws it.
class TemplateDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QStringList names READ names NOTIFY templatesChanged)
  Q_PROPERTY(int current READ current WRITE setCurrent NOTIFY
                 templatesChanged)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY editChanged)
  Q_PROPERTY(QString templateText READ templateText WRITE setTemplateText
                 NOTIFY editChanged)
  Q_PROPERTY(bool nameExists READ nameExists NOTIFY editChanged)
  Q_PROPERTY(QString cursorHint READ cursorHint CONSTANT)

public:
  TemplateDialog(QList<CommitTemplates::Template> &templates,
                 QWidget *parent = nullptr);

  QStringList names() const;
  int current() const { return mCurrent; }
  void setCurrent(int index);

  // The template that is edited.
  QString name() const { return mName; }
  void setName(const QString &name);
  QString templateText() const { return mTemplate; }
  void setTemplateText(const QString &text);
  bool nameExists() const { return !uniqueName(mName); }

  QString cursorHint() const;

  Q_INVOKABLE void addTemplate();
  Q_INVOKABLE void removeTemplate();
  Q_INVOKABLE void moveTemplateUp();
  Q_INVOKABLE void moveTemplateDown();
  Q_INVOKABLE void importTemplates(QString filename = QString());
  Q_INVOKABLE void exportTemplates(QString filename = QString());
  Q_INVOKABLE void applyTemplates();

signals:
  void templatesChanged();
  void editChanged();

private:
  bool uniqueName(const QString &name) const;
  void showTemplate(int index);
  void moveTemplate(int offset);

  QList<CommitTemplates::Template> &mTemplates;
  QList<CommitTemplates::Template> mNew;
  int mCurrent = -1;
  QString mName;
  QString mTemplate;

  friend class TestCommitMessageTemplate;
};

#endif
