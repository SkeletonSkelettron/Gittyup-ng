//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Claude Opus 5.5, an AI model by Anthropic
//

#include "Test.h"
#include "dialogs/ConfirmDialog.h"
#include "git/Index.h"
#include "ui/FileContextMenu.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QAction>
#include <QFile>
#include <QProcess>

using namespace Test;
using namespace QTest;

namespace {

QString runGit(const QString &dir, const QStringList &args) {
  QProcess process;
  process.setWorkingDirectory(dir);
  process.start("git", QStringList{"-c", "user.name=Test", "-c",
                                   "user.email=test@example.com"} +
                           args);
  process.waitForFinished(-1);
  return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

void write(const QString &path, const QByteArray &content) {
  QFile file(path);
  QVERIFY(file.open(QFile::WriteOnly));
  file.write(content);
}

QString content(const QString &path) {
  QFile file(path);
  return file.open(QFile::ReadOnly) ? QString::fromUtf8(file.readAll())
                                    : QString();
}

// Merge a branch that changes f.txt like we do and deletes g.txt, which we
// change: f.txt conflicts on both sides, and g.txt was deleted by them.
void makeConflicts(const QString &dir) {
  write(dir + "/f.txt", "a\nb\n");
  write(dir + "/g.txt", "keep\n");
  runGit(dir, {"add", "."});
  runGit(dir, {"commit", "-q", "-m", "base"});

  runGit(dir, {"checkout", "-q", "-b", "other"});
  write(dir + "/f.txt", "a\ntheirs\n");
  runGit(dir, {"rm", "-q", "g.txt"});
  runGit(dir, {"commit", "-q", "-a", "-m", "other"});

  runGit(dir, {"checkout", "-q", "-"});
  write(dir + "/f.txt", "a\nours\n");
  write(dir + "/g.txt", "changed\n");
  runGit(dir, {"commit", "-q", "-a", "-m", "ours"});

  runGit(dir, {"merge", "-q", "other"});
}

// The visible confirmation dialog of the view.
ConfirmDialog *confirmDialog(QWidget *parent) {
  for (ConfirmDialog *dialog : parent->findChildren<ConfirmDialog *>()) {
    if (dialog->isVisible())
      return dialog;
  }
  return nullptr;
}

} // namespace

class TestTakeSide : public QObject {
  Q_OBJECT

private slots:
  void theirs();
  void ours();
  void menu();
};

// Their version of a conflict, and their deletion.
void TestTakeSide::theirs() {
  ScratchRepository repo;
  QString dir = repo->workdir().path();
  makeConflicts(dir);
  QCOMPARE(runGit(dir, {"status", "--porcelain"}),
           QString("UU f.txt\nUD g.txt"));

  git::Index index = repo->index();
  index.read();
  QCOMPARE(index.isStaged("f.txt"), git::Index::Conflicted);
  QVERIFY(index.takeSide({"f.txt", "g.txt"}, git::Index::Theirs));

  QCOMPARE(content(dir + "/f.txt"), QString("a\ntheirs\n"));
  QVERIFY(!QFile::exists(dir + "/g.txt"));
  QCOMPARE(runGit(dir, {"ls-files", "-u"}), QString());
  QCOMPARE(runGit(dir, {"status", "--porcelain"}),
           QString("M  f.txt\nD  g.txt"));
  QCOMPARE(index.isStaged("f.txt"), git::Index::Staged);
}

// Our version of a conflict, and of a file they deleted.
void TestTakeSide::ours() {
  ScratchRepository repo;
  QString dir = repo->workdir().path();
  makeConflicts(dir);

  git::Index index = repo->index();
  index.read();
  QVERIFY(index.takeSide({"f.txt", "g.txt"}, git::Index::Ours));

  QCOMPARE(content(dir + "/f.txt"), QString("a\nours\n"));
  QCOMPARE(content(dir + "/g.txt"), QString("changed\n"));
  QCOMPARE(runGit(dir, {"ls-files", "-u"}), QString());

  // Both are like in our commit.
  QCOMPARE(runGit(dir, {"status", "--porcelain"}), QString());
}

// The file lists of the details offer it for conflicts, after confirming it.
void TestTakeSide::menu() {
  ScratchRepository repo;
  QString dir = repo->workdir().path();
  makeConflicts(dir);
  write(dir + "/new.txt", "new\n");

  MainWindow window(repo);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));
  RepoView *view = window.currentView();
  refresh(view);

  auto action = [](QMenu &menu, const QString &text) -> QAction * {
    for (QAction *action : menu.actions()) {
      if (action->text() == text)
        return action;
    }
    return nullptr;
  };

  // Not for a file that doesn't conflict.
  FileContextMenu plain(view, {"new.txt"}, git::Index());
  QVERIFY(!action(plain, "Take Ours"));
  QVERIFY(!action(plain, "Take Theirs"));

  // The lists of the details give no index.
  FileContextMenu menu(view, {"f.txt"}, git::Index());
  QVERIFY(action(menu, "Take Ours"));
  QAction *theirs = action(menu, "Take Theirs");
  QVERIFY(theirs);

  theirs->trigger();
  ConfirmDialog *dialog = confirmDialog(view);
  QVERIFY(dialog);
  dialog->accept();

  QCOMPARE(content(dir + "/f.txt"), QString("a\ntheirs\n"));
  QCOMPARE(runGit(dir, {"ls-files", "-u", "--", "f.txt"}), QString());
}

TEST_MAIN(TestTakeSide)

#include "take_side.moc"
