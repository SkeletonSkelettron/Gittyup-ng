//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef FILEEDITOR_H
#define FILEEDITOR_H

#include "FindController.h"
#include "git/Blob.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include <QObject>
#include <QPointer>
#include <QTimer>

class CodeHighlighter;
class FileViewModel;
class QQuickTextDocument;
class QTextDocument;

// Edits a file in the working copy, or shows a version of it, with the
// commit that last changed each line. qrc:/qml/EditorPage.qml draws it as
// 'editor' and gives it the document of its text area.
class FileEditor : public QObject, public FindTarget {
  Q_OBJECT

  Q_PROPERTY(QString name READ name NOTIFY fileChanged)
  Q_PROPERTY(QString revision READ revision NOTIFY fileChanged)
  Q_PROPERTY(bool readOnly READ isReadOnly NOTIFY fileChanged)
  Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
  Q_PROPERTY(int tabWidth READ tabWidth NOTIFY settingsChanged)
  Q_PROPERTY(bool wrapLines READ wrapLines NOTIFY settingsChanged)
  Q_PROPERTY(bool useTabs READ useTabs NOTIFY settingsChanged)
  Q_PROPERTY(int indentWidth READ indentWidth NOTIFY settingsChanged)
  Q_PROPERTY(QObject *blame READ blame CONSTANT)
  Q_PROPERTY(QObject *finder READ finder CONSTANT)

public:
  FileEditor(const git::Repository &repo, QObject *parent = nullptr);
  ~FileEditor() override;

  // Load the file 'name' of the repository, or any file if there's no
  // repository. A valid 'blob' is shown read-only. Binary files aren't
  // loaded.
  bool load(const QString &name, const git::Blob &blob,
            const git::Commit &commit);

  QString name() const { return mName; }
  QString path() const;
  QString revision() const { return mRevision; }
  bool isReadOnly() const { return mReadOnly; }
  bool isModified() const;
  int tabWidth() const;
  bool wrapLines() const;
  bool useTabs() const;
  int indentWidth() const;

  QObject *blame() const;
  QObject *finder() const;

  // Write the text to the file, and ask for its name if it's new. Returns
  // false if it wasn't saved.
  bool save();

  // Move the cursor to 'line', from 1.
  void goToLine(int line);

  void find();
  void findNext();
  void findPrevious();

  // Called by QML with the document of the text area.
  Q_INVOKABLE void setDocument(QQuickTextDocument *document);

  // The position in the document of 'column' of 'row', from 0.
  Q_INVOKABLE int position(int row, int column) const;
  // The row of 'position' in the document.
  Q_INVOKABLE int row(int position) const;

  int findRowCount() const override;
  QString findRowText(int row) const override;
  void setFindState(const QString &text, int row, int start) override;

signals:
  void fileChanged();
  void modifiedChanged();
  void settingsChanged();
  void saved();
  // Show the commit of a line.
  void linkActivated(const QString &link);
  // Move the cursor of the text area to 'position' and show it.
  void cursorRequested(int position);

private:
  void setText();
  QByteArray text() const;

  git::Repository mRepo;
  QString mName;
  QString mRevision;
  git::Commit mCommit;
  bool mReadOnly = false;

  // The text isn't in the document until QML gives it.
  QString mText;
  // The file is saved in the encoding it was read in.
  QStringConverter::Encoding mEncoding = QStringConverter::Utf8;
  bool mCrlf = false;
  int mLine = -1;

  QPointer<QTextDocument> mDocument;
  CodeHighlighter *mHighlighter = nullptr;
  FileViewModel *mBlame;
  FindController *mFinder;
  QTimer mBlameTimer;
};

#endif
