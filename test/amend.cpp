#include "Test.h"

#include "qtsupport.h"
#include "git/Signature.h"
#include "git/Reference.h"
#include "git/Tree.h"
#include "ui/MainWindow.h"
#include "ui/DetailView.h"
#include "ui/RepoView.h"
#include "dialogs/AmendDialog.h"


#define INIT_REPO(repoPath)                                                    \
  QString path = Test::extractRepository(repoPath);                            \
  QVERIFY(!path.isEmpty());                                                    \
  git::Repository repo = git::Repository::open(path);                          \
  QVERIFY(repo.isValid());                                                     \
  Test::initRepo(repo);

class TestAmend : public QObject {
  Q_OBJECT

private slots:
  void testAmend();
  void testAmendAddFile();
  void testAmendDialog();
  void testAmendDialog2();
  void testAmendDialogNewLineInMessage();
};

using namespace git;

void TestAmend::testAmend() {
  INIT_REPO("CherryPickAuthorEmail.zip");

  git::Reference master = repo.lookupRef(QString("refs/heads/master"));
  QVERIFY(master.isValid());
  auto c = master.annotatedCommit().commit();
  QCOMPARE(c.message(), "changes");
  QCOMPARE(c.author().email(), "martin.marmsoler@gmail.com");
  QCOMPARE(c.author().name(), "Martin Marmsoler");
  QCOMPARE(
      c.author().date(),
      QDateTime::fromString("Sun May 22 10:36:26 2022 +0200", Qt::RFC2822Date));
  QCOMPARE(c.committer().name(), "Martin Marmsoler");
  QCOMPARE(c.committer().email(), "martin.marmsoler@gmail.com");
  QCOMPARE(
      c.committer().date(),
      QDateTime::fromString("Sun May 22 10:36:26 2022 +0200", Qt::RFC2822Date));

  const QString commitMessage = "New commit message";

  auto authorSignature = repo.signature(
      "New Author", "New Author Email",
      QDateTime::fromString("Mon May 23 10:36:26 2022 +0200", Qt::RFC2822Date));

  QString dateAuthor = "Mon May 23 10:36:26 2022 +0200";
  auto committerSignature = repo.signature(
      "New Committer", "New Committer Email",
      QDateTime::fromString("Mon May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  Tree tree;
  c.amend(authorSignature, committerSignature, commitMessage, tree);

  master = repo.lookupRef(QString("refs/heads/master"));
  QVERIFY(master.isValid());
  c = master.annotatedCommit().commit();
  QCOMPARE(c.message(), "New commit message");
  QCOMPARE(c.author().email(), "New Author Email");
  QCOMPARE(c.author().name(), "New Author");
  QCOMPARE(
      c.author().date(),
      QDateTime::fromString("Mon May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  QCOMPARE(c.committer().name(), "New Committer");
  QCOMPARE(c.committer().email(), "New Committer Email");
  QCOMPARE(
      c.committer().date(),
      QDateTime::fromString("Mon May 23 11:36:26 2022 +0200", Qt::RFC2822Date));
}

void TestAmend::testAmendAddFile() {

  // Create repo
  Test::ScratchRepository mRepo;
  auto mMainBranch = mRepo->unbornHeadName();
  auto mWindow = new MainWindow(mRepo);
  mWindow->show();
  QVERIFY(QTest::qWaitForWindowExposed(mWindow));
  RepoView *view = mWindow->currentView();

  {
    // Add file and refresh.
    QFile file(mRepo->workdir().filePath("test"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "This will be a test." << Qt::endl;

    Test::refresh(view);

    auto details = view->findChild<DetailView *>();
    QVERIFY(details);

    QAbstractItemModel *model = details->unstagedFiles();
    QCOMPARE(model->rowCount(), 1);

    // Stage the file.
    details->stageFiles(DetailView::UnstagedFiles, 0, true);

    // Commit and refresh.
    details->setCommitMessage("base commit");
    view->commit();
    Test::refresh(view, false);

    // Create branch and stage changes
    git::Branch branch2 =
        mRepo->createBranch("branch2", mRepo->head().target());
    QVERIFY(branch2.isValid());

    view->checkout(branch2);
    QCOMPARE(mRepo->head().name(), QString("branch2"));

    // Check if file has correct content
    QVERIFY(branch2.isValid());
    auto c = branch2.annotatedCommit().commit();
    QCOMPARE(c.blob("test").content(), "This will be a test.\n");
  }

  // Stage file with changes
  {
    QFile file(mRepo->workdir().filePath("test"));
    QVERIFY(file.open(QFile::WriteOnly));
    QTextStream(&file) << "Changes made" << Qt::endl;

    Test::refresh(view);

    auto details = view->findChild<DetailView *>();
    QVERIFY(details);

    // Staging the file
    QAbstractItemModel *model = details->unstagedFiles();
    QCOMPARE(model->rowCount(), 1);
    details->stageFiles(DetailView::UnstagedFiles, 0, true);
  }

  // Check that changes applied after amending
  {
    // Amend changes
    git::Reference branch2 = mRepo->lookupRef(QString("refs/heads/branch2"));
    QVERIFY(branch2.isValid());
    auto c = branch2.annotatedCommit().commit();
    auto authorSignature = mRepo->signature("New Author", "New Author Email");
    auto committerSignature =
        mRepo->signature("New Committer", "New Committer Email");
    QCOMPARE(mRepo->amend(c, authorSignature, committerSignature,
                          "New commit message"),
             true);

    {
      branch2 = mRepo->lookupRef(QString("refs/heads/branch2"));
      QVERIFY(branch2.isValid());
      c = branch2.annotatedCommit().commit();
      QCOMPARE(c.message(), "New commit message");
      QCOMPARE(c.author().email(), "New Author Email");
      QCOMPARE(c.author().name(), "New Author");
      QCOMPARE(c.committer().name(), "New Committer");
      QCOMPARE(c.committer().email(), "New Committer Email");

      QCOMPARE(c.blob("test").content(), "Changes made\n");
    }
  }
}

void TestAmend::testAmendDialog() {
  // Checking that original information is passed to the return function
  // correctly Name and email will not be changed. Only different datetypes are
  // tested
  Test::ScratchRepository repo;
  auto authorSignature = repo->signature(
      "New Author", "New Author Email",
      QDateTime::fromString("Mon May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  auto committerSignature = repo->signature(
      "New Committer", "New Committer Email",
      QDateTime::fromString("Mon May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  {
    AmendDialog d(authorSignature, committerSignature, "Test commit message");
    d.show();

    using Type = ContributorInfo::SelectedDateTimeType;
    struct Case {
      AmendContributor *contributor;
      ContributorInfo (*info)(const AmendInfo &);
      QString original;
      QDateTime manual;
    };

    QList<Case> cases = {
        {d.author(), [](const AmendInfo &info) { return info.authorInfo; },
         "Mon May 23 10:36:26 2022 +0200",
         QDateTime(QDate(2012, 7, 6), QTime(8, 30, 5))},
        {d.committer(),
         [](const AmendInfo &info) { return info.committerInfo; },
         "Mon May 23 11:36:26 2022 +0200",
         QDateTime(QDate(2013, 5, 2), QTime(11, 22, 7))}};

    for (const Case &c : cases) {
      QVERIFY(c.contributor);
      c.contributor->setManualDate(c.manual);

      // current
      c.contributor->setDateType(Type::Current);
      QCOMPARE(c.info(d.getInfo()).commitDateType, Type::Current);

      // original
      c.contributor->setDateType(Type::Original);
      auto info = c.info(d.getInfo());
      QCOMPARE(info.commitDateType, Type::Original);
      QCOMPARE(info.commitDate,
               QDateTime::fromString(c.original, Qt::RFC2822Date));

      // manual
      c.contributor->setDateType(Type::Manual);
      info = c.info(d.getInfo());
      QCOMPARE(info.commitDateType, Type::Manual);
      QCOMPARE(info.commitDate, c.manual);
      QVERIFY(d.isAcceptable());

      // An invalid manual date can't be used.
      c.contributor->setDateText("not a date");
      QVERIFY(!c.contributor->isDateValid());
      QVERIFY(!d.isAcceptable());
      c.contributor->setManualDate(c.manual);
    }

    auto info = d.getInfo();
    QCOMPARE(info.authorInfo.name, "New Author");
    QCOMPARE(info.authorInfo.email, "New Author Email");
    QCOMPARE(info.committerInfo.name, "New Committer");
    QCOMPARE(info.committerInfo.email, "New Committer Email");
    QCOMPARE(info.commitMessage, "Test commit message");
  }
}

void TestAmend::testAmendDialog2() {
  // Test changing author name, author email, committer name, committer email

  Test::ScratchRepository repo;
  auto authorSignature = repo->signature(
      "New Author", "New Author Email",
      QDateTime::fromString("Mon May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  auto committerSignature = repo->signature(
      "New Committer", "New Committer Email",
      QDateTime::fromString("Mon May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  AmendDialog d(authorSignature, committerSignature, "Test commit message");

  d.setCommitMessage("Changing the commit message");

  // Author
  d.author()->setName("Another author name");
  d.author()->setEmail("Another author email address");

  // Committer
  d.committer()->setName("Another committer name");
  d.committer()->setEmail("Another committer email address");

  const auto info = d.getInfo();

  QCOMPARE(info.authorInfo.name, "Another author name");
  QCOMPARE(info.authorInfo.email, "Another author email address");
  QCOMPARE(info.committerInfo.name, "Another committer name");
  QCOMPARE(info.committerInfo.email, "Another committer email address");
  QCOMPARE(info.commitMessage, "Changing the commit message");
}

void TestAmend::testAmendDialogNewLineInMessage() {
  // Test changing author name, author email, committer name, committer email

  Test::ScratchRepository repo;
  auto authorSignature = repo->signature(
      "New Author", "New Author Email",
      QDateTime::fromString("Mon May 23 10:36:26 2022 +0200", Qt::RFC2822Date));
  auto committerSignature = repo->signature(
      "New Committer", "New Committer Email",
      QDateTime::fromString("Mon May 23 11:36:26 2022 +0200", Qt::RFC2822Date));

  AmendDialog d(authorSignature, committerSignature,
                "Test commit message\nNewLine");

  QCOMPARE(d.getInfo().commitMessage, "Test commit message\nNewLine");
}

TEST_MAIN(TestAmend)
#include "amend.moc"
