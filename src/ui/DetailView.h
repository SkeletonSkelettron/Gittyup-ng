//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DETAILVIEW_H
#define DETAILVIEW_H

#include "RepoView.h"
#include "git/Diff.h"
#include <QFutureWatcher>
#include <QObject>
#include <QVariantList>

class ChangedFilesModel;
class DiffModel;
class FileViewModel;
class FindController;
class MergeModel;
class SpellCheck;
class CommitTemplates;
class TreeModel;

namespace git {
class Commit;
class Repository;
} // namespace git

// Controller for the right panel of the repository page: the details of the
// selected commits, or the staging area and the commit message for the
// uncommitted changes. It also owns the diff of the selected file shown in
// the middle. qrc:/qml/DetailsPanel.qml and DiffPanel.qml draw it.
class DetailView : public QObject {
  Q_OBJECT

  Q_PROPERTY(int mode READ mode NOTIFY modeChanged)
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(int viewMode READ viewModeValue NOTIFY viewModeChanged)

  // commit details
  Q_PROPERTY(QString summary READ summary NOTIFY commitChanged)
  Q_PROPERTY(QString body READ body NOTIFY commitChanged)
  Q_PROPERTY(QString authorName READ authorName NOTIFY commitChanged)
  Q_PROPERTY(QString authorEmail READ authorEmail NOTIFY commitChanged)
  Q_PROPERTY(QString initials READ initials NOTIFY commitChanged)
  Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY commitChanged)
  Q_PROPERTY(QString committerName READ committerName NOTIFY commitChanged)
  Q_PROPERTY(QString committerEmail READ committerEmail NOTIFY commitChanged)
  Q_PROPERTY(bool sameCommitter READ sameCommitter NOTIFY commitChanged)
  Q_PROPERTY(QString date READ date NOTIFY commitChanged)
  Q_PROPERTY(QString shortId READ shortId NOTIFY commitChanged)
  Q_PROPERTY(QVariantList parents READ parents NOTIFY commitChanged)
  // How the commit is signed, like "GPG" or "SSH", or an empty string.
  Q_PROPERTY(QString signature READ signature NOTIFY commitChanged)
  Q_PROPERTY(QVariantList refs READ refs NOTIFY commitChanged)

  // files
  Q_PROPERTY(QAbstractItemModel *files READ files CONSTANT)
  Q_PROPERTY(QAbstractItemModel *stagedFiles READ stagedFiles CONSTANT)
  Q_PROPERTY(QAbstractItemModel *unstagedFiles READ unstagedFiles CONSTANT)
  Q_PROPERTY(QAbstractItemModel *tree READ tree CONSTANT)
  Q_PROPERTY(QObject *diff READ diffModel CONSTANT)
  // The content of the selected file in tree mode.
  Q_PROPERTY(QObject *content READ contentModel CONSTANT)
  // Finds text in the diff or the content of the selected file.
  Q_PROPERTY(QObject *finder READ finder CONSTANT)
  // The conflicts of the selected file, and whether they are shown in the
  // merge editor instead of the diff.
  Q_PROPERTY(QObject *merge READ mergeModel CONSTANT)
  Q_PROPERTY(bool mergeEditor READ mergeEditor WRITE setMergeEditor NOTIFY
                 mergeEditorChanged)
  Q_PROPERTY(QObject *spellCheck READ spellCheck CONSTANT)
  Q_PROPERTY(QString selectedFile READ file NOTIFY selectedFileChanged)
  Q_PROPERTY(bool listMode READ listMode NOTIFY settingsChanged)
  Q_PROPERTY(bool hideUntracked READ hideUntracked NOTIFY settingsChanged)

  // uncommitted changes
  Q_PROPERTY(QString message READ commitMessage WRITE setCommitMessage NOTIFY
                 messageChanged)
  Q_PROPERTY(QString branchName READ branchName NOTIFY buttonsChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY buttonsChanged)
  Q_PROPERTY(QString commitText READ commitText NOTIFY buttonsChanged)
  Q_PROPERTY(bool canCommit READ isCommitEnabled NOTIFY buttonsChanged)
  Q_PROPERTY(bool canStage READ isStageEnabled NOTIFY buttonsChanged)
  Q_PROPERTY(bool canUnstage READ isUnstageEnabled NOTIFY buttonsChanged)
  Q_PROPERTY(bool rebaseOngoing READ isRebaseContinueVisible NOTIFY
                 buttonsChanged)
  Q_PROPERTY(bool mergeAbortVisible READ isMergeAbortVisible NOTIFY
                 buttonsChanged)
  Q_PROPERTY(QString mergeAbortText READ mergeAbortText NOTIFY buttonsChanged)
  Q_PROPERTY(QString authorText READ authorText NOTIFY authorChanged)
  Q_PROPERTY(bool authorOverridden READ isAuthorOverridden NOTIFY
                 authorChanged)

public:
  // Keep in sync with DetailsPanel.qml.
  enum Mode { NoMode, CommitMode, RangeMode, WipMode };
  enum List { AllFiles, UnstagedFiles, StagedFiles };

  DetailView(const git::Repository &repo, RepoView *view);
  virtual ~DetailView();

  // commit
  void commit(bool force = false);
  bool isCommitEnabled() const { return mCanCommit; }
  bool isRebaseContinueVisible() const { return mRebaseOngoing; }
  bool isRebaseAbortVisible() const { return mRebaseOngoing; }
  bool isMergeAbortVisible() const { return mMergeAbortVisible; }
  QString mergeAbortText() const { return mMergeAbortText; }

  // stage / unstage
  void stage();
  bool isStageEnabled() const { return mCanStage; }
  void unstage();
  bool isUnstageEnabled() const { return mCanUnstage; }

  // mode
  RepoView::ViewMode viewMode() const { return mViewMode; }
  int viewModeValue() const { return mViewMode; }
  void setViewMode(RepoView::ViewMode mode, bool spontaneous);

  QString file() const { return mFile; }

  QString commitMessage() const { return mMessage; }
  void setCommitMessage(const QString &message);
  void setDiff(const git::Diff &diff, const QString &file = QString(),
               const QString &pathspec = QString());
  void setLoading();

  void cancelBackgroundTasks();

  void find();
  void findNext();
  void findPrevious();

  QString overrideUser() const { return mOverrideUser; }
  QString overrideEmail() const { return mOverrideEmail; }

  int mode() const { return mMode; }
  bool isLoading() const { return mLoading; }

  QString summary() const { return mSummary; }
  QString body() const { return mBody; }
  QString authorName() const { return mAuthorName; }
  QString authorEmail() const { return mAuthorEmail; }
  QString initials() const { return mInitials; }
  QString avatarUrl() const { return mAvatarUrl; }
  QString committerName() const { return mCommitterName; }
  QString committerEmail() const { return mCommitterEmail; }
  bool sameCommitter() const { return mSameCommitter; }
  QString date() const { return mDate; }
  QString shortId() const { return mShortId; }
  QVariantList parents() const { return mParents; }
  QString signature() const { return mSignature; }
  QVariantList refs() const { return mRefs; }

  QAbstractItemModel *files() const;
  QAbstractItemModel *stagedFiles() const;
  QAbstractItemModel *unstagedFiles() const;
  QAbstractItemModel *tree() const;
  QObject *diffModel() const;
  QObject *contentModel() const;
  QObject *finder() const;
  QObject *mergeModel() const;
  bool mergeEditor() const;
  void setMergeEditor(bool merge);
  QObject *spellCheck() const;
  bool listMode() const;
  bool hideUntracked() const;

  QString branchName() const { return mBranchName; }
  QString statusText() const { return mStatusText; }
  QString commitText() const { return mCommitText; }
  QString authorText() const;
  bool isAuthorOverridden() const;

  // QML interface
  Q_INVOKABLE void selectFile(int list, int row);
  Q_INVOKABLE void selectPath(const QString &path);
  Q_INVOKABLE void closeFile();
  Q_INVOKABLE void stageFiles(int list, int row, bool staged);
  Q_INVOKABLE void discardFiles(int list, int row);
  Q_INVOKABLE void discardFile(const QString &path);
  Q_INVOKABLE void editFile(const QString &path);
  Q_INVOKABLE void showFileMenu(int list, int row, qreal x, qreal y);
  Q_INVOKABLE void showTreeMenu(const QString &path, qreal x, qreal y);
  Q_INVOKABLE void openTreeFile(const QString &path);
  Q_INVOKABLE void showOptionsMenu(qreal x, qreal y);
  Q_INVOKABLE void showTemplateMenu(qreal x, qreal y);
  Q_INVOKABLE void setListMode(bool list);
  Q_INVOKABLE void selectParent(const QString &id);
  Q_INVOKABLE void copyId();
  Q_INVOKABLE void changeAuthor();
  Q_INVOKABLE void resetAuthor();
  Q_INVOKABLE void commitChanges();
  Q_INVOKABLE void abortMerge();
  Q_INVOKABLE void abortRebase();
  Q_INVOKABLE void continueRebase();

signals:
  void viewModeChanged(RepoView::ViewMode mode, bool spontaneous = false);
  void modeChanged();
  void loadingChanged();
  void commitChanged();
  void selectedFileChanged();
  void mergeEditorChanged();
  void settingsChanged();
  void messageChanged();
  void buttonsChanged();
  void authorChanged();

  // The commit message was filled in automatically, so the view should
  // select it for easy replacement.
  void messagePopulated();

private:
  // Show the content of 'path' at the selected commit.
  bool showContent(const QString &path);

  ChangedFilesModel *model(int list) const;
  void setMode(Mode mode);
  void setCommits(const QList<git::Commit> &commits);
  void setReferences(const QList<git::Commit> &commits);
  void updateButtons(bool yieldFocus = true);
  void updateFiles();
  void populateMessage(const QStringList &files);
  void applyTemplate(const QString &text, const QStringList &files);
  QStringList stagedFileNames() const;

  RepoView *mView;
  git::Repository mRepo;
  git::Diff mDiff;
  QString mFile;
  Mode mMode = NoMode;
  bool mLoading = false;
  RepoView::ViewMode mViewMode = RepoView::DoubleTree;

  ChangedFilesModel *mFiles;
  ChangedFilesModel *mStagedFiles;
  ChangedFilesModel *mUnstagedFiles;
  TreeModel *mTree;
  DiffModel *mDiffModel;
  FileViewModel *mContentModel;
  FindController *mFinder;
  MergeModel *mMerge;
  SpellCheck *mSpellCheck;
  CommitTemplates *mTemplates;

  QString mSummary;
  QString mBody;
  QString mAuthorName;
  QString mAuthorEmail;
  QString mInitials;
  QString mAvatarUrl;
  QString mCommitterName;
  QString mCommitterEmail;
  bool mSameCommitter = true;
  QString mDate;
  QString mShortId;
  QString mId;
  QVariantList mParents;
  QString mSignature;
  QVariantList mRefs;
  QFutureWatcher<QString> mDescription;

  QString mMessage;
  bool mPopulate = true;
  // The status that was committed: its files are stale until the status is
  // refreshed.
  git::Diff mCommitted;
  QString mBranchName;
  QString mStatusText;
  QString mCommitText;
  bool mCanCommit = false;
  bool mCanStage = false;
  bool mCanUnstage = false;
  bool mRebaseOngoing = false;
  bool mMergeAbortVisible = false;
  QString mMergeAbortText;

  QString mOverrideUser;
  QString mOverrideEmail;
};

#endif
