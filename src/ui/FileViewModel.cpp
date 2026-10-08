//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "FileViewModel.h"
#include "SyntaxHighlighter.h"
#include "app/Application.h"
#include "app/Theme.h"
#include "conf/Constants.h"
#include "conf/Settings.h"
#include "git/Blob.h"
#include "git/Repository.h"
#include "git/Signature.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QUrl>
#include <QUrlQuery>
#include <QtConcurrent>

namespace {

// Larger files aren't shown.
const qint64 kMaxSize = 4 * 1024 * 1024;

QString relativeDate(const QDateTime &dateTime) {
  qint64 secs = dateTime.secsTo(QDateTime::currentDateTime());
  auto format = [](qint64 n, const QString &one, const QString &many) {
    return (n == 1) ? one : many.arg(n);
  };

  if (secs < 60)
    return FileViewModel::tr("just now");
  if (secs < 3600)
    return format(secs / 60, FileViewModel::tr("1 minute ago"),
                  FileViewModel::tr("%1 minutes ago"));
  if (secs < 86400)
    return format(secs / 3600, FileViewModel::tr("1 hour ago"),
                  FileViewModel::tr("%1 hours ago"));
  if (secs < 86400 * 30)
    return format(secs / 86400, FileViewModel::tr("1 day ago"),
                  FileViewModel::tr("%1 days ago"));
  if (secs < 86400 * 365)
    return format(secs / (86400 * 30), FileViewModel::tr("1 month ago"),
                  FileViewModel::tr("%1 months ago"));
  return format(secs / (86400 * 365), FileViewModel::tr("1 year ago"),
                FileViewModel::tr("%1 years ago"));
}

} // namespace

// Stops the blame of a file that is no longer shown.
class BlameCanceler : public git::Blame::Callbacks {
public:
  void setCanceled(bool canceled) { mCanceled = canceled; }
  bool progress() override { return !mCanceled; }

private:
  std::atomic<bool> mCanceled{false};
};

FileViewModel::FileViewModel(const git::Repository &repo, QObject *parent)
    : QAbstractListModel(parent), mRepo(repo), mCanceler(new BlameCanceler) {
  connect(&mBlameWatcher, &QFutureWatcher<git::Blame>::finished, this, [this] {
    QFuture<git::Blame> future = mBlameWatcher.future();
    git::Blame blame = future.isValid() && future.resultCount() > 0
                           ? future.result()
                           : git::Blame();

    // Mark the lines that were changed in the working copy.
    mSourceBlame = blame;
    if (blame.isValid() && !mCommit.isValid())
      blame = blame.updated(mContent);

    setBlame(blame);
  });

  // Draw the lines with the new tab width or whitespace setting.
  connect(Settings::instance(), &Settings::settingsChanged, this, [this] {
    if (!mLines.isEmpty())
      emit dataChanged(index(0), index(mLines.size() - 1),
                       {HtmlRole, MatchesRole});
  });
}

FileViewModel::~FileViewModel() { cancelBlame(); }

void FileViewModel::load(const QString &path, const git::Commit &commit) {
  QByteArray content;
  bool readable = false;
  qint64 size = 0;
  if (commit.isValid()) {
    git::Blob blob = commit.blob(path);
    if (blob.isValid()) {
      content = blob.content();
      size = content.size();
      readable = true;
    }
  } else {
    QFile file(mRepo.workdir().filePath(path));
    if (file.open(QFile::ReadOnly)) {
      size = file.size();
      content = size > kMaxSize ? file.read(kMaxReadBinary) : file.readAll();
      readable = true;
    }
  }

  // Keep the file and its blame if nothing changed, for example when the
  // status of the working copy is refreshed.
  bool sameCommit = commit.isValid() ? mCommit.isValid() && commit == mCommit
                                     : !mCommit.isValid();
  if (!mEditing && path == mPath && sameCommit && readable &&
      content == mContent && (!mLines.isEmpty() || !mNotice.isEmpty()))
    return;

  cancelBlame();
  beginResetModel();
  reset(path, commit, content);
  mEditing = false;

  if (!readable) {
    mNotice = tr("This file doesn't exist in this version.");
  } else if (git::Blob::isBinary(content.left(kMaxReadBinary))) {
    mNotice = tr("Binary file");
  } else if (size > kMaxSize) {
    mNotice = tr("This file is %1 and wasn't loaded.")
                  .arg(QLocale().formattedDataSize(size));
  } else if (content.isEmpty()) {
    mNotice = tr("Empty file");
  } else {
    setLines(content, true);
  }

  endResetModel();
  emit fileChanged();
  emit selectedCommitChanged();
  startBlame();
}

void FileViewModel::setEditorText(const QString &path,
                                  const git::Commit &commit,
                                  const QByteArray &content) {
  cancelBlame();
  beginResetModel();
  reset(path, commit, content);
  mEditing = true;
  setLines(content, false);
  endResetModel();

  emit fileChanged();
  emit selectedCommitChanged();
  startBlame();
}

void FileViewModel::updateEditorText(const QByteArray &content) {
  if (!mEditing || content == mContent)
    return;

  // Move the blame of the source to the lines of the edited text.
  beginResetModel();
  mContent = content;
  mLines.clear();
  setLines(content, false);
  git::Blame blame = mSourceBlame.isValid() && !mBlameLoading
                         ? mSourceBlame.updated(content)
                         : git::Blame();
  updateBlocks(blame);
  endResetModel();
  emit blameChanged();
}

void FileViewModel::reset(const QString &path, const git::Commit &commit,
                          const QByteArray &content) {
  mPath = path;
  mCommit = commit;
  mContent = content;
  mNotice.clear();
  mMaxLineLength = 0;
  mLines.clear();
  mStyles.clear();
  mBlame = git::Blame();
  mSourceBlame = git::Blame();
  mBlocks.clear();
  mLineBlocks.clear();
  mLineOffsets.clear();
  mSelectedCommit.clear();
}

void FileViewModel::setLines(const QByteArray &content, bool highlight) {
  // Split into lines without their line endings.
  int tabWidth = SyntaxHighlighter::tabWidth();
  mMaxLineLength = 0;
  for (QByteArray line : content.split('\n')) {
    if (line.endsWith('\r'))
      line.chop(1);
    mLines.append(line);

    int length = line.size() + line.count('\t') * (tabWidth - 1);
    mMaxLineLength = qMax(mMaxLineLength, length);
  }

  // The last line ending doesn't start a new line.
  if (content.endsWith('\n') || content.isEmpty())
    mLines.removeLast();

  if (!highlight)
    return;

  QByteArray text;
  QList<QPair<int, int>> spans;
  for (const QByteArray &line : mLines) {
    spans.append({static_cast<int>(text.size()),
                  static_cast<int>(line.size())});
    text += line;
    text += '\n';
  }

  if (!mHighlighter)
    mHighlighter.reset(new SyntaxHighlighter);
  QByteArray styles = mHighlighter->style(mPath, text);
  for (const auto &span : spans)
    mStyles.append(styles.mid(span.first, span.second));
}

void FileViewModel::startBlame() {
  // Find the commits of the lines in the background.
  if (!mLines.isEmpty() && mRepo.isValid() && !mPath.isEmpty()) {
    mBlameLoading = true;
    mBlameWatcher.setFuture(QtConcurrent::run(&git::Repository::blame, mRepo,
                                              mPath, mCommit,
                                              mCanceler.data()));
  }

  emit blameChanged();
}

void FileViewModel::clear() {
  if (mPath.isEmpty() && mLines.isEmpty())
    return;

  cancelBlame();
  beginResetModel();
  reset(QString(), git::Commit(), QByteArray());
  mEditing = false;
  endResetModel();

  emit fileChanged();
  emit blameChanged();
  emit selectedCommitChanged();
}

QString FileViewModel::revision() const {
  if (mPath.isEmpty())
    return QString();

  return mCommit.isValid() ? mCommit.shortId() : tr("Working Copy");
}

int FileViewModel::lineNumberWidth() const {
  return qMax(2, static_cast<int>(QString::number(mLines.size()).size()));
}

void FileViewModel::setSelectedCommit(const QString &id) {
  if (id == mSelectedCommit)
    return;

  mSelectedCommit = id;
  emit selectedCommitChanged();
}

void FileViewModel::showCommit(const QString &id) {
  if (id.isEmpty())
    return;

  QUrlQuery query;
  query.addQueryItem("file", mPath);

  QUrl url;
  url.setScheme("id");
  url.setPath(id);
  url.setQuery(query);
  emit linkActivated(url.toString());
}

QVariantMap FileViewModel::line(int row) const {
  // Rows after the last line, like the empty line after the last line
  // ending, are numbered but have no commit.
  QModelIndex index = this->index(row);
  if (!index.isValid())
    return QVariantMap{{"number", row + 1},
                       {"blameId", QString()},
                       {"blameFirst", false},
                       {"blameLast", false},
                       {"blameOffset", -1},
                       {"blameCommitted", false},
                       {"blameSummary", QString()},
                       {"blameAuthor", QString()},
                       {"blameDate", QString()},
                       {"blameColor", QString()},
                       {"blameTip", QString()}};

  QVariantMap line;
  QHash<int, QByteArray> roles = roleNames();
  for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
    if (it.key() != HtmlRole && it.key() != MatchesRole)
      line.insert(QString::fromUtf8(it.value()), data(index, it.key()));
  }

  return line;
}

int FileViewModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : mLines.size();
}

QVariant FileViewModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= mLines.size())
    return QVariant();

  int row = index.row();
  if (role == NumberRole)
    return row + 1;

  if (role == HtmlRole) {
    if (!mHighlighter)
      return QString();

    return mHighlighter->html(
        mLines.at(row), mStyles.value(row),
        [this](const QByteArray &bytes) { return decode(bytes); });
  }

  if (role == MatchesRole)
    return findMatches(mFindText, row, mFindRow, mFindStart);

  int block = mLineBlocks.value(row, -1);
  if (block < 0) {
    switch (role) {
      case BlameFirstRole:
      case BlameLastRole:
      case BlameCommittedRole:
        return false;
      case BlameOffsetRole:
        return -1;
      default:
        return QString();
    }
  }

  const Block &current = mBlocks.at(block);
  switch (role) {
    case BlameIdRole:
      return current.id;
    case BlameFirstRole:
      return row == 0 || mLineBlocks.at(row - 1) != block;
    case BlameLastRole:
      return row + 1 >= mLineBlocks.size() || mLineBlocks.at(row + 1) != block;
    case BlameOffsetRole:
      return mLineOffsets.value(row);
    case BlameCommittedRole:
      return current.committed;
    case BlameSummaryRole:
      return current.summary;
    case BlameAuthorRole:
      return current.author;
    case BlameDateRole:
      return current.date.isValid() ? relativeDate(current.date) : QString();
    case BlameColorRole:
      return current.color;
    case BlameTipRole:
      return current.tip;
  }

  return QVariant();
}

QHash<int, QByteArray> FileViewModel::roleNames() const {
  return {{NumberRole, "number"},
          {HtmlRole, "html"},
          {BlameIdRole, "blameId"},
          {BlameFirstRole, "blameFirst"},
          {BlameLastRole, "blameLast"},
          {BlameOffsetRole, "blameOffset"},
          {BlameCommittedRole, "blameCommitted"},
          {BlameSummaryRole, "blameSummary"},
          {BlameAuthorRole, "blameAuthor"},
          {BlameDateRole, "blameDate"},
          {BlameColorRole, "blameColor"},
          {BlameTipRole, "blameTip"},
          {MatchesRole, "matches"}};
}

QString FileViewModel::findRowText(int row) const {
  if (row < 0 || row >= mLines.size())
    return QString();

  return SyntaxHighlighter::expandTabs(decode(mLines.at(row)));
}

void FileViewModel::setFindState(const QString &text, int row, int start) {
  if (text == mFindText && row == mFindRow && start == mFindStart)
    return;

  // Only the rows of the current match change when moving between matches.
  int previous = mFindRow;
  bool all = (text != mFindText);
  mFindText = text;
  mFindRow = row;
  mFindStart = start;
  if (mLines.isEmpty())
    return;

  if (all) {
    emit dataChanged(index(0), index(mLines.size() - 1), {MatchesRole});
    return;
  }

  for (int changed : {previous, row}) {
    if (changed >= 0 && changed < mLines.size())
      emit dataChanged(index(changed), index(changed), {MatchesRole});
  }
}

void FileViewModel::cancelBlame() {
  mBlameLoading = false;
  if (!mBlameWatcher.isRunning())
    return;

  mCanceler->setCanceled(true);
  mBlameWatcher.waitForFinished();
  mBlameWatcher.setFuture(QFuture<git::Blame>());
  mCanceler->setCanceled(false);
}

void FileViewModel::setBlame(const git::Blame &blame) {
  mBlameLoading = false;
  updateBlocks(blame);

  if (!mLines.isEmpty())
    emit dataChanged(index(0), index(mLines.size() - 1));
  emit blameChanged();
}

QString FileViewModel::decode(const QByteArray &text) const {
  return mRepo.isValid() ? mRepo.decode(text) : QString::fromUtf8(text);
}

void FileViewModel::updateBlocks(const git::Blame &blame) {
  mBlame = blame;
  mBlocks.clear();
  mLineBlocks = QList<int>(mLines.size(), -1);
  mLineOffsets = QList<int>(mLines.size(), 0);

  if (blame.isValid()) {
    // Lines of the same commit that follow each other form a block.
    int count = blame.count();
    for (int line = 0; line < mLines.size(); ++line) {
      int index = blame.index(line + 1);
      if (index < 0 || index >= count)
        continue;

      bool committed = blame.isCommitted(index);
      QString id = committed ? blame.id(index).toString() : QString();
      int previous = line > 0 ? mLineBlocks.at(line - 1) : -1;
      if (previous >= 0 && mBlocks.at(previous).id == id) {
        mLineBlocks[line] = previous;
        mLineOffsets[line] = mLineOffsets.at(line - 1) + 1;
        continue;
      }

      Block block;
      block.id = id;
      block.committed = committed;
      if (!committed) {
        block.summary = tr("Not Committed");
      } else {
        block.summary = blame.message(index).section('\n', 0, 0);
        git::Signature signature = blame.signature(index);
        if (signature.isValid()) {
          block.author = signature.name();
          block.date = signature.date();
        }

        QStringList lines;
        if (!block.author.isEmpty())
          lines.append(QString("<b>%1</b> &lt;%2&gt;")
                           .arg(block.author.toHtmlEscaped(),
                                signature.email().toHtmlEscaped()));
        if (block.date.isValid())
          lines.append(QLocale().toString(block.date, QLocale::LongFormat));
        lines.append(id);
        block.tip = lines.join("<br>");

        QString message = blame.message(index).trimmed();
        if (!message.isEmpty())
          block.tip += QString("<p>%1</p>").arg(
              message.toHtmlEscaped().replace('\n', "<br>"));
      }

      mBlocks.append(block);
      mLineBlocks[line] = mBlocks.size() - 1;
    }

    // The age of each commit relative to the oldest and newest in the file.
    bool heatMap =
        Settings::instance()->value(Setting::Id::ShowHeatmapInBlameMargin)
            .toBool();
    qint64 min = std::numeric_limits<qint64>::max();
    qint64 max = std::numeric_limits<qint64>::min();
    for (const Block &block : mBlocks) {
      if (!block.date.isValid())
        continue;
      qint64 time = block.date.toSecsSinceEpoch();
      min = qMin(min, time);
      max = qMax(max, time);
    }

    // Blend from cold for the oldest to hot for the newest.
    QColor cold = Application::theme()->heatMap(Theme::HeatMap::Cold);
    QColor hot = Application::theme()->heatMap(Theme::HeatMap::Hot);
    for (Block &block : mBlocks) {
      if (!heatMap || !block.date.isValid())
        continue;

      qreal age = 1;
      if (max > min)
        age = qreal(block.date.toSecsSinceEpoch() - min) / (max - min);
      block.color = QColor::fromRgbF(
                        cold.redF() + (hot.redF() - cold.redF()) * age,
                        cold.greenF() + (hot.greenF() - cold.greenF()) * age,
                        cold.blueF() + (hot.blueF() - cold.blueF()) * age)
                        .name();
    }
  }
}
