//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//          Copyright (c) 2023, Gittyup Contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DetailView.h"
#include "qml/QmlSupport.h"
#include "SpellCheck.h"
#include "ChangedFilesModel.h"
#include "CommitMessage.h"
#include "DiffModel.h"
#include "FileContextMenu.h"
#include "FileViewModel.h"
#include "FindController.h"
#include "MergeModel.h"
#include "MenuBar.h"
#include "CommitTemplates.h"
#include "TreeModel.h"
#include "conf/Settings.h"
#include "dialogs/InputDialog.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "git/Repository.h"
#include "git/Blob.h"
#include "git/Signature.h"
#include "git/Tree.h"
#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QMenu>
#include <QSettings>
#include <QRegularExpression>
#include <QtConcurrent>

namespace {

const QString kAvatarUrl = "https://www.gravatar.com/avatar/%1?s=%2&d=mm";

QString initialsOf(const QString &name) {
  QStringList parts = name.split(' ', Qt::SkipEmptyParts);
  QString initials;
  if (!parts.isEmpty())
    initials += parts.first().left(1);
  if (parts.size() > 1)
    initials += parts.last().left(1);
  return initials.toUpper();
}

// Join the lines of paragraphs that were wrapped for a terminal, so that the
// text wraps to the width of the panel. Lists, indented lines and trailers
// like 'Signed-off-by:' keep their lines.
QString reflow(const QString &text) {
  static const QRegularExpression kSeparate(
      "^(\\s|[-*+]\\s|\\d+[.)]\\s|[A-Za-z][A-Za-z-]*:\\s)");

  QStringList lines = text.split('\n');
  QString result;
  for (int i = 0; i < lines.size(); ++i) {
    const QString &line = lines.at(i);
    if (i > 0) {
      const QString &previous = lines.at(i - 1);
      bool join = !previous.trimmed().isEmpty() &&
                  !line.trimmed().isEmpty() &&
                  !previous.at(0).isSpace() &&
                  !kSeparate.match(line).hasMatch();
      result += join ? QChar(' ') : QChar('\n');
    }

    result += line;
  }

  return result;
}

} // namespace

DetailView::DetailView(const git::Repository &repo, RepoView *view)
    : QObject(view), mView(view), mRepo(repo),
      mFiles(new ChangedFilesModel(ChangedFilesModel::All, this)),
      mStagedFiles(new ChangedFilesModel(ChangedFilesModel::Staged, this)),
      mUnstagedFiles(new ChangedFilesModel(ChangedFilesModel::Unstaged, this)),
      mTree(new TreeModel(repo, this)), mDiffModel(new DiffModel(view, this)),
      mContentModel(new FileViewModel(repo, this)),
      mSpellCheck(new SpellCheck(repo, view)) {
  connect(mContentModel, &FileViewModel::linkActivated, view,
          &RepoView::visitLink);

  // The merge editor shows the conflicts of the selected file.
  mMerge = new MergeModel(view, this);
  connect(mDiffModel, &DiffModel::diffChanged, this, [this] {
    if (mDiffModel->isConflicted())
      mMerge->load(mDiffModel->path());
    else
      mMerge->clear();
  });

  // Find in the diff, or in the content in tree mode.
  mFinder = new FindController(
      [this]() -> FindTarget * {
        if (mFile.isEmpty())
          return nullptr;
        if (mViewMode == RepoView::Tree)
          return mContentModel;
        return mDiffModel;
      },
      this);
  connect(this, &DetailView::selectedFileChanged, mFinder,
          &FindController::refresh);
  connect(mDiffModel, &QAbstractItemModel::modelReset, mFinder,
          &FindController::refresh);
  connect(mContentModel, &QAbstractItemModel::modelReset, mFinder,
          &FindController::refresh);

  mTemplates = new CommitTemplates(this);
  connect(mTemplates, &CommitTemplates::templateChanged, this,
          [this](const QString &text) {
            applyTemplate(text, stagedFileNames());
          });

  connect(&mDescription, &QFutureWatcher<QString>::finished, this, [this] {
    QString result = mDescription.result();
    if (result.contains('+')) {
      mRefs.append(QVariantMap{{"name", result}, {"tag", true}});
      emit commitChanged();
    }
  });

  git::RepositoryNotifier *notifier = repo.notifier();
  auto resetRefs = [this] {
    if (mMode == CommitMode || mMode == RangeMode) {
      setReferences(mView->commits());
      emit commitChanged();
    }
  };

  connect(notifier, &git::RepositoryNotifier::referenceAdded, this, resetRefs);
  connect(notifier, &git::RepositoryNotifier::referenceRemoved, this,
          resetRefs);
  connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
          resetRefs);

  // Update the staging area when the index changes.
  connect(notifier, &git::RepositoryNotifier::indexChanged, this,
          [this](const QStringList &paths, bool yieldFocus) {
            if (mMode != WipMode)
              return;

            mStagedFiles->refresh();
            mUnstagedFiles->refresh();
            updateButtons(yieldFocus);
          });

  // Update the rebase and merge buttons when the repository state changes.
  auto updateState = [this] { updateButtons(false); };
  connect(notifier, &git::RepositoryNotifier::stateChanged, this, updateState);
  connect(notifier, &git::RepositoryNotifier::rebaseFinished, this,
          updateState);
  connect(notifier, &git::RepositoryNotifier::rebaseConflict, this,
          updateState);

  connect(Settings::instance(), &Settings::settingsChanged, this, [this] {
    updateFiles();
    emit settingsChanged();
  });

  mCommitText = tr("Commit");
}

DetailView::~DetailView() {}

QAbstractItemModel *DetailView::files() const { return mFiles; }

QAbstractItemModel *DetailView::stagedFiles() const { return mStagedFiles; }

QAbstractItemModel *DetailView::unstagedFiles() const {
  return mUnstagedFiles;
}

QAbstractItemModel *DetailView::tree() const { return mTree; }

QObject *DetailView::diffModel() const { return mDiffModel; }

QObject *DetailView::contentModel() const { return mContentModel; }

QObject *DetailView::finder() const { return mFinder; }

QObject *DetailView::mergeModel() const { return mMerge; }

bool DetailView::mergeEditor() const {
  return QSettings().value("diff/mergeEditor", false).toBool();
}

void DetailView::setMergeEditor(bool merge) {
  if (merge == mergeEditor())
    return;

  QSettings().setValue("diff/mergeEditor", merge);
  emit mergeEditorChanged();
}

QObject *DetailView::spellCheck() const { return mSpellCheck; }

bool DetailView::listMode() const {
  return Settings::instance()
      ->value(Setting::Id::ShowChangedFilesAsList, false)
      .toBool();
}

bool DetailView::hideUntracked() const {
  return Settings::instance()->value(Setting::Id::HideUntracked, false).toBool();
}

ChangedFilesModel *DetailView::model(int list) const {
  switch (list) {
    case UnstagedFiles:
      return mUnstagedFiles;
    case StagedFiles:
      return mStagedFiles;
    default:
      return mFiles;
  }
}

void DetailView::commit(bool force) {
  // Check for a merge head.
  git::AnnotatedCommit upstream;
  if (git::Reference mergeHead = mRepo.lookupRef("MERGE_HEAD"))
    upstream = mergeHead.annotatedCommit();

  if (mView->commit(mMessage, upstream, nullptr, force)) {
    mCommitted = mDiff;
    mPopulate = true;
    setCommitMessage(QString());
    updateButtons(false);
  }
}

void DetailView::commitChanges() {
  if (mCanCommit)
    commit();
}

void DetailView::stage() {
  if (mDiff.isValid())
    mDiff.setAllStaged(true);
}

void DetailView::unstage() {
  if (mDiff.isValid())
    mDiff.setAllStaged(false);
}

void DetailView::setViewMode(RepoView::ViewMode mode, bool spontaneous) {
  if (mode == mViewMode)
    return;

  mViewMode = mode;

  // The file is shown differently in the other mode.
  closeFile();

  // Emit own signal so that the view can respond *after* the change.
  emit viewModeChanged(mode, spontaneous);
}

void DetailView::setCommitMessage(const QString &message) {
  if (message == mMessage)
    return;

  bool wasEmpty = mMessage.isEmpty();
  mMessage = message;

  // Stop filling in the message once the user writes one.
  mPopulate = message.isEmpty();

  emit messageChanged();
  if (wasEmpty != message.isEmpty())
    updateButtons(false);
}

void DetailView::setMode(Mode mode) {
  if (mode == mMode)
    return;

  mMode = mode;
  emit modeChanged();
}

void DetailView::setDiff(const git::Diff &diff, const QString &file,
                         const QString &pathspec) {
  Q_UNUSED(pathspec)

  QList<git::Commit> commits = mView->commits();
  mDiff = diff;
  if (!(diff == mCommitted))
    mCommitted = git::Diff();

  if (mLoading) {
    mLoading = false;
    emit loadingChanged();
  }

  if (!commits.isEmpty())
    setCommits(commits);

  Mode mode = !diff.isValid()       ? NoMode
              : commits.isEmpty()   ? WipMode
              : commits.size() == 1 ? CommitMode
                                    : RangeMode;
  setMode(mode);
  updateFiles();
  updateButtons(false);

  if (mode == WipMode) {

    // Pre-populate the commit message with the merge message.
    QString msg = mRepo.message();
    if (!msg.isEmpty())
      setCommitMessage(msg);
  }

  // Keep showing the same file if it's still there.
  QString path = !file.isEmpty() ? file : mFile;
  if (mViewMode == RepoView::Tree) {
    if (!path.isEmpty() && !showContent(path))
      path.clear();
    if (path.isEmpty())
      mContentModel->clear();
    mDiffModel->setDiff(git::Diff(), QString());
  } else {
    if (!diff.isValid() || diff.indexOf(path) < 0)
      path.clear();
    mDiffModel->setDiff(diff, path);
  }

  if (path != mFile) {
    mFile = path;
    emit selectedFileChanged();
  }

  // Update menu actions.
  MenuBar::instance(mView)->updateRepository();
}

void DetailView::setLoading() {
  // Commit metadata comes from the selected commits, not from the diff, so
  // it can be shown immediately.
  QList<git::Commit> commits = mView->commits();
  if (!commits.isEmpty()) {
    setCommits(commits);
    setMode(commits.size() == 1 ? CommitMode : RangeMode);
  }

  mDiff = git::Diff();
  updateFiles();

  mLoading = true;
  emit loadingChanged();
}

void DetailView::cancelBackgroundTasks() { mDescription.waitForFinished(); }

void DetailView::find() {
  if (!mFile.isEmpty())
    mFinder->show();
}

void DetailView::findNext() {
  if (!mFile.isEmpty())
    mFinder->next();
}

void DetailView::findPrevious() {
  if (!mFile.isEmpty())
    mFinder->previous();
}

void DetailView::updateFiles() {
  bool list = listMode();
  bool hide = hideUntracked();
  for (ChangedFilesModel *model : {mFiles, mStagedFiles, mUnstagedFiles}) {
    model->setListMode(list);
    model->setHideUntracked(hide);
  }

  bool wip = mDiff.isValid() && mDiff.isStatusDiff();
  mFiles->setDiff(wip ? git::Diff() : mDiff);
  mStagedFiles->setDiff(wip ? mDiff : git::Diff());
  mUnstagedFiles->setDiff(wip ? mDiff : git::Diff());

  // The complete tree of the commit, or of HEAD for the uncommitted changes.
  git::Tree tree;
  QList<git::Commit> commits = mView->commits();
  if (!commits.isEmpty()) {
    tree = commits.first().tree();
  } else if (git::Reference head = mRepo.head()) {
    tree = head.target().tree();
  }
  mTree->setTree(tree, mDiff);
}

void DetailView::setCommits(const QList<git::Commit> &commits) {
  mSummary.clear();
  mBody.clear();
  mAuthorName.clear();
  mAuthorEmail.clear();
  mInitials.clear();
  mAvatarUrl.clear();
  mCommitterName.clear();
  mCommitterEmail.clear();
  mSameCommitter = true;
  mDate.clear();
  mShortId.clear();
  mId.clear();
  mParents.clear();
  mSignature.clear();

  setReferences(commits);

  if (commits.size() > 1) {
    // Show range details.
    git::Commit last = commits.last();
    git::Commit first = commits.first();

    QSet<QString> authors;
    for (const git::Commit &commit : commits)
      authors.insert(commit.author().name());
    QStringList names = authors.values();
    names.sort();
    if (names.size() > 3)
      names = names.mid(0, 3) << "...";
    mAuthorName = names.join(", ");

    QDate lastDate = last.committer().date().toLocalTime().date();
    QDate firstDate = first.committer().date().toLocalTime().date();
    QString lastStr = QLocale().toString(lastDate, QLocale::ShortFormat);
    QString firstStr = QLocale().toString(firstDate, QLocale::ShortFormat);
    mDate = (lastDate == firstDate) ? lastStr
                                    : QString("%1 - %2").arg(lastStr, firstStr);

    mSummary = tr("%1 commits").arg(commits.size());
    mShortId = QString("%1..%2").arg(last.shortId(), first.shortId());
    mId = QString("%1..%2").arg(last.id().toString(), first.id().toString());
    emit commitChanged();
    return;
  }

  if (commits.isEmpty()) {
    emit commitChanged();
    return;
  }

  git::Commit commit = commits.first();
  git::Signature author = commit.author();
  git::Signature committer = commit.committer();
  mSummary = commit.summary(git::Commit::SubstituteEmoji);
  mBody = reflow(commit.body(git::Commit::SubstituteEmoji).trimmed());
  mAuthorName = author.name();
  mAuthorEmail = author.email();
  mInitials = initialsOf(author.name());
  mCommitterName = committer.name();
  mCommitterEmail = committer.email();
  mSameCommitter =
      author.name() == committer.name() && author.email() == committer.email();
  mDate = QLocale().toString(committer.date().toLocalTime(),
                             QLocale::LongFormat);
  mShortId = commit.shortId();
  mId = commit.id().toString();
  mSignature = commit.signatureKind();

  for (const git::Commit &parent : commit.parents()) {
    mParents.append(QVariantMap{{"id", parent.id().toString()},
                                {"shortId", parent.shortId()}});
  }

  if (Settings::instance()->value(Setting::Id::ShowAvatars).toBool()) {
    QByteArray email = author.email().trimmed().toLower().toUtf8();
    QByteArray hash = QCryptographicHash::hash(email, QCryptographicHash::Md5);
    mAvatarUrl = kAvatarUrl.arg(QString::fromLatin1(hash.toHex())).arg(96);
  }

  emit commitChanged();
}

void DetailView::setReferences(const QList<git::Commit> &commits) {
  mRefs.clear();
  for (const git::Commit &commit : commits) {
    for (const git::Reference &ref : commit.refs()) {
      mRefs.append(QVariantMap{{"name", ref.name()},
                               {"head", ref.isHead()},
                               {"tag", ref.isTag()},
                               {"local", ref.isLocalBranch()},
                               {"remote", ref.isRemoteBranch()}});
    }
  }

  // Compute the description asynchronously.
  if (commits.size() == 1)
    mDescription.setFuture(
        QtConcurrent::run(&git::Commit::description, commits.first()));
}

QStringList DetailView::stagedFileNames() const {
  QStringList files;
  if (!mDiff.isValid())
    return files;

  git::Index index = mDiff.index();
  for (int i = 0; i < mDiff.count(); ++i) {
    QString name = mDiff.name(i);
    switch (index.isStaged(name)) {
      case git::Index::PartiallyStaged:
      case git::Index::Staged:
        files.append(QFileInfo(name).fileName());
        break;
      default:
        break;
    }
  }

  return files;
}

void DetailView::populateMessage(const QStringList &files) {
  QString msg;
  QList<CommitTemplates::Template> templates = mTemplates->templates();
  if (!templates.isEmpty()) {
    msg = CommitMessage::applyTemplate(templates.first().value, files).text;
  } else {
    msg = CommitMessage::fileList(files, 3);
    if (!msg.isEmpty())
      msg = QStringLiteral("Update ") + msg;
  }

  // Set it directly, this is called while updating the buttons. The
  // message was generated, so keep generating it.
  if (msg != mMessage) {
    mMessage = msg;
    emit messageChanged();
    emit messagePopulated();
  }

  mPopulate = true;
}

void DetailView::applyTemplate(const QString &text, const QStringList &files) {
  CommitMessage::Result result = CommitMessage::applyTemplate(text, files);
  setCommitMessage(result.text);
}

void DetailView::updateButtons(bool yieldFocus) {
  Q_UNUSED(yieldFocus)

  mRebaseOngoing = mRepo.isValid() && mRepo.rebaseOngoing();

  bool merging = false;
  QString text = tr("Merge");
  switch (mRepo.state()) {
    case GIT_REPOSITORY_STATE_MERGE:
      merging = true;
      break;

    case GIT_REPOSITORY_STATE_REVERT:
    case GIT_REPOSITORY_STATE_REVERT_SEQUENCE:
      merging = true;
      text = tr("Revert");
      break;

    case GIT_REPOSITORY_STATE_CHERRYPICK:
    case GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE:
      merging = true;
      text = tr("Cherry-pick");
      break;

    case GIT_REPOSITORY_STATE_REBASE:
    case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
    case GIT_REPOSITORY_STATE_REBASE_MERGE:
      text = tr("Rebase");
      break;
  }

  git::Reference head = mRepo.head();
  git::Branch headBranch = head;
  mBranchName = head.isValid() ? head.name() : mRepo.unbornHeadName();
  mMergeAbortText = tr("Abort %1").arg(text);
  mMergeAbortVisible = headBranch.isValid() && merging;

  bool wip = mDiff.isValid() && mDiff.isStatusDiff();
  if (!wip) {
    mCanStage = false;
    mCanUnstage = false;
    mCanCommit = false;
    mStatusText.clear();
    mCommitText = tr("Commit");
    emit buttonsChanged();
    MenuBar::instance(mView)->updateRepository();
    return;
  }

  QStringList files;
  int staged = 0;
  int partial = 0;
  int conflicted = 0;
  int count = mDiff.count();
  git::Index index = mDiff.index();
  if (index.isValid()) {
    for (int i = 0; i < count; ++i) {
      QString name = mDiff.name(i);
      switch (index.isStaged(name)) {
        case git::Index::Disabled:
        case git::Index::Unstaged:
          break;

        case git::Index::PartiallyStaged:
          files.append(QFileInfo(name).fileName());
          ++partial;
          break;

        case git::Index::Staged:
          files.append(QFileInfo(name).fileName());
          ++staged;
          break;

        case git::Index::Conflicted:
          ++conflicted;
          break;
      }
    }
  }

  // Don't commit the files of the last commit again.
  bool committed = mCommitted.isValid() && mDiff == mCommitted;
  if (mPopulate && !committed)
    populateMessage(files);

  int total = staged + partial + conflicted;
  mCanStage = count > staged;
  mCanUnstage = total;

  // Set status text.
  QString status = tr("Nothing staged");
  if (staged || partial || conflicted) {
    QString fmt = (staged == 1 && count == 1) ? tr("%1 of %2 file staged")
                                              : tr("%1 of %2 files staged");
    QStringList fragments(fmt.arg(staged).arg(count));

    if (partial) {
      QString partialFmt = (partial == 1) ? tr("%1 file partially staged")
                                          : tr("%1 files partially staged");
      fragments.append(partialFmt.arg(partial));
    }

    if (conflicted) {
      QString conflictedFmt = (conflicted == 1) ? tr("%1 unresolved conflict")
                                                : tr("%1 unresolved conflicts");
      fragments.append(conflictedFmt.arg(conflicted));
    } else if (mDiff.isConflicted()) {
      fragments.append(tr("all conflicts resolved"));
    }

    status = fragments.join(", ");
  }

  mStatusText = status;

  // Change the commit button for committing a merge or a rebase.
  bool hasMessage = !mMessage.trimmed().isEmpty();
  switch (mRepo.state()) {
    case GIT_REPOSITORY_STATE_MERGE:
      mCommitText = tr("Commit Merge");
      mCanCommit = total && hasMessage;
      break;
    case GIT_REPOSITORY_STATE_REBASE:
    case GIT_REPOSITORY_STATE_REBASE_MERGE:
    case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
      mCommitText = tr("Commit Rebase");
      mCanCommit = total && conflicted == 0 && hasMessage;
      break;
    default: {
      int files = staged + partial;
      mCommitText = files > 0 ? tr("Commit Changes to %n File(s)", "", files)
                              : tr("Commit");
      mCanCommit = total && hasMessage;
      break;
    }
  }

  if (committed)
    mCanCommit = false;

  emit buttonsChanged();

  // Update menu actions.
  MenuBar::instance(mView)->updateRepository();
}

QString DetailView::authorText() const {
  git::Config config = mRepo.gitConfig();
  QString name = mOverrideUser.isEmpty()
                     ? config.value<QString>("user.name")
                     : mOverrideUser;
  QString email = mOverrideEmail.isEmpty()
                      ? config.value<QString>("user.email")
                      : mOverrideEmail;
  return QString("%1 <%2>").arg(name, email);
}

bool DetailView::isAuthorOverridden() const {
  return !mOverrideUser.isEmpty() || !mOverrideEmail.isEmpty();
}

void DetailView::selectFile(int list, int row) {
  ChangedFilesModel *files = model(list);
  if (files->isDir(row)) {
    files->toggle(row);
    return;
  }

  selectPath(files->path(row));
}

void DetailView::selectPath(const QString &path) {
  if (path.isEmpty())
    return;

  // Tree mode shows the content of the file, like the diff mode its diff.
  if (mViewMode == RepoView::Tree) {
    if (!showContent(path))
      return;
  } else {
    if (!mDiff.isValid() || mDiff.indexOf(path) < 0)
      return;
    mDiffModel->setDiff(mDiff, path);
  }

  if (path != mFile) {
    mFile = path;
    emit selectedFileChanged();
  }
}

void DetailView::closeFile() {
  if (mFile.isEmpty())
    return;

  mFinder->hide();
  mFile.clear();
  mDiffModel->setDiff(git::Diff(), QString());
  mContentModel->clear();
  emit selectedFileChanged();
}

bool DetailView::showContent(const QString &path) {
  // The selected commit, or the working copy for the uncommitted changes.
  QList<git::Commit> commits = mView->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
  bool exists = commit.isValid()
                    ? commit.blob(path).isValid()
                    : QFileInfo(mRepo.workdir().filePath(path)).isFile();
  if (!exists)
    return false;

  mContentModel->load(path, commit);
  return true;
}

void DetailView::stageFiles(int list, int row, bool staged) {
  QStringList files = (row < 0) ? model(list)->allFiles() : model(list)->files(row);
  if (!files.isEmpty() && mDiff.isValid())
    mDiff.index().setStaged(files, staged);
}

void DetailView::discardFiles(int list, int row) {
  QStringList files = model(list)->files(row);
  if (files.isEmpty())
    return;

  FileContextMenu menu(mView, files, git::Index());
  if (QAction *discard = menu.discardAction())
    discard->trigger();
}

void DetailView::discardFile(const QString &path) {
  if (path.isEmpty())
    return;

  FileContextMenu menu(mView, {path}, git::Index());
  if (QAction *discard = menu.discardAction())
    discard->trigger();
}

void DetailView::editFile(const QString &path) {
  if (!path.isEmpty())
    mView->edit(path);
}

void DetailView::showFileMenu(int list, int row, qreal x, qreal y) {
  QStringList files = model(list)->files(row);
  if (files.isEmpty())
    return;

  FileContextMenu menu(mView, files, git::Index());
  QmlSupport::execMenu(&menu, mView->mapFromPage(x, y));
}

void DetailView::showTreeMenu(const QString &path, qreal x, qreal y) {
  if (path.isEmpty())
    return;

  FileContextMenu menu(mView, {path}, git::Index());
  QmlSupport::execMenu(&menu, mView->mapFromPage(x, y));
}

void DetailView::openTreeFile(const QString &path) {
  QList<git::Commit> commits = mView->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
  git::Blob blob = commit.isValid() ? commit.blob(path) : git::Blob();
  mView->openEditor(path, -1, blob, commit);
}

void DetailView::showOptionsMenu(qreal x, qreal y) {
  Settings *settings = Settings::instance();

  QMenu menu;
  QAction *list = menu.addAction(tr("Show as List"), [this](bool checked) {
    setListMode(checked);
  });
  list->setCheckable(true);
  list->setChecked(listMode());

  QAction *untracked =
      menu.addAction(tr("Hide Untracked Files"), [settings](bool checked) {
        settings->setValue(Setting::Id::HideUntracked, checked);
      });
  untracked->setCheckable(true);
  untracked->setChecked(hideUntracked());

  QmlSupport::execMenu(&menu, mView->mapFromPage(x, y));
}

void DetailView::showTemplateMenu(qreal x, qreal y) {
  mTemplates->showMenu(mView->mapFromPage(x, y), mView);
}

void DetailView::setListMode(bool list) {
  Settings::instance()->setValue(Setting::Id::ShowChangedFilesAsList, list);
}

void DetailView::selectParent(const QString &id) {
  QUrl url;
  url.setScheme("id");
  url.setPath(id);
  mView->visitLink(url.toString());
}

void DetailView::copyId() { QApplication::clipboard()->setText(mId); }

void DetailView::changeAuthor() {
  // Empty fields keep the author from the configuration.
  git::Config config = mRepo.gitConfig();
  InputDialog *dialog = new InputDialog(
      tr("Change Author"),
      tr("Set the author of the next commits. The change isn't saved "
         "permanently."),
      {{tr("Name"), mOverrideUser, false, false,
        config.value<QString>("user.name")},
       {tr("Email"), mOverrideEmail, false, false,
        config.value<QString>("user.email")}},
      mView, tr("Change Author"));
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, &QDialog::accepted, this, [this, dialog] {
    mOverrideUser = dialog->value(0);
    mOverrideEmail = dialog->value(1);
    emit authorChanged();
  });

  dialog->open();
}

void DetailView::resetAuthor() {
  mOverrideUser.clear();
  mOverrideEmail.clear();
  emit authorChanged();
}

void DetailView::abortMerge() { mView->mergeAbort(); }

void DetailView::abortRebase() { mView->abortRebase(); }

void DetailView::continueRebase() { mView->continueRebase(); }
