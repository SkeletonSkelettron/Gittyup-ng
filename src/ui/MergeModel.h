//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef MERGEMODEL_H
#define MERGEMODEL_H

#include "git/Repository.h"
#include <QAbstractListModel>
#include <QPointer>
#include <QScopedPointer>
#include <QTextCursor>
#include <QTimer>
#include <QVariantList>

class CodeHighlighter;
class MergeModel;
class QQuickTextDocument;
class QTextDocument;
class RepoView;
class SyntaxHighlighter;

// The lines of one side of a conflicted file, with the lines of its
// conflicts that can be taken into the output.
class MergeSideModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum Kind { CommonRow, ConflictRow, LineRow };

  enum Role {
    KindRole = Qt::UserRole,
    ConflictRole,
    LineRole,
    NumberRole,
    HtmlRole,
    CheckedRole,
    // 0 when no line of the conflict is taken, 1 for some and 2 for all.
    CheckStateRole
  };

  MergeSideModel(MergeModel *merge, int side);
  ~MergeSideModel() override;

  // The row of the header of a conflict.
  int conflictRow(int conflict) const;

  void reset();
  void updateChecks(int conflict);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  struct Row {
    Kind kind;
    int conflict = -1;
    int line = -1;
    int number = 0;
    QString text;
  };

  MergeModel *mMerge;
  int mSide;
  QList<Row> mRows;
  QList<QByteArray> mStyles;
  QScopedPointer<SyntaxHighlighter> mHighlighter;
};

// A conflicted file resolved like in GitKraken: the lines of the conflicts
// of each side are taken into the output, which can also be edited.
// qrc:/qml/MergePanel.qml draws it as 'detailView.merge'. Side 0 is ours
// and side 1 is theirs.
class MergeModel : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString path READ path NOTIFY loaded)
  Q_PROPERTY(QString notice READ notice NOTIFY loaded)
  Q_PROPERTY(QString oursLabel READ oursLabel NOTIFY loaded)
  Q_PROPERTY(QString theirsLabel READ theirsLabel NOTIFY loaded)
  Q_PROPERTY(int conflictCount READ conflictCount NOTIFY loaded)
  Q_PROPERTY(int unresolvedCount READ unresolvedCount NOTIFY selectionChanged)
  Q_PROPERTY(int lineNumberWidth READ lineNumberWidth NOTIFY loaded)
  Q_PROPERTY(QObject *ours READ ours CONSTANT)
  Q_PROPERTY(QObject *theirs READ theirs CONSTANT)
  // The ranges of the conflicts in the output, as 'start', 'middle' and
  // 'end' positions. The lines of the 'first' side are before 'middle'.
  Q_PROPERTY(QVariantList regions READ regions NOTIFY regionsChanged)
  // The parts of the file in order, as the 'conflict' or -1 for common lines,
  // and the number of 'lines' of each side.
  Q_PROPERTY(QVariantList layout READ layout NOTIFY loaded)

public:
  struct Conflict {
    QStringList lines[2];
    QStringList base;
    QList<bool> checked[2];
    // Whether a side without lines is taken: the conflict resolves without
    // lines of it.
    bool empty[2] = {false, false};
    // The side whose lines come first in the output.
    int first = 0;
    bool touched = false;
  };

  // A part of the file, either lines that are the same on both sides or a
  // conflict.
  struct Segment {
    QStringList common;
    int conflict = -1;
  };

  MergeModel(RepoView *view, QObject *parent = nullptr);
  ~MergeModel() override;

  // Load the conflicted file 'path' from the working copy. Nothing changes
  // if it's already loaded and wasn't changed.
  void load(const QString &path);
  void clear();

  QString path() const { return mPath; }
  QString notice() const { return mNotice; }
  QString oursLabel() const { return mLabels[0]; }
  QString theirsLabel() const { return mLabels[1]; }
  int conflictCount() const { return mConflicts.size(); }
  int unresolvedCount() const;
  int lineNumberWidth() const;
  QObject *ours() const;
  QObject *theirs() const;
  QVariantList regions() const;
  QVariantList layout() const;

  const QList<Segment> &segments() const { return mSegments; }
  const QList<Conflict> &conflicts() const { return mConflicts; }

  // Called by QML with the document of the output.
  Q_INVOKABLE void setDocument(QQuickTextDocument *document);
  void setTextDocument(QTextDocument *document);

  Q_INVOKABLE void setLineChecked(int side, int conflict, int line,
                                  bool checked);
  // Take all lines of a side of a conflict, or the side when it has none.
  Q_INVOKABLE void setConflictChecked(int side, int conflict, bool checked);
  // Take all lines of the conflicts of 'side', and none of the other side.
  Q_INVOKABLE void takeAll(int side);

  // The row of a conflict in the model of a side, and its position in the
  // output.
  Q_INVOKABLE int conflictRow(int side, int conflict) const;
  Q_INVOKABLE int regionStart(int conflict) const;

  // The row of a position in the output, and the position of a row.
  Q_INVOKABLE int outputRow(int position) const;
  Q_INVOKABLE int outputPosition(int row) const;

  // Write the output to the file and mark it as resolved.
  Q_INVOKABLE void save();

signals:
  void loaded();
  void selectionChanged();
  void regionsChanged();

private:
  struct Region {
    QTextCursor start;
    QTextCursor middle;
    QTextCursor end;
  };

  bool parse(const QString &text);
  void setOutput();
  void updateRegion(int conflict);

  RepoView *mView;
  MergeSideModel *mSides[2];

  QString mPath;
  QString mNotice;
  QString mLabels[2];
  QByteArray mContent;
  bool mCrlf = false;
  bool mTrailingNewline = true;
  // The output is saved in the encoding of the file.
  QStringConverter::Encoding mEncoding = QStringConverter::Utf8;

  QList<Segment> mSegments;
  QList<Conflict> mConflicts;

  QPointer<QTextDocument> mDocument;
  CodeHighlighter *mHighlighter = nullptr;
  QList<Region> mRegions;
  QTimer mRegionsTimer;
};

#endif
