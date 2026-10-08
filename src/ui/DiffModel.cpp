//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "DiffModel.h"
#include "editor/TextEditor.h"
#include "qml/QmlSupport.h"
#include "dialogs/ConfirmDialog.h"
#include "RepoView.h"
#include "app/Application.h"
#include "app/Theme.h"
#include "conf/Constants.h"
#include "conf/Settings.h"
#include "git/Blob.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "git/Reference.h"
#include "git/Repository.h"
#include "git/Tree.h"
#include "git2/diff.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QSaveFile>
#include <QTextStream>

namespace {

// Diffs with more lines are only loaded on request.
const int kMaxLines = 20000;

QByteArray chomp(const QByteArray &line) {
  QByteArray result = line;
  while (result.endsWith('\n') || result.endsWith('\r'))
    result.chop(1);
  return result;
}

} // namespace

DiffModel::DiffModel(RepoView *view, QObject *parent)
    : QAbstractListModel(parent), mView(view) {
  git::RepositoryNotifier *notifier = view->repo().notifier();
  connect(notifier, &git::RepositoryNotifier::indexChanged, this,
          &DiffModel::updateIndex);

  // Draw the lines with the new tab width or whitespace setting.
  connect(Settings::instance(), &Settings::settingsChanged, this, [this] {
    if (!mRows.isEmpty())
      emit dataChanged(index(0), index(mRows.size() - 1),
                       {HtmlRole, MatchesRole});
  });
}

void DiffModel::setDiff(const git::Diff &diff, const QString &path) {
  // Keep the forced load of a large diff for the same file.
  if (path != mPath)
    mLoadAnyway = false;

  mDiff = diff;
  mPath = diff.isValid() && diff.indexOf(path) >= 0 ? path : QString();
  load();
}

QString DiffModel::oldPath() const {
  return mPatch.isValid() ? mPatch.name(git::Diff::OldFile) : QString();
}

QString DiffModel::status() const {
  if (!mPatch.isValid())
    return QString();

  if (mPatch.isConflicted())
    return "!";

  return QString(git::Diff::statusChar(mUntracked ? GIT_DELTA_UNTRACKED
                                                  : mPatch.status()));
}

bool DiffModel::isEditable() const {
  return mDiff.isValid() && mDiff.isStatusDiff() && mPatch.isValid();
}

bool DiffModel::isConflicted() const {
  return mPatch.isValid() && mPatch.isConflicted();
}

int DiffModel::stageState() const {
  if (!isEditable())
    return git::Index::Disabled;

  return mDiff.index().isStaged(mPath);
}

int DiffModel::row(int hunk, int line) const {
  for (int i = 0; i < mRows.size(); ++i) {
    if (mRows.at(i).hunk == hunk && mRows.at(i).line == line)
      return i;
  }

  return -1;
}

void DiffModel::loadAnyway() {
  mLoadAnyway = true;
  load();
}

void DiffModel::load() {
  beginResetModel();

  mPatch = git::Patch();
  mStaged = git::Patch();
  mContent.clear();
  mHunks.clear();
  mRows.clear();
  mStyles.clear();
  mDiagnostics.clear();
  mNotice.clear();
  clearImages();
  mCanLoadAnyway = false;
  mUntracked = false;
  mAdditions = 0;
  mDeletions = 0;
  mLineNumberWidth = 2;
  mMaxLineLength = 0;

  if (!mPath.isEmpty()) {
    mPatch = mDiff.patch(mDiff.indexOf(mPath));
    mUntracked = mPatch.isUntracked();

    // The diff has no content for untracked files. Show them in full, but
    // gate large ones on their size.
    bool binary = mPatch.isBinary();
    qint64 size = -1;
    if (mUntracked) {
      QFile file(mView->repo().workdir().filePath(mPath));
      if (QFileInfo(file).isFile() && file.open(QFile::ReadOnly)) {
        size = file.size();
        bool load = mLoadAnyway || size <= qint64(kMaxAutoLoadDiffSize);
        mContent = load ? file.readAll() : file.read(kMaxReadBinary);
        binary = git::Blob::isBinary(mContent.left(kMaxReadBinary));
        // The patch refers to the content, keep it until the patch is reset.
        if (load && !binary)
          mPatch = git::Patch::fromBuffers(QByteArray(), mContent, mPath, mPath);
        else if (!binary)
          mCanLoadAnyway = true;
      }
    }

    git::Patch::LineStats stats = mPatch.lineStats();
    mAdditions = stats.additions;
    mDeletions = stats.deletions;

    if (binary) {
      mNotice = tr("Binary file");
      loadImages(false);
    } else if (mPatch.isLfsPointer()) {
      mNotice = tr("Git LFS object");
      loadImages(true);
    } else if (mCanLoadAnyway) {
      mNotice = tr("This file is %1 and wasn't loaded.")
                    .arg(QLocale().formattedDataSize(size));
    } else if (!mLoadAnyway && stats.additions + stats.deletions > kMaxLines) {
      mNotice = tr("This diff has %1 changed lines and wasn't loaded.")
                    .arg(stats.additions + stats.deletions);
      mCanLoadAnyway = true;
    } else {
      loadStaged();

      int maxLine = 0;
      int tabWidth = SyntaxHighlighter::tabWidth();
      for (int h = 0; h < mPatch.count(); ++h) {
        QList<DiffLines::Line> lines = DiffLines::lines(mPatch, h, mStaged);
        mRows.append({HunkRow, h, -1});
        for (int l = 0; l < lines.size(); ++l) {
          mRows.append({LineRow, h, l});
          maxLine = qMax(maxLine, qMax(lines.at(l).oldLine, lines.at(l).newLine));

          // Approximate the width, tabs are expanded when drawn.
          const QByteArray &content = lines.at(l).content;
          int length = content.size() + content.count('\t') * (tabWidth - 1);
          mMaxLineLength = qMax(mMaxLineLength, length);
        }

        mHunks.append(lines);
      }

      mLineNumberWidth = qMax(2, static_cast<int>(QString::number(maxLine).size()));
      highlight();
      lint();

      if (mHunks.isEmpty())
        mNotice = mUntracked && QFileInfo(
                      mView->repo().workdir().filePath(mPath)).isDir()
                      ? tr("Untracked directory")
                  : mUntracked ? tr("Empty file")
                               : tr("No changes to show");
    }
  }

  endResetModel();
  emit diffChanged();
  emit stageStateChanged();
}

void DiffModel::loadStaged() {
  mStaged = git::Patch();
  if (!mDiff.isStatusDiff())
    return;

  // Generate a diff between the head tree and the index.
  git::Repository repo = mView->repo();
  git::Reference head = repo.head();
  git::Commit commit = head.isValid() ? head.target() : git::Commit();
  if (!commit.isValid())
    return;

  git::Diff staged = repo.diffTreeToIndex(commit.tree());
  int index = staged.indexOf(mPath);
  if (index >= 0)
    mStaged = staged.patch(index);
}

void DiffModel::updateIndex(const QStringList &paths) {
  if (mPath.isEmpty() || !mDiff.isStatusDiff() ||
      (!paths.isEmpty() && !paths.contains(mPath)))
    return;

  // Only the staged state of the lines changed.
  loadStaged();
  for (int h = 0; h < mHunks.size(); ++h) {
    QList<DiffLines::Line> lines = DiffLines::lines(mPatch, h, mStaged);
    for (int l = 0; l < lines.size() && l < mHunks[h].size(); ++l)
      mHunks[h][l].staged = lines.at(l).staged;
  }

  if (!mRows.isEmpty())
    emit dataChanged(index(0), index(mRows.size() - 1),
                     {StagedRole, HunkStateRole});
  emit stageStateChanged();
}

int DiffModel::hunkState(int hunk) const {
  int staged = 0;
  int changes = 0;
  for (const DiffLines::Line &line : mHunks.at(hunk)) {
    if (!line.isChange())
      continue;

    ++changes;
    if (line.staged)
      ++staged;
  }

  if (!staged)
    return git::Index::Unstaged;
  return staged == changes ? git::Index::Staged : git::Index::PartiallyStaged;
}

DiffModel::~DiffModel() { clearImages(); }

void DiffModel::loadImages(bool lfs) {
  git::Repository repo = mView->repo();
  auto load = [this, &repo, lfs](git::Diff::File file, QString &url,
                                 QString &info) {
    QByteArray data;
    git::Blob blob = mPatch.blob(file);
    if (blob.isValid()) {
      data = blob.content();
      if (lfs)
        data = repo.lfsSmudge(data, mPath);
    } else if (file == git::Diff::NewFile) {
      // The working copy.
      QFile workdirFile(repo.workdir().filePath(mPath));
      if (workdirFile.open(QFile::ReadOnly))
        data = workdirFile.readAll();
    }

    QImage image = QImage::fromData(data);
    if (image.isNull())
      return;

    url = QmlSupport::addImage(image);
    info = QString("%1 × %2 · %3")
               .arg(image.width())
               .arg(image.height())
               .arg(QLocale().formattedDataSize(data.size()));
  };

  load(git::Diff::OldFile, mOldImage, mOldImageInfo);
  if (mPatch.status() != GIT_DELTA_DELETED)
    load(git::Diff::NewFile, mNewImage, mNewImageInfo);
}

void DiffModel::clearImages() {
  if (!mOldImage.isEmpty())
    QmlSupport::removeImage(mOldImage);
  if (!mNewImage.isEmpty())
    QmlSupport::removeImage(mNewImage);
  mOldImage.clear();
  mNewImage.clear();
  mOldImageInfo.clear();
  mNewImageInfo.clear();
}

void DiffModel::highlight() {
  if (mHunks.isEmpty())
    return;

  // Style all lines of the file at once, in the order they appear.
  QByteArray text;
  QList<QList<QPair<int, int>>> spans;
  for (const QList<DiffLines::Line> &lines : mHunks) {
    QList<QPair<int, int>> lineSpans;
    for (const DiffLines::Line &line : lines) {
      QByteArray content = chomp(line.content);
      lineSpans.append({static_cast<int>(text.size()),
                        static_cast<int>(content.size())});
      text += content;
      text += '\n';
    }
    spans.append(lineSpans);
  }

  if (!mHighlighter)
    mHighlighter.reset(new SyntaxHighlighter);
  QByteArray styles = mHighlighter->style(mPath, text);

  for (const QList<QPair<int, int>> &lineSpans : spans) {
    QList<QByteArray> hunkStyles;
    for (const auto &span : lineSpans)
      hunkStyles.append(styles.mid(span.first, span.second));
    mStyles.append(hunkStyles);
  }
}

void DiffModel::lint() {
  if (mHunks.isEmpty() || !mHighlighter)
    return;

  if (!mPluginsLoaded) {
    mPluginsLoaded = true;
    mPlugins = Plugin::plugins(mView->repo());
  }

  QList<PluginRef> plugins;
  for (const PluginRef &plugin : mPlugins) {
    if (plugin->isValid() && plugin->isEnabled())
      plugins.append(plugin);
  }

  if (plugins.isEmpty())
    return;

  // Run the plugins on each hunk like the text editor of the widget diff.
  TextEditor *editor = mHighlighter->editor();
  for (const QList<DiffLines::Line> &lines : mHunks) {
    QByteArray text;
    for (const DiffLines::Line &line : lines)
      text += chomp(line.content) + '\n';

    editor->setLexer(mPath);
    editor->setText(text.constData());
    editor->clearDiagnostics();
    for (int i = 0; i < lines.size(); ++i) {
      char origin = lines.at(i).origin;
      editor->markerAdd(i, origin == '+'   ? TextEditor::Addition
                           : origin == '-' ? TextEditor::Deletion
                                           : TextEditor::Context);
    }
    editor->colourise(0, -1);

    for (const PluginRef &plugin : plugins)
      plugin->hunk(editor);

    QList<QVariantList> hunkDiagnostics;
    for (int i = 0; i < lines.size(); ++i) {
      QVariantList diagnostics;
      for (const TextEditor::Diagnostic &diag : editor->diagnostics(i)) {
        diagnostics.append(QVariantMap{{"kind", diag.kind},
                                       {"message", diag.message},
                                       {"description", diag.description}});
      }
      hunkDiagnostics.append(diagnostics);
    }
    mDiagnostics.append(hunkDiagnostics);
  }

  editor->setText("");
  editor->clearDiagnostics();
}

QString DiffModel::html(int hunk, int line) const {
  const QList<DiffLines::Line> &lines = mHunks.at(hunk);
  const DiffLines::Line &current = lines.at(line);
  git::Repository repo = mView->repo();
  QByteArray content = chomp(current.content);

  // Highlight the changed words of modified lines.
  DiffLines::Ranges ranges;
  if (current.isChange() && current.matchingLine >= 0) {
    const DiffLines::Line &match = lines.at(current.matchingLine);
    bool deletion = (current.origin == '-');
    auto changes = deletion ? DiffLines::changedRanges(current.content,
                                                       match.content)
                            : DiffLines::changedRanges(match.content,
                                                       current.content);
    ranges = deletion ? changes.first : changes.second;
  }

  Theme *theme = Application::theme();
  QColor wordColor = theme->diff(current.origin == '-'
                                     ? Theme::Diff::WordDeletion
                                     : Theme::Diff::WordAddition);

  // Whether each byte is in a changed word.
  QVector<bool> changed(content.size(), false);
  for (const auto &range : ranges) {
    int end = qMin(range.first + range.second, static_cast<int>(content.size()));
    for (int i = qMax(0, range.first); i < end; ++i)
      changed[i] = true;
  }

  QByteArray styles;
  if (hunk < mStyles.size() && line < mStyles.at(hunk).size())
    styles = mStyles.at(hunk).at(line);

  if (!mHighlighter)
    return QString();

  return mHighlighter->html(
      content, styles,
      [&repo](const QByteArray &bytes) { return repo.decode(bytes); }, changed,
      wordColor);
}

void DiffModel::toggleLine(int row) {
  if (row < 0 || row >= mRows.size() || mRows.at(row).kind != LineRow)
    return;

  const Row &r = mRows.at(row);
  DiffLines::Line &line = mHunks[r.hunk][r.line];
  if (!isEditable() || isConflicted() || !line.isChange())
    return;

  line.staged = !line.staged;
  stage(r.hunk);
}

void DiffModel::setLinesStaged(int first, int last, bool staged) {
  if (!isEditable() || isConflicted())
    return;

  QSet<int> hunks;
  for (int row = qMax(0, first); row <= last && row < mRows.size(); ++row) {
    const Row &r = mRows.at(row);
    if (r.kind != LineRow)
      continue;

    DiffLines::Line &line = mHunks[r.hunk][r.line];
    if (line.isChange()) {
      line.staged = staged;
      hunks.insert(r.hunk);
    }
  }

  if (!hunks.isEmpty())
    stage(*hunks.begin());
}

void DiffModel::setHunkStaged(int hunk, bool staged) {
  if (!isEditable() || isConflicted() || hunk < 0 || hunk >= mHunks.size())
    return;

  for (DiffLines::Line &line : mHunks[hunk]) {
    if (line.isChange())
      line.staged = staged;
  }

  stage(hunk);
}

void DiffModel::setFileStaged(bool staged) {
  if (isEditable())
    mDiff.index().setStaged({mPath}, staged);
}

void DiffModel::stage(int changedHunk) {
  Q_UNUSED(changedHunk)

  git::Index index = mDiff.index();
  if (!index.isValid())
    return;

  int staged = 0;
  int unstaged = 0;
  for (int h = 0; h < mHunks.size(); ++h) {
    int state = hunkState(h);
    if (state == git::Index::Staged)
      ++staged;
    else if (state == git::Index::Unstaged)
      ++unstaged;
  }

  if (staged == mHunks.size() && !mHunks.isEmpty()) {
    index.setStaged({mPath}, true);
    return;
  }

  if (unstaged == mHunks.size()) {
    index.setStaged({mPath}, false);
    return;
  }

  // Build the content of the index from the staged lines. When a line is
  // changed, the old and the new version are in the diff. Staging only the
  // new one keeps the old one in the file.
  git::Repository repo = mView->repo();
  git::Blob blob = repo.lookupBlob(repo.workdirId(mPath));

  QList<QList<QByteArray>> image;
  git::Patch::populatePreimage(image, blob.content());
  for (int h = 0; h < mHunks.size(); ++h) {
    QByteArray content = DiffLines::stagedContent(mHunks.at(h));
    mPatch.apply(image, h, content);
  }

  index.add(mPath, mPatch.generateResult(image));
}

void DiffModel::discardHunk(int hunk) {
  if (!isEditable() || hunk < 0 || hunk >= mHunks.size())
    return;

  QString text =
      mUntracked
          ? tr("Are you sure you want to remove '%1'?").arg(mPath)
          : tr("Are you sure you want to discard this hunk of '%1'?").arg(mPath);
  ConfirmDialog *dialog = new ConfirmDialog(mView);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setTitle(tr("Discard Hunk?"));
  dialog->setText(text);
  dialog->setInformativeText(tr("This action cannot be undone."));
  dialog->setAcceptText(tr("Discard Hunk"));
  dialog->setDanger(true);

  QList<bool> lines(mHunks.at(hunk).size(), true);
  connect(dialog, &QDialog::accepted, this,
          [this, hunk, lines] { this->discard(hunk, lines); });

  dialog->open();
}

void DiffModel::discardLines(int first, int last) {
  if (!isEditable() || first < 0 || first >= mRows.size())
    return;

  int hunk = mRows.at(first).hunk;
  QList<bool> lines(mHunks.at(hunk).size(), false);
  for (int row = first; row <= last && row < mRows.size(); ++row) {
    if (mRows.at(row).hunk == hunk && mRows.at(row).kind == LineRow)
      lines[mRows.at(row).line] = true;
  }

  ConfirmDialog *dialog = new ConfirmDialog(mView);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setTitle(tr("Discard Lines?"));
  dialog->setText(
      tr("Are you sure you want to discard the selected lines of '%1'?")
          .arg(mPath));
  dialog->setInformativeText(tr("This action cannot be undone."));
  dialog->setAcceptText(tr("Discard Lines"));
  dialog->setDanger(true);

  connect(dialog, &QDialog::accepted, this,
          [this, hunk, lines] { this->discard(hunk, lines); });

  dialog->open();
}

void DiffModel::discard(int hunk, const QList<bool> &lines) {
  git::Repository repo = mView->repo();
  if (mUntracked) {
    repo.workdir().remove(mPath);
    mView->refresh();
    return;
  }

  git::Blob blob = repo.lookupBlob(repo.workdirId(mPath));
  QByteArray content = DiffLines::discardContent(mHunks.at(hunk), lines);
  QByteArray buffer = mPatch.apply(hunk, content, blob.content());

  QSaveFile file(repo.workdir().filePath(mPath));
  if (!file.open(QFile::WriteOnly))
    return;

  file.write(buffer);
  if (!file.commit())
    return;

  mView->refresh();
}

void DiffModel::editHunk(int hunk) {
  if (hunk < 0 || hunk >= mHunks.size())
    return;

  int line = 1;
  for (const DiffLines::Line &l : mHunks.at(hunk)) {
    if (l.newLine > 0) {
      line = l.newLine;
      break;
    }
  }

  mView->edit(mPath, line);
}

void DiffModel::showEditMenu(int hunk, qreal x, qreal y) {
  if (mPath.isEmpty() || !mPatch.isValid())
    return;

  // Calculate starting line numbers.
  int oldLine = -1;
  int newLine = -1;
  if (hunk >= 0 && hunk < mPatch.count() && mPatch.lineCount(hunk) > 0) {
    oldLine = mPatch.lineNumber(hunk, 0, git::Diff::OldFile);
    newLine = mPatch.lineNumber(hunk, 0, git::Diff::NewFile);
  }

  QMenu menu;
  RepoView *view = mView;
  QString name = mPath;

  if (view->repo().workdir().exists(name)) {
    menu.addAction(tr("Edit Working Copy"), this,
                   [view, name, newLine] { view->edit(name, newLine); });
  }

  QList<git::Commit> commits = view->commits();
  git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();

  git::Blob newBlob = mPatch.blob(git::Diff::NewFile);
  if (newBlob.isValid()) {
    menu.addAction(tr("Edit New Revision"), this,
                   [view, name, newLine, newBlob, commit] {
                     view->openEditor(name, newLine, newBlob, commit);
                   });
  }

  git::Blob oldBlob = mPatch.blob(git::Diff::OldFile);
  if (oldBlob.isValid()) {
    git::Commit parent = commit;
    if (parent.isValid() && !parent.parents().isEmpty())
      parent = parent.parents().first();
    menu.addAction(tr("Edit Old Revision"), this,
                   [view, name, oldLine, oldBlob, parent] {
                     view->openEditor(name, oldLine, oldBlob, parent);
                   });
  }

  if (!menu.isEmpty())
    QmlSupport::execMenu(&menu, view->mapFromPage(x, y));
}

void DiffModel::chooseConflict(int hunk, int resolution) {
  if (!isConflicted() || hunk < 0 || hunk >= mHunks.size())
    return;

  mPatch.setConflictResolution(
      hunk, static_cast<git::Patch::ConflictResolution>(resolution));

  for (int row = 0; row < mRows.size(); ++row) {
    if (mRows.at(row).hunk == hunk)
      emit dataChanged(index(row), index(row), {ResolutionRole, ChosenRole});
  }
}

void DiffModel::saveConflict(int hunk) {
  if (!isConflicted() || hunk < 0 || hunk >= mHunks.size())
    return;

  git::Patch::ConflictResolution resolution = mPatch.conflictResolution(hunk);
  if (resolution == git::Patch::Unresolved)
    return;

  git::Repository repo = mView->repo();
  QString path = repo.workdir().filePath(mPath);

  QStringList lines;
  QStringConverter::Encoding encoding = QStringConverter::Utf8;
  {
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
      return;

    // Keep the line endings and the encoding.
    QByteArray bytes = file.readAll();
    encoding = repo.encoding(bytes);
    QString text = QStringDecoder(encoding).decode(bytes);
    int pos = 0;
    while (pos < text.length()) {
      int end = text.indexOf('\n', pos);
      end = (end < 0) ? text.length() : end + 1;
      lines.append(text.mid(pos, end - pos));
      pos = end;
    }
  }

  // Remove the lines of the other side and the conflict markers.
  for (int i = mPatch.lineCount(hunk) - 1; i >= 0; --i) {
    char origin = mPatch.lineOrigin(hunk, i);
    if (origin == GIT_DIFF_LINE_CONTEXT ||
        (origin == 'O' && resolution == git::Patch::Ours) ||
        (origin == 'T' && resolution == git::Patch::Theirs))
      continue;

    int line = mPatch.lineNumber(hunk, i);
    if (line >= 0 && line < lines.size())
      lines.removeAt(line);
  }

  QSaveFile file(path);
  if (!file.open(QFile::WriteOnly))
    return;

  QTextStream out(&file);
  out.setEncoding(encoding);
  out << lines.join(QString());
  out.flush();
  if (!file.commit())
    return;

  mPatch.setConflictResolution(hunk, git::Patch::Unresolved);
  mView->refresh();
}

int DiffModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : mRows.size();
}

QVariant DiffModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= mRows.size())
    return QVariant();

  const Row &row = mRows.at(index.row());
  switch (role) {
    case KindRole:
      return row.kind;
    case HunkRole:
      return row.hunk;
    case HeaderRole:
      return row.kind == HunkRow ? QString::fromUtf8(chomp(mPatch.header(row.hunk)))
                                 : QString();
    case HunkStateRole:
      return hunkState(row.hunk);
    case ResolutionRole:
      return isConflicted()
                 ? static_cast<int>(
                       const_cast<git::Patch &>(mPatch).conflictResolution(
                           row.hunk))
                 : 0;
  }

  if (row.kind != LineRow) {
    switch (role) {
      case OriginRole:
        return QString();
      case OldLineRole:
      case NewLineRole:
        return -1;
      case HtmlRole:
        return QString();
      case StagedRole:
      case StageableRole:
        return false;
      case ChosenRole:
        return true;
      case DiagnosticsRole:
      case MatchesRole:
        return QVariantList();
    }

    return QVariant();
  }

  const DiffLines::Line &line = mHunks.at(row.hunk).at(row.line);
  switch (role) {
    case OriginRole:
      return QString(QChar(line.origin));
    case OldLineRole:
      return line.oldLine;
    case NewLineRole:
      return line.newLine;
    case HtmlRole:
      return html(row.hunk, row.line);
    case StagedRole:
      return line.staged;
    case StageableRole:
      return isEditable() && !isConflicted() && line.isChange();
    case ChosenRole: {
      if (!isConflicted())
        return true;

      auto resolution =
          const_cast<git::Patch &>(mPatch).conflictResolution(row.hunk);
      if (resolution == git::Patch::Ours)
        return line.origin != 'T';
      if (resolution == git::Patch::Theirs)
        return line.origin != 'O';
      return true;
    }
    case DiagnosticsRole:
      if (row.hunk < mDiagnostics.size() &&
          row.line < mDiagnostics.at(row.hunk).size())
        return mDiagnostics.at(row.hunk).at(row.line);
      return QVariantList();
    case MatchesRole:
      return findMatches(mFindText, index.row(), mFindRow, mFindStart);
  }

  return QVariant();
}

QHash<int, QByteArray> DiffModel::roleNames() const {
  return {{KindRole, "kind"},         {HunkRole, "hunk"},
          {OriginRole, "origin"},     {OldLineRole, "oldLine"},
          {NewLineRole, "newLine"},   {HtmlRole, "html"},
          {StagedRole, "staged"},     {StageableRole, "stageable"},
          {HeaderRole, "header"},     {HunkStateRole, "hunkState"},
          {ResolutionRole, "resolution"}, {ChosenRole, "chosen"},
          {DiagnosticsRole, "diagnostics"}, {MatchesRole, "matches"}};
}

QString DiffModel::findRowText(int row) const {
  if (row < 0 || row >= mRows.size() || mRows.at(row).kind != LineRow)
    return QString();

  const Row &current = mRows.at(row);
  const DiffLines::Line &line = mHunks.at(current.hunk).at(current.line);
  return SyntaxHighlighter::expandTabs(
      mView->repo().decode(chomp(line.content)));
}

void DiffModel::setFindState(const QString &text, int row, int start) {
  if (text == mFindText && row == mFindRow && start == mFindStart)
    return;

  // Only the rows of the current match change when moving between matches.
  int previous = mFindRow;
  bool all = (text != mFindText);
  mFindText = text;
  mFindRow = row;
  mFindStart = start;
  if (mRows.isEmpty())
    return;

  if (all) {
    emit dataChanged(index(0), index(mRows.size() - 1), {MatchesRole});
    return;
  }

  for (int changed : {previous, row}) {
    if (changed >= 0 && changed < mRows.size())
      emit dataChanged(index(changed), index(changed), {MatchesRole});
  }
}
