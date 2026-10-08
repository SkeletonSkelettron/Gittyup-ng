#include "Test.h"

#include "ui/CommitTemplates.h"
#include "ui/TemplateDialog.h"

#include <QLineEdit>
#include <QListWidget>

class TestCommitMessageTemplate : public QObject {
  Q_OBJECT

private slots:
  void testImportExport();
  void testMoveUp();
  void testMoveDown();
  void testRemove();
  void testRemoveNoItemAvailable();
};

void TestCommitMessageTemplate::testImportExport() {
  QList<CommitTemplates::Template> templates;
  QTemporaryFile f;
  QVERIFY(f.open());
  QVERIFY(!f.fileName().isEmpty());

  {
    TemplateDialog d(templates);

    d.setName(QStringLiteral("Template1"));
    d.setTemplateText(QStringLiteral("ContentTemplate1:"));
    d.addTemplate();

    d.setName(QStringLiteral("Template2"));
    d.setTemplateText(
        QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));
    d.addTemplate();
    d.exportTemplates(f.fileName());
  }

  {
    TemplateDialog d(templates);
    QCOMPARE(d.mTemplates.length(), 0);
    QCOMPARE(d.names().count(), 0);
    d.importTemplates(f.fileName());
    d.applyTemplates();
    QCOMPARE(d.mTemplates.length(), 2);
    QCOMPARE(d.mTemplates.at(0).name, QStringLiteral("Template1"));
    QCOMPARE(d.mTemplates.at(0).value, QStringLiteral("ContentTemplate1:"));
    QCOMPARE(d.mTemplates.at(1).name, QStringLiteral("Template2"));
    QCOMPARE(d.mTemplates.at(1).value,
             QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));

    QCOMPARE(d.names().count(), 2);
  }
}

void TestCommitMessageTemplate::testMoveUp() {
  QList<CommitTemplates::Template> templates;
  TemplateDialog d(templates);

  d.setName(QStringLiteral("Template1"));
  d.setTemplateText(QStringLiteral("ContentTemplate1:"));
  d.addTemplate();

  d.setName(QStringLiteral("Template2"));
  d.setTemplateText(
      QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));
  d.addTemplate();

  d.setName(QStringLiteral("Template3"));
  d.setTemplateText(QStringLiteral("ContentTemplate3:"));
  d.addTemplate();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template2"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template3"));
  QCOMPARE(d.names().count(), 3);
  QCOMPARE(d.names().at(0), QStringLiteral("Template1"));
  QCOMPARE(d.names().at(1), QStringLiteral("Template2"));
  QCOMPARE(d.names().at(2), QStringLiteral("Template3"));

  d.setCurrent(1);
  d.moveTemplateUp();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template2"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template3"));
  QCOMPARE(d.names().count(), 3);
  QCOMPARE(d.names().at(0), QStringLiteral("Template2"));
  QCOMPARE(d.names().at(1), QStringLiteral("Template1"));
  QCOMPARE(d.names().at(2), QStringLiteral("Template3"));

  d.setCurrent(0);
  d.moveTemplateUp();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template2"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template3"));
  QCOMPARE(d.names().count(), 3);
  QCOMPARE(d.names().at(0), QStringLiteral("Template2"));
  QCOMPARE(d.names().at(1), QStringLiteral("Template1"));
  QCOMPARE(d.names().at(2), QStringLiteral("Template3"));
}

void TestCommitMessageTemplate::testMoveDown() {
  QList<CommitTemplates::Template> templates;
  TemplateDialog d(templates);

  d.setName(QStringLiteral("Template1"));
  d.setTemplateText(QStringLiteral("ContentTemplate1:"));
  d.addTemplate();

  d.setName(QStringLiteral("Template2"));
  d.setTemplateText(
      QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));
  d.addTemplate();

  d.setName(QStringLiteral("Template3"));
  d.setTemplateText(QStringLiteral("ContentTemplate3:"));
  d.addTemplate();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template2"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template3"));
  QCOMPARE(d.names().count(), 3);
  QCOMPARE(d.names().at(0), QStringLiteral("Template1"));
  QCOMPARE(d.names().at(1), QStringLiteral("Template2"));
  QCOMPARE(d.names().at(2), QStringLiteral("Template3"));

  d.setCurrent(1);
  d.moveTemplateDown();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template3"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template2"));
  QCOMPARE(d.names().count(), 3);
  QCOMPARE(d.names().at(0), QStringLiteral("Template1"));
  QCOMPARE(d.names().at(1), QStringLiteral("Template3"));
  QCOMPARE(d.names().at(2), QStringLiteral("Template2"));

  d.setCurrent(2);
  d.moveTemplateDown();

  QCOMPARE(d.mNew.count(), 3);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template3"));
  QCOMPARE(d.mNew.at(2).name, QStringLiteral("Template2"));
  QCOMPARE(d.names().count(), 3);
  QCOMPARE(d.names().at(0), QStringLiteral("Template1"));
  QCOMPARE(d.names().at(1), QStringLiteral("Template3"));
  QCOMPARE(d.names().at(2), QStringLiteral("Template2"));
}

void TestCommitMessageTemplate::testRemove() {
  QList<CommitTemplates::Template> templates;
  TemplateDialog d(templates);

  d.setName(QStringLiteral("Template1"));
  d.setTemplateText(QStringLiteral("ContentTemplate1:"));
  d.addTemplate();

  d.setName(QStringLiteral("Template2"));
  d.setTemplateText(
      QStringLiteral("ContentTemplate2: %|\nfiles: ${files:3}\t"));
  d.addTemplate();

  d.setName(QStringLiteral("Template3"));
  d.setTemplateText(QStringLiteral("ContentTemplate3:"));
  d.addTemplate();

  d.setCurrent(1);
  d.removeTemplate();

  QCOMPARE(d.mNew.count(), 2);
  QCOMPARE(d.mNew.at(0).name, QStringLiteral("Template1"));
  QCOMPARE(d.mNew.at(1).name, QStringLiteral("Template3"));
  QCOMPARE(d.names().count(), 2);
  QCOMPARE(d.names().at(0), QStringLiteral("Template1"));
  QCOMPARE(d.names().at(1), QStringLiteral("Template3"));

  QCOMPARE(d.name(), QStringLiteral("Template3"));
  QCOMPARE(d.templateText(), QStringLiteral("ContentTemplate3:"));
}

void TestCommitMessageTemplate::testRemoveNoItemAvailable() {
  QList<CommitTemplates::Template> templates;
  TemplateDialog d(templates);
  d.removeTemplate();

  // Should not crash!
}

TEST_MAIN(TestCommitMessageTemplate)
#include "commitMessageTemplate.moc"
