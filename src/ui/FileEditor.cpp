//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "FileEditor.h"
#include "CodeHighlighter.h"
#include "FileViewModel.h"
#include "SyntaxHighlighter.h"
#include "conf/Constants.h"
#include "conf/Settings.h"
#include "git/Index.h"
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QQuickTextDocument>
#include <QSaveFile>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextStream>

namespace {

// Update the blame after the text stops changing.
const int kBlameDelay = 300;

} // namespace

FileEditor::FileEditor(const git::Repository &repo, QObject *parent)
    : QObject(parent), mRepo(repo), mBlame(new FileViewModel(repo, this)) {
  connect(mBlame, &FileViewModel::linkActivated, this,
          &FileEditor::linkActivated);

  // Number the lines of a new file.
  mBlame->setEditorText(QString(), git::Commit(), QByteArray());

  mFinder = new FindController([this]() -> FindTarget * { return this; }, this);

  mBlameTimer.setSingleShot(true);
  mBlameTimer.setInterval(kBlameDelay);
  connect(&mBlameTimer, &QTimer::timeout, this, [this] {
    mBlame->updateEditorText(text());
    mFinder->refresh();
  });

  connect(Settings::instance(), &Settings::settingsChanged, this, [this] {
    if (mDocument) {
      QTextOption option = mDocument->defaultTextOption();
      bool whitespace = Settings::instance()
                            ->value(Setting::Id::ShowWhitespaceInEditor)
                            .toBool();
      option.setFlags(whitespace ? QTextOption::ShowTabsAndSpaces
                                 : QTextOption::Flags());
      mDocument->setDefaultTextOption(option);
    }

    emit settingsChanged();
  });
}

FileEditor::~FileEditor() {}

bool FileEditor::load(const QString &name, const git::Blob &blob,
                      const git::Commit &commit) {
  mName = name;

  QByteArray content;
  if (blob.isValid()) {
    if (blob.isBinary())
      return false;

    content = blob.content();
    mRevision = commit.isValid() ? commit.shortId() : tr("HEAD");
    mCommit = commit;
    mReadOnly = true;

  } else {
    mRevision = mRepo.isValid() && mRepo.index().isTracked(name)
                    ? tr("Working Copy")
                    : QString();
    mCommit = git::Commit();
    mReadOnly = false;

    QFile file(path());
    if (!file.open(QFile::ReadOnly))
      return false;

    // Read the start of the file to check if it's binary.
    content = file.read(kMaxReadBinary);
    if (git::Blob::isBinary(content))
      return false;

    if (static_cast<size_t>(content.length()) >= kMaxReadBinary)
      content += file.readAll();
  }

  // The text area has lines without carriage returns.
  mCrlf = content.contains("\r\n");
  mEncoding =
      mRepo.isValid() ? mRepo.encoding(content) : QStringConverter::Utf8;
  mText = QStringDecoder(mEncoding).decode(content);
  if (mCrlf)
    mText.replace("\r\n", "\n");

  setText();

  // Find the commits of the lines of files in the repository.
  bool blame = mRepo.isValid() && !mRevision.isEmpty();
  mBlame->setEditorText(blame ? name : QString(), mCommit, content);

  emit fileChanged();
  return true;
}

QString FileEditor::path() const {
  if (mName.isEmpty() || QDir::isAbsolutePath(mName) || !mRepo.isValid())
    return mName;

  return mRepo.workdir().filePath(mName);
}

bool FileEditor::isModified() const {
  return mDocument && mDocument->isModified();
}

int FileEditor::tabWidth() const { return SyntaxHighlighter::tabWidth(); }

bool FileEditor::wrapLines() const {
  return Settings::instance()->isTextEditorWrapLines();
}

bool FileEditor::useTabs() const {
  return Settings::instance()->value(Setting::Id::UseTabsForIndent).toBool();
}

int FileEditor::indentWidth() const {
  int width = Settings::instance()->value(Setting::Id::IndentWidth).toInt();
  return width > 0 ? width : 4;
}

QObject *FileEditor::blame() const { return mBlame; }

QObject *FileEditor::finder() const { return mFinder; }

bool FileEditor::save() {
  if (!mDocument || mReadOnly)
    return false;

  // Ask for the name of a new file.
  QString path = this->path();
  if (path.isEmpty()) {
    QDir dir = mRepo.isValid() ? mRepo.workdir() : QDir();
    QWidget *window = qobject_cast<QWidget *>(parent());
    path = QFileDialog::getSaveFileName(window, tr("Save File"), dir.path());
    if (path.isEmpty())
      return false;

    mName = path;
    if (mHighlighter)
      mHighlighter->setPath(path);
  }

  QSaveFile file(path);
  if (!file.open(QFile::WriteOnly))
    return false;

  QString text = mDocument->toPlainText();
  if (mCrlf)
    text.replace("\n", "\r\n");

  QTextStream out(&file);
  out.setEncoding(mEncoding);
  out << text;
  out.flush();
  if (!file.commit())
    return false;

  mDocument->setModified(false);
  emit fileChanged();
  emit saved();
  return true;
}

void FileEditor::goToLine(int line) {
  mLine = line;
  if (!mDocument)
    return;

  QTextBlock block = mDocument->findBlockByNumber(line - 1);
  if (block.isValid())
    emit cursorRequested(block.position());
}

void FileEditor::find() {
  if (mDocument && !mDocument->isEmpty())
    mFinder->show();
}

void FileEditor::findNext() {
  if (mDocument && !mDocument->isEmpty())
    mFinder->next();
}

void FileEditor::findPrevious() {
  if (mDocument && !mDocument->isEmpty())
    mFinder->previous();
}

void FileEditor::setDocument(QQuickTextDocument *document) {
  if (!document || mDocument == document->textDocument())
    return;

  mDocument = document->textDocument();
  mHighlighter = new CodeHighlighter(mName, mDocument);

  connect(mDocument, &QTextDocument::modificationChanged, this,
          &FileEditor::modifiedChanged);

  // Follow the text with the blame and the matches.
  connect(mDocument, &QTextDocument::contentsChange, this,
          [this](int, int removed, int added) {
            if (removed || added)
              mBlameTimer.start();
          });

  QTextOption option = mDocument->defaultTextOption();
  if (Settings::instance()->value(Setting::Id::ShowWhitespaceInEditor).toBool())
    option.setFlags(QTextOption::ShowTabsAndSpaces);
  mDocument->setDefaultTextOption(option);

  setText();
}

int FileEditor::position(int row, int column) const {
  if (!mDocument)
    return 0;

  QTextBlock block = mDocument->findBlockByNumber(row);
  return block.isValid() ? block.position() + column : 0;
}

int FileEditor::row(int position) const {
  if (!mDocument)
    return 0;

  QTextBlock block = mDocument->findBlock(position);
  return block.isValid() ? block.blockNumber()
                         : qMax(0, mDocument->blockCount() - 1);
}

int FileEditor::findRowCount() const {
  return mDocument ? mDocument->blockCount() : 0;
}

QString FileEditor::findRowText(int row) const {
  if (!mDocument)
    return QString();

  return mDocument->findBlockByNumber(row).text();
}

void FileEditor::setFindState(const QString &text, int row, int start) {
  if (mHighlighter)
    mHighlighter->setFind(text, row, start);
}

void FileEditor::setText() {
  if (!mDocument)
    return;

  // Loading isn't a change that can be undone.
  mDocument->setUndoRedoEnabled(false);
  mDocument->setPlainText(mText);
  mDocument->setUndoRedoEnabled(true);
  mDocument->setModified(false);
  mBlameTimer.stop();

  if (mHighlighter) {
    mHighlighter->setPath(mName);
    mHighlighter->restyle();
  }

  if (mLine > 0)
    goToLine(mLine);
}

QByteArray FileEditor::text() const {
  QString text = mDocument ? mDocument->toPlainText() : mText;
  if (mCrlf)
    text.replace("\n", "\r\n");

  // Encode like the file is saved.
  QByteArray bytes;
  QTextStream out(&bytes);
  out.setEncoding(mEncoding);
  out << text;
  out.flush();
  return bytes;
}
