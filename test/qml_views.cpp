//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "Test.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/AccountDialog.h"
#include "dialogs/AddRemoteDialog.h"
#include "dialogs/AmendDialog.h"
#include "dialogs/CheckoutDialog.h"
#include "dialogs/CloneDialog.h"
#include "dialogs/CommitDialog.h"
#include "dialogs/ConfigDialog.h"
#include "dialogs/ConfirmDialog.h"
#include "dialogs/ExternalToolsDialog.h"
#include "dialogs/InputDialog.h"
#include "dialogs/MergeDialog.h"
#include "dialogs/NewBranchDialog.h"
#include "dialogs/PluginsDialog.h"
#include "dialogs/RemoteDialog.h"
#include "dialogs/RenameBranchDialog.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/TagDialog.h"
#include "dialogs/ThemeDialog.h"
#include "dialogs/UpdateSubmodulesDialog.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "log/LogEntry.h"
#include "ui/IgnoreDialog.h"
#include "ui/DetailView.h"
#include "ui/MergeModel.h"
#include "git/Config.h"
#include "ui/CommitList.h"
#include "ui/CommandPalette.h"
#include "ui/PullRequestList.h"
#include "ui/RefsPanel.h"
#include "host/PullRequests.h"
#include "ui/FindController.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "ui/TabWidget.h"
#include "ui/TemplateDialog.h"
#include "ui/MenuBar.h"
#include "ui/SearchField.h"
#include "ui/ToolBar.h"
#include "ui/qml/QmlSupport.h"
#include "update/UpdateDialog.h"
#include <QClipboard>
#include <QMenu>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QFile>
#include <QProcess>
#include <QQuickItem>
#include <QQuickWidget>

using namespace Test;
using namespace QTest;

namespace {

// Warnings and errors of the QML files.
QStringList sMessages;
QtMessageHandler sPrevious = nullptr;

void handleMessage(QtMsgType type, const QMessageLogContext &context,
                   const QString &message) {
  if (message.contains("qrc:/qml/"))
    sMessages.append(message);
  if (sPrevious)
    sPrevious(type, context, message);
}

} // namespace

// Opens the main window and every QML dialog and checks that their QML
// loads without warnings.
class TestQmlViews : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void mainWindow();
  void dialogs();
  void settings();
  void search();
  void menu();
  void solo();
  void hide();
  void palette();
  void pullRequests();
  void merge();
  void mergeEncoding();
  void mergeScroll();
  void mergeEmptySide();
  void dragTab();
  void cleanupTestCase();

private:
  // Show the dialog and check its QML.
  void check(QDialog *dialog, const QString &name);

  ScratchRepository mRepo;
  MainWindow *mWindow = nullptr;
};

void TestQmlViews::initTestCase() {
  sPrevious = qInstallMessageHandler(handleMessage);

  // A commit and a change.
  QFile file(mRepo->workdir().filePath("file.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("content\n");
  file.close();
  mRepo->index().setStaged({"file.txt"}, true);
  QVERIFY(mRepo->commit("initial"));

  QVERIFY(file.open(QFile::WriteOnly | QFile::Append));
  file.write("change\n");
  file.close();

  mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(qWaitForWindowExposed(mWindow));
}

void TestQmlViews::mainWindow() {
  RepoView *view = mWindow->currentView();
  refresh(view);

  // The commit, the log and the welcome page.
  view->selectFirstCommit();
  view->addLogEntry("text", "Title")->addEntry(LogEntry::Error, "error");
  view->setLogVisible(true);
  mWindow->tabWidget()->setWelcomeVisible(true);
  qWait(200);
  mWindow->tabWidget()->setWelcomeVisible(false);
  qWait(100);

  // The content of a file with its blame in tree mode.
  view->setViewMode(RepoView::Tree);
  DetailView *details = view->findChild<DetailView *>();
  details->selectPath("file.txt");
  QCOMPARE(details->file(), QString("file.txt"));
  QAbstractItemModel *content =
      qobject_cast<QAbstractItemModel *>(details->contentModel());
  QVERIFY(content->rowCount() > 0);
  QTRY_VERIFY(!content->property("blameLoading").toBool());
  QVERIFY(content->property("hasBlame").toBool());
  QCOMPARE(content->index(0, 0).data(Qt::UserRole + 3).toString().isEmpty(),
           false);

  // Find in the file.
  FindController *finder = qobject_cast<FindController *>(details->finder());
  finder->show();
  finder->search("change");
  QVERIFY(finder->hasMatches());
  QCOMPARE(finder->hitsText(), QString("1 of 1"));
  qWait(100);
  details->closeFile();
  QVERIFY(!finder->isVisible());
  view->setViewMode(RepoView::DoubleTree);
  qWait(100);

  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::dialogs() {
  RepoView *view = mWindow->currentView();
  git::Repository repo = view->repo();
  git::Commit head = repo.head().target();

  check(new NewBranchDialog(repo, git::Commit(), view), "NewBranchDialog");
  check(new TagDialog(repo, head.shortId(), git::Remote(), view), "TagDialog");
  check(new CheckoutDialog(repo, repo.head(), view), "CheckoutDialog");
  check(new RenameBranchDialog(repo, repo.head(), view), "RenameBranchDialog");
  check(new AddRemoteDialog("origin", view), "AddRemoteDialog");
  check(new MergeDialog(RepoView::Merge, repo, view), "MergeDialog");
  check(new CloneDialog(CloneDialog::Clone, view), "CloneDialog");
  check(new CloneDialog(CloneDialog::Init, view), "CloneDialog (init)");
  check(new AmendDialog(head.author(), head.committer(), head.message(), view),
        "AmendDialog");
  check(new CommitDialog("message", Prompt::Kind::Stash, view),
        "CommitDialog");
  check(new AccountDialog(nullptr, view), "AccountDialog");
  check(new RemoteDialog(RemoteDialog::Push, view), "RemoteDialog");
  check(new UpdateSubmodulesDialog(repo, view), "UpdateSubmodulesDialog");
  check(new IgnoreDialog("*.log", view), "IgnoreDialog");
  check(new ThemeDialog(view), "ThemeDialog");
  check(new AboutDialog(view), "AboutDialog");
  check(new PluginsDialog(repo, view), "PluginsDialog");
  check(new ExternalToolsDialog("diff", view), "ExternalToolsDialog");
  check(new UpdateDialog("linux", "99.0.0", "<p>Notes</p>", "", view),
        "UpdateDialog");
  check(new InputDialog("Title", "Text",
                        {{"Username", "name"}, {"Password", "", true}}, view),
        "InputDialog");

  ConfirmDialog *confirm = new ConfirmDialog(view);
  confirm->setTitle("Title");
  confirm->setText("Text");
  confirm->setDetailedText("Details");
  confirm->setCheckText("Check");
  confirm->addButton("Other");
  check(confirm, "ConfirmDialog");

  QList<CommitTemplates::Template> templates = {{"Name", "Value"}};
  TemplateDialog *templateDialog = new TemplateDialog(templates, view);
  check(templateDialog, "TemplateDialog");
}

void TestQmlViews::settings() {
  RepoView *view = mWindow->currentView();

  // Every section of the application settings.
  SettingsDialog *settings = new SettingsDialog(SettingsDialog::General);
  for (int i = SettingsDialog::General; i <= SettingsDialog::Terminal; ++i)
    settings->setSection(i);
  check(settings, "SettingsDialog");

  // Every section of the repository settings.
  ConfigDialog *config = new ConfigDialog(view);
  for (int i = ConfigDialog::General; i <= ConfigDialog::Lfs; ++i)
    config->setSection(i);
  check(config, "ConfigDialog");
}

void TestQmlViews::search() {
  SearchField *search = mWindow->toolBar()->searchField();
  QVERIFY(search->isEnabled());

  // The advanced search fills its fields from the query.
  search->setText("author:someone words");
  search->showAdvanced();
  QVERIFY(search->advancedVisible());
  qWait(200);
  search->setAdvancedValue(0, "other");
  search->acceptAdvanced();
  QCOMPARE(search->text(), QString("other author:someone"));
  QVERIFY(!search->advancedVisible());

  // The edit menu works on the focused QML field.
  search->setText("hello");
  QQuickWidget *view = mWindow->quickView();
  QQuickItem *input =
      view->rootObject()->findChild<QQuickItem *>("searchInput");
  QVERIFY(input);
  mWindow->activateWindow();
  QVERIFY(qWaitForWindowActive(mWindow));
  view->setFocus();
  qWait(50);
  input->forceActiveFocus();
  qWait(50);
  QApplication::clipboard()->clear();

  MenuBar *menuBar = MenuBar::instance(mWindow);
  QMenu *edit = nullptr;
  for (QMenu *menu : menuBar->findChildren<QMenu *>()) {
    if (menu->title() == "Edit")
      edit = menu;
  }
  QVERIFY(edit);

  QAction *selectAll = nullptr;
  QAction *copy = nullptr;
  for (QAction *action : edit->actions()) {
    if (action->text() == "Select All")
      selectAll = action;
    else if (action->text() == "Copy")
      copy = action;
  }
  QVERIFY(selectAll && copy);

  // Opening the menu updates the actions.
  emit edit->aboutToShow();
  QVERIFY(selectAll->isEnabled());
  QVERIFY(!copy->isEnabled());
  selectAll->trigger();
  emit edit->aboutToShow();
  QVERIFY(copy->isEnabled());
  copy->trigger();
  QCOMPARE(QApplication::clipboard()->text(), QString("hello"));

  search->edit("", 0);
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::menu() {
  QMenu menu;
  QAction *first = menu.addAction("First");
  menu.addSeparator();
  QMenu *more = menu.addMenu("More");
  more->addAction("Second");

  bool triggered = false;
  connect(first, &QAction::triggered, [&triggered] { triggered = true; });

  // The view of the window draws the menu.
  QQuickWidget *view = mWindow->quickView();
  QPoint pos = view->mapToGlobal(QPoint(300, 300));
  QCOMPARE(QApplication::widgetAt(pos), view);

  // Tool tips of the items below the menu aren't shown over it.
  bool toolTipShown = true;
  QTimer::singleShot(200, [view, &toolTipShown] {
    QmlHost *host = QmlSupport::host(view);
    host->showToolTip("tip", 10, 10, 20, 20);
    toolTipShown = host->toolTipVisible();

    keyClick(view, Qt::Key_Down);
    keyClick(view, Qt::Key_Return);
  });

  QCOMPARE(QmlSupport::execMenu(&menu, pos), first);
  QVERIFY(!toolTipShown);
  QVERIFY(triggered);
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::solo() {
  RepoView *view = mWindow->currentView();
  git::Repository repo = view->repo();
  CommitList *commits = view->commitList();
  QAbstractItemModel *model = commits->model();

  // A branch at HEAD that isn't checked out.
  git::Branch other = repo.createBranch("other", repo.head().target());
  QVERIFY(other.isValid());
  QTRY_VERIFY(!commits->isLoading());
  QTRY_COMPARE(model->rowCount(), 2); // The uncommitted changes and a commit.

  // Soloing it hides the uncommitted changes of HEAD.
  commits->setSoloed(other.qualifiedName(), true);
  QCOMPARE(commits->solo(), QStringList({other.qualifiedName()}));
  QCOMPARE(commits->soloText(), QString("other"));
  QTRY_COMPARE(model->rowCount(), 1);
  QVERIFY(model->index(0, 0).data(CommitList::CommitRole).value<git::Commit>()
              .isValid());

  // It's saved in the repository.
  QCOMPARE(repo.appConfig().value<QString>("solo.refs"),
           other.qualifiedName());

  // Soloing HEAD's branch too shows them again.
  commits->setSoloed(repo.head().qualifiedName(), true);
  QCOMPARE(commits->soloText(), QString("2 branches"));
  QTRY_COMPARE(model->rowCount(), 2);

  commits->unsoloAll();
  QVERIFY(commits->solo().isEmpty());
  QTRY_COMPARE(model->rowCount(), 2);

  // Deleting a soloed branch stops soloing it.
  commits->setSoloed(other.qualifiedName(), true);
  other.remove();
  QTRY_VERIFY(commits->solo().isEmpty());
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::hide() {
  RepoView *view = mWindow->currentView();
  git::Repository repo = view->repo();
  CommitList *commits = view->commitList();
  QAbstractItemModel *model = commits->model();

  // A branch with a commit that HEAD doesn't have.
  git::Commit first = repo.head().target();
  QFile file(repo.workdir().filePath("side.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("side\n");
  file.close();
  repo.index().setStaged({"side.txt"}, true);
  git::Commit second = repo.commit("side");
  git::Branch side = repo.createBranch("side", second);
  repo.head().setTarget(first, "test");
  QTRY_VERIFY(!commits->isLoading());
  QTRY_COMPARE(model->rowCount(), 3);

  // Hiding it leaves out its commit and saves it in the repository.
  QVERIFY(!commits->canHide(repo.head().qualifiedName()));
  commits->setHidden(side.qualifiedName(), true);
  QVERIFY(commits->isHidden(side.qualifiedName()));
  QCOMPARE(commits->hiddenText(), QString("side"));
  QCOMPARE(repo.appConfig().value<QString>("hide.refs"),
           side.qualifiedName());
  QTRY_COMPARE(model->rowCount(), 2);

  // The checked out branch can't be hidden.
  commits->setHidden(repo.head().qualifiedName(), true);
  QCOMPARE(commits->hidden(), QStringList({side.qualifiedName()}));

  // Hiding a remote hides its branches.
  commits->setHidden("refs/remotes/origin/", true);
  QVERIFY(commits->isHidden("refs/remotes/origin/main"));
  QCOMPARE(commits->hiddenText(), QString("2 branches"));

  commits->showAll();
  QVERIFY(commits->hidden().isEmpty());
  QTRY_COMPARE(model->rowCount(), 3);

  // Soloing a hidden branch shows it.
  commits->setHidden(side.qualifiedName(), true);
  QTRY_COMPARE(model->rowCount(), 2);
  commits->setSoloed(side.qualifiedName(), true);
  QTRY_COMPARE(model->rowCount(), 2); // The commit and its parent.
  commits->unsoloAll();

  // Deleting a hidden branch stops hiding it.
  side.remove();
  QTRY_VERIFY(commits->hidden().isEmpty());

  repo.index().setStaged({"side.txt"}, false);
  QFile::remove(repo.workdir().filePath("side.txt"));
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::palette() {
  RepoView *view = mWindow->currentView();
  git::Repository repo = view->repo();
  CommandPalette *palette = mWindow->commandPalette();

  // Fuzzy matching prefers the starts of words and consecutive characters.
  QList<int> positions;
  QCOMPARE(CommandPalette::score("chk", "Checkout", &positions), 15);
  QCOMPARE(positions, QList<int>({0, 1, 4}));
  QVERIFY(CommandPalette::score("fb", "foo_bar") >
          CommandPalette::score("fb", "fooxbar"));
  QCOMPARE(CommandPalette::score("xyz", "Checkout"), -1);

  // Ctrl+P opens it with the focus in the search field.
  QQuickItem *input = mWindow->quickView()->rootObject()->findChild<QQuickItem *>(
      "commandPaletteInput");
  QVERIFY(input);
  mWindow->activateWindow();
  keyClick(mWindow->quickView(), Qt::Key_P, Qt::ControlModifier);
  QTRY_VERIFY(palette->isVisible());
  QTRY_VERIFY(input->hasActiveFocus());
  QVERIFY(palette->rowCount() > 0);

  // Files, commands and branches.
  palette->setQuery("file.txt");
  QVERIFY(palette->rowCount() > 0);
  QCOMPARE(palette->index(0).data(CommandPalette::KindRole).toInt(),
           int(CommandPalette::File));
  QCOMPARE(palette->index(0).data(CommandPalette::TitleRole).toString(),
           QString("file.txt"));

  palette->setQuery(">refresh");
  QVERIFY(palette->rowCount() > 0);
  QCOMPARE(palette->index(0).data(CommandPalette::TitleRole).toString(),
           QString("Refresh"));
  for (int row = 0; row < palette->rowCount(); ++row)
    QCOMPARE(palette->index(row).data(CommandPalette::KindRole).toInt(),
             int(CommandPalette::Command));

  palette->setQuery("@" + repo.head().name());
  QVERIFY(palette->rowCount() > 0);
  QCOMPARE(palette->index(0).data(CommandPalette::KindRole).toInt(),
           int(CommandPalette::Branch));

  // Escape closes it.
  keyClick(mWindow->quickView(), Qt::Key_Escape);
  QTRY_VERIFY(!palette->isVisible());

  // Opening a file shows it in the tree view.
  palette->open("file.txt");
  palette->activate(0);
  QVERIFY(!palette->isVisible());
  QTRY_COMPARE(view->viewMode(), RepoView::Tree);
  QTRY_COMPARE(view->detailView()->file(), QString("file.txt"));
  view->setViewMode(RepoView::DoubleTree);
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::pullRequests() {
  // Remote URLs of the hosting services.
  QString server, path;
  QVERIFY(PullRequests::parseUrl("git@github.com:owner/repo.git", &server,
                                 &path));
  QCOMPARE(server, QString("github.com"));
  QCOMPARE(path, QString("owner/repo"));
  QVERIFY(PullRequests::parseUrl("https://gitlab.com/group/sub/repo", &server,
                                 &path));
  QCOMPARE(path, QString("group/sub/repo"));
  QVERIFY(PullRequests::parseUrl("ssh://git@example.com:2222/a/b.git",
                                 &server, &path));
  QCOMPARE(server, QString("example.com"));
  QCOMPARE(path, QString("a/b"));
  QVERIFY(!PullRequests::parseUrl("/home/user/repo", &server, &path));

  // Replies of GitHub and GitLab.
  QByteArray github = R"([{
    "number": 12, "title": "Add login", "body": "Adds **login**.",
    "user": {"login": "ada"}, "draft": false,
    "html_url": "https://github.com/owner/repo/pull/12",
    "created_at": "2026-09-01T10:00:00Z", "updated_at": "2026-09-02T10:00:00Z",
    "head": {"ref": "login", "repo": {"full_name": "owner/repo"}},
    "base": {"ref": "main", "repo": {"full_name": "owner/repo"}},
    "labels": [{"name": "feature"}]
  }, {
    "number": 13, "title": "Fix typo", "user": {"login": "bob"}, "draft": true,
    "head": {"ref": "typo", "repo": {"full_name": "bob/repo"}},
    "base": {"ref": "main", "repo": {"full_name": "owner/repo"}}
  }])";
  QString error;
  QList<PullRequest> list =
      PullRequests::parse(PullRequests::GitHub, github, &error);
  QCOMPARE(error, QString());
  QCOMPARE(list.size(), 2);
  QCOMPARE(list.at(0).number, 12);
  QCOMPARE(list.at(0).author, QString("ada"));
  QCOMPARE(list.at(0).head, QString("login"));
  QCOMPARE(list.at(0).labels, QStringList({"feature"}));
  QVERIFY(!list.at(0).fork);
  QVERIFY(list.at(1).fork);
  QVERIFY(list.at(1).draft);
  QCOMPARE(list.at(1).headRepo, QString("bob/repo"));

  QByteArray gitlab = R"([{"iid": 3, "title": "Speed up", "description": "",
    "author": {"username": "cy"}, "source_branch": "fast",
    "target_branch": "main", "source_project_id": 1, "target_project_id": 1,
    "web_url": "https://gitlab.com/g/r/-/merge_requests/3", "labels": ["perf"]}])";
  list = PullRequests::parse(PullRequests::GitLab, gitlab, &error);
  QCOMPARE(list.size(), 1);
  QCOMPARE(list.at(0).number, 3);
  QCOMPARE(list.at(0).head, QString("fast"));
  QVERIFY(!list.at(0).fork);

  PullRequests::parse(PullRequests::GitHub, R"({"message": "Not Found"})",
                      &error);
  QCOMPARE(error, QString("Not Found"));

  // The references panel lists them, and they are shown in the page.
  RepoView *view = mWindow->currentView();
  PullRequestList *pulls = view->pullRequestList();
  QVERIFY(!pulls->isSupported());
  QVERIFY(pulls->source()->setRemote("https://github.com/owner/repo.git"));
  QCOMPARE(pulls->service(), QString("GitHub"));
  pulls->source()->setPullRequests(
      PullRequests::parse(PullRequests::GitHub, github));

  QAbstractItemModel *refs = view->findChild<RefsPanel *>()->model();
  QStringList names;
  for (int row = 0; row < refs->rowCount(); ++row)
    names.append(refs->index(row, 0).data(RefsModel::NameRole).toString());
  QVERIFY(names.contains("Pull Requests"));
  QVERIFY(names.contains("#12 Add login"));
  QVERIFY(names.contains("#13 Fix typo"));

  pulls->show(12);
  QVERIFY(pulls->isActive());
  QCOMPARE(pulls->current().value("title").toString(), QString("Add login"));
  QVERIFY(!pulls->canCheckout());
  QTest::qWait(100);
  pulls->show(13);
  QCOMPARE(pulls->current().value("head").toString(),
           QString("bob/repo:typo"));
  pulls->close();
  QVERIFY(!pulls->isActive());

  // The list of another repository closes the one that's shown.
  pulls->show(12);
  pulls->source()->setRemote(QString());
  QVERIFY(!pulls->isActive());
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::merge() {
  RepoView *view = mWindow->currentView();
  git::Repository repo = view->repo();

  // A file with two conflicts.
  QFile file(repo.workdir().filePath("conflict.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write("top\n"
             "<<<<<<< HEAD\n"
             "ours 1\n"
             "=======\n"
             "theirs 1\n"
             ">>>>>>> feature\n"
             "middle\n"
             "<<<<<<< HEAD\n"
             "ours 2a\n"
             "ours 2b\n"
             "||||||| base\n"
             "base 2\n"
             "=======\n"
             "theirs 2\n"
             ">>>>>>> feature\n"
             "bottom\n");
  file.close();

  MergeModel merge(view);
  QTextDocument document;
  merge.setTextDocument(&document);
  merge.load("conflict.txt");
  QCOMPARE(merge.notice(), QString());
  QCOMPARE(merge.conflictCount(), 2);
  QCOMPARE(merge.oursLabel(), QString("HEAD"));
  QCOMPARE(merge.theirsLabel(), QString("feature"));
  QCOMPARE(merge.unresolvedCount(), 2);

  // Common lines and a header and the lines of each conflict.
  QAbstractItemModel *ours = qobject_cast<QAbstractItemModel *>(merge.ours());
  QCOMPARE(ours->rowCount(), 3 + 1 + 1 + 1 + 2);
  QCOMPARE(merge.conflictRow(0, 1), 4);

  // Nothing is taken from the conflicts yet.
  QCOMPARE(document.toPlainText(), QString("top\nmiddle\nbottom\n"));

  // The side that is taken first comes first.
  merge.setConflictChecked(1, 0, true);
  merge.setLineChecked(1, 1, 0, true);
  merge.setLineChecked(0, 1, 1, true);
  QCOMPARE(merge.unresolvedCount(), 0);
  QCOMPARE(document.toPlainText(),
           QString("top\ntheirs 1\nmiddle\ntheirs 2\nours 2b\nbottom\n"));

  // Edits of the output outside of a conflict are kept.
  QTextCursor cursor(&document);
  cursor.insertText("edited ");
  merge.setLineChecked(0, 1, 1, false);
  QCOMPARE(document.toPlainText(),
           QString("edited top\ntheirs 1\nmiddle\ntheirs 2\nbottom\n"));

  // Taking all of a side replaces the other one.
  merge.takeAll(0);
  QCOMPARE(document.toPlainText(),
           QString("edited top\nours 1\nmiddle\nours 2a\nours 2b\nbottom\n"));

  // Saving writes the output and stages the file.
  merge.save();
  QVERIFY(file.open(QFile::ReadOnly));
  QCOMPARE(file.readAll(),
           QByteArray("edited top\nours 1\nmiddle\nours 2a\nours 2b\nbottom\n"));
  file.close();
  QCOMPARE(repo.index().isStaged("conflict.txt"), git::Index::Staged);
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::mergeEncoding() {
  RepoView *view = mWindow->currentView();
  git::Repository repo = view->repo();

  // UTF-8 text isn't decoded in the encoding of the system, and is saved in
  // UTF-8 again. Georgian has bytes that Windows-1252 doesn't define.
  QString text = QString::fromUtf8("// \xe1\x83\xa5\xe1\x83\x90\xe1\x83\xa0"
                                   "\xe1\x83\x97\xe1\x83\xa3\xe1\x83\x9a"
                                   "\xe1\x83\x98\n");
  QByteArray utf8 = text.toUtf8();
  QCOMPARE(repo.encoding(utf8), QStringConverter::Utf8);
  QCOMPARE(repo.decode(utf8), text);

  QFile file(repo.workdir().filePath("georgian.txt"));
  QVERIFY(file.open(QFile::WriteOnly));
  file.write(utf8 + "<<<<<<< HEAD\nours\n=======\ntheirs\n>>>>>>> other\n");
  file.close();

  MergeModel merge(view);
  QTextDocument document;
  merge.setTextDocument(&document);
  merge.load("georgian.txt");
  QCOMPARE(merge.conflictCount(), 1);
  QCOMPARE(document.toPlainText(), text);

  merge.takeAll(0);
  merge.save();
  QVERIFY(file.open(QFile::ReadOnly));
  QCOMPARE(file.readAll(), utf8 + "ours\n");
  file.close();

  // Text that isn't UTF-8 keeps its bytes.
  QByteArray latin1 = "caf\xe9\n";
  QVERIFY(repo.encoding(latin1) != QStringConverter::Utf8);
  QVERIFY(file.open(QFile::WriteOnly));
  file.write(latin1 + "<<<<<<< HEAD\nours\n=======\ntheirs\n>>>>>>> other\n");
  file.close();

  merge.load("georgian.txt");
  QCOMPARE(document.toPlainText(), QString::fromLatin1("caf\xe9\n"));
  merge.takeAll(1);
  merge.save();
  QVERIFY(file.open(QFile::ReadOnly));
  QCOMPARE(file.readAll(), latin1 + "theirs\n");
  file.close();
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::mergeScroll() {
  // A merge with two conflicts that have more lines on the second side.
  ScratchRepository repo;
  auto git = [&repo](const QStringList &args) {
    QProcess process;
    process.setWorkingDirectory(repo->workdir().path());
    process.start(GIT_EXECUTABLE, args);
    return process.waitForFinished() &&
           process.exitStatus() == QProcess::NormalExit;
  };

  auto write = [&repo](const QStringList &changes) {
    QStringList lines;
    for (int i = 1; i <= 100; ++i)
      lines.append(QString("line %1").arg(i));
    for (const QString &change : changes) {
      int index = change.section(':', 0, 0).toInt();
      lines[index] = change.section(':', 1);
    }

    QFile file(repo->workdir().filePath("file.txt"));
    if (file.open(QFile::WriteOnly))
      file.write(lines.join('\n').toUtf8() + '\n');
  };

  write({});
  QVERIFY(git({"add", "file.txt"}));
  QVERIFY(git({"commit", "-q", "-m", "Add file"}));
  QVERIFY(git({"checkout", "-q", "-b", "other"}));
  write({"4:theirs 1\na\nb\nc\nd\ne\nf\ng", "20:theirs 2\na\nb"});
  QVERIFY(git({"commit", "-q", "-am", "Change theirs"}));
  QVERIFY(git({"checkout", "-q", "-"}));
  write({"4:ours 1\na", "20:ours 2"});
  QVERIFY(git({"commit", "-q", "-am", "Change ours"}));
  git({"merge", "other"});

  MainWindow window(repo);
  window.resize(1200, 1000);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));
  RepoView *view = window.currentView();
  refresh(view);

  // The file can be selected when the diff of the working copy is loaded.
  DetailView *detail = view->findChild<DetailView *>();
  detail->setMergeEditor(true);
  QTRY_VERIFY((detail->selectPath("file.txt"),
               detail->property("selectedFile").toString() == "file.txt"));

  QCOMPARE(repo->index().isStaged("file.txt"), git::Index::Conflicted);
  // The page of the repository belongs to its view.
  QQuickItem *panel = nullptr;
  QTRY_VERIFY((panel = view->findChild<QQuickItem *>("mergePanel")) &&
              panel->isVisible());
  QObject *ours = panel->findChild<QObject *>("mergeOurs");
  QObject *theirs = panel->findChild<QObject *>("mergeTheirs");
  QObject *output = panel->findChild<QObject *>("mergeOutput");
  QObject *outputPane = panel->findChild<QObject *>("mergeOutputPane");
  QVERIFY(ours && theirs && output && outputPane);
  QTRY_COMPARE(panel->property("layout").toList().size(), 5);

  // Rows of the sides, and the parts of the output, which has nothing of the
  // conflicts yet.
  qreal line = panel->property("lineHeight").toReal();
  auto y = [](QObject *pane) { return pane->property("contentY").toReal(); };
  auto bounds = [outputPane](int part) {
    return outputPane->property("bounds").toList().value(part).toReal();
  };
  QTRY_VERIFY(bounds(5) > bounds(4));

  // The pane under the mouse leads.
  QCOMPARE(panel->property("driver").toInt(), -1);
  panel->setProperty("driver", 0);

  // Scrolling ours into its conflict scrolls theirs faster through the longer
  // one. The output waits at the conflict.
  ours->setProperty("contentY", 5 * line);
  QCOMPARE(y(theirs), 4 * line + 9 * line / 3);
  QCOMPARE(y(output), bounds(1));

  // Scrolling theirs through its conflict, ours waits at the end of its
  // shorter one.
  panel->setProperty("driver", 1);
  theirs->setProperty("contentY", 10 * line);
  QCOMPARE(y(ours), 7 * line);
  QCOMPARE(y(output), bounds(1));

  // Common lines scroll line by line in every pane, also in the output,
  // whose lines have another height.
  theirs->setProperty("contentY", 15 * line);
  QCOMPARE(y(ours), 9 * line);
  QCOMPARE(y(output), bounds(2) + 2 * (bounds(3) - bounds(2)) / 15);

  // Taken lines are in the output.
  QObject *merge = detail->mergeModel();
  QMetaObject::invokeMethod(merge, "setConflictChecked", Q_ARG(int, 1),
                            Q_ARG(int, 0), Q_ARG(bool, true));
  QTRY_VERIFY(bounds(2) > bounds(1));
  panel->setProperty("driver", 0);
  ours->setProperty("contentY", 0);
  ours->setProperty("contentY", 4 * line + 3 * line / 2);
  QCOMPARE(y(output), bounds(1) + (bounds(2) - bounds(1)) / 2);

  // The output following its text cursor doesn't move the sides.
  qreal before = y(ours);
  output->setProperty("contentY", 0);
  QCOMPARE(y(ours), before);
  QVERIFY2(sMessages.isEmpty(), qPrintable(sMessages.join('\n')));
}

void TestQmlViews::mergeEmptySide() {
  // Ours changes a line that theirs removes: theirs has no lines.
  ScratchRepository repo;
  auto git = [&repo](const QStringList &args) {
    QProcess process;
    process.setWorkingDirectory(repo->workdir().path());
    process.start(GIT_EXECUTABLE, args);
    return process.waitForFinished() &&
           process.exitStatus() == QProcess::NormalExit;
  };

  auto write = [&repo](const QString &changed) {
    QStringList lines;
    for (int i = 1; i <= 20; ++i)
      lines.append(i == 10 ? changed : QString("line %1").arg(i));
    lines.removeAll(QString());

    QFile file(repo->workdir().filePath("file.txt"));
    if (file.open(QFile::WriteOnly))
      file.write(lines.join('\n').toUtf8() + '\n');
  };

  write("line 10");
  QVERIFY(git({"add", "file.txt"}));
  QVERIFY(git({"commit", "-q", "-m", "Add file"}));
  QVERIFY(git({"checkout", "-q", "-b", "other"}));
  write(QString());
  QVERIFY(git({"commit", "-q", "-am", "Remove the line"}));
  QVERIFY(git({"checkout", "-q", "-"}));
  write("ours 10");
  QVERIFY(git({"commit", "-q", "-am", "Change the line"}));
  git({"merge", "other"});

  MainWindow window(repo);
  window.resize(1200, 1000);
  window.show();
  QVERIFY(qWaitForWindowExposed(&window));
  RepoView *view = window.currentView();
  refresh(view);

  DetailView *detail = view->findChild<DetailView *>();
  detail->setMergeEditor(true);
  QTRY_VERIFY((detail->selectPath("file.txt"),
               detail->property("selectedFile").toString() == "file.txt"));

  QQuickItem *panel = nullptr;
  QTRY_VERIFY((panel = view->findChild<QQuickItem *>("mergePanel")) &&
              panel->isVisible());
  QObject *theirs = panel->findChild<QObject *>("mergeTheirs");
  QVERIFY(theirs);
  auto *model = qobject_cast<QAbstractItemModel *>(
      theirs->property("model").value<QObject *>());
  QVERIFY(model);
  QTRY_VERIFY(model->rowCount() > 0);

  // The header of the conflict is followed by a row that says so.
  auto kind = [model](int row) {
    return model->data(model->index(row, 0), MergeSideModel::KindRole).toInt();
  };
  int header = -1;
  for (int row = 0; row < model->rowCount() && header < 0; ++row) {
    if (kind(row) == MergeSideModel::ConflictRow)
      header = row;
  }
  QCOMPARE(header, 9);
  QCOMPARE(kind(header + 1), int(MergeSideModel::EmptyRow));
  QCOMPARE(kind(header + 2), int(MergeSideModel::CommonRow));

  // The panes scroll together over the rows that they show.
  qreal line = panel->property("lineHeight").toReal();
  QList<QVariant> bounds = panel->property("sideBounds1").toList();
  QCOMPARE(bounds.last().toReal(), model->rowCount() * line);
}

void TestQmlViews::dragTab() {
  // A second tab.
  ScratchRepository other;
  QVERIFY(mWindow->addTab(other));
  QCOMPARE(mWindow->count(), 2);
  mWindow->resize(1000, 700);
  qWait(100);

  QString first = mWindow->tabWidget()->tabText(0);
  QString second = mWindow->tabWidget()->tabText(1);

  // The tabs are at the top of the view of the window.
  QQuickWidget *view = mWindow->quickView();
  QVERIFY(view);

  // Drag the first tab past the second one. The tabs are below the menu
  // bar when the view draws it.
  QPoint start(40, (mWindow->isMenuBarVisible() ? 28 : 0) + 20);
  mousePress(view, Qt::LeftButton, Qt::NoModifier, start);
  for (int x = 10; x <= 240; x += 10)
    mouseMove(view, start + QPoint(x, 0));
  mouseRelease(view, Qt::LeftButton, Qt::NoModifier, start + QPoint(240, 0));
  qWait(100);

  QCOMPARE(mWindow->tabWidget()->tabText(0), second);
  QCOMPARE(mWindow->tabWidget()->tabText(1), first);

  mWindow->tabWidget()->widget(1)->close();
  qWait(100);
}

void TestQmlViews::cleanupTestCase() {
  qInstallMessageHandler(sPrevious);
  mWindow->close();
}

void TestQmlViews::check(QDialog *dialog, const QString &name) {
  sMessages.clear();
  dialog->setAttribute(Qt::WA_DeleteOnClose, false);
  dialog->show();
  QVERIFY2(qWaitForWindowExposed(dialog), qPrintable(name));
  qWait(50);

  QQuickWidget *view = dialog->findChild<QQuickWidget *>();
  QVERIFY2(view, qPrintable(name));
  QVERIFY2(view->status() == QQuickWidget::Ready, qPrintable(name));
  QVERIFY2(view->rootObject(), qPrintable(name));

  // Switching sections creates their content.
  if (SettingsDialog *settings = qobject_cast<SettingsDialog *>(dialog)) {
    for (int i = SettingsDialog::General; i <= SettingsDialog::Terminal; ++i) {
      settings->setSection(i);
      qWait(20);
    }
  } else if (ConfigDialog *config = qobject_cast<ConfigDialog *>(dialog)) {
    for (int i = ConfigDialog::General; i <= ConfigDialog::Lfs; ++i) {
      config->setSection(i);
      qWait(20);
    }
  }

  dialog->hide();
  delete dialog;

  QVERIFY2(sMessages.isEmpty(),
           qPrintable(name + ":\n" + sMessages.join('\n')));
}

TEST_MAIN(TestQmlViews)

#include "qml_views.moc"
