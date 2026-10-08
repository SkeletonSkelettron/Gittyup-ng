//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef SPELLCHECK_H
#define SPELLCHECK_H

#include "git/Repository.h"
#include <QObject>
#include <QSet>
#include <QVariantList>
#include <memory>

class RepoView;
class SpellChecker;

// Spell checking of the commit message. The QML fields underline the
// ranges of misspelled words and show the menu to correct them.
class SpellCheck : public QObject {
  Q_OBJECT

  // Changes when the dictionary or the ignored words change.
  Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
  SpellCheck(const git::Repository &repo, RepoView *view);
  ~SpellCheck() override;

  int revision() const { return mRevision; }

  // The misspelled words of the text as {start, length}.
  Q_INVOKABLE QVariantList misspelled(const QString &text,
                                      int revision) const;

  // Show the menu of the word at 'position'. Suggestions are applied by
  // replaceRequested() for the field.
  Q_INVOKABLE void showMenu(const QString &field, const QString &text,
                            int position, qreal x, qreal y);

signals:
  void changed();
  void replaceRequested(const QString &field, int start, int length,
                        const QString &text);

private:
  struct Word {
    int start = -1;
    int length = 0;
    QString text;
  };

  Word wordAt(const QString &text, int position) const;
  void setDictionary(const QString &name, bool save = true);
  QStringList dictionaries() const;
  QString dictionaryText(const QString &name) const;

  git::Repository mRepo;
  RepoView *mView;
  std::unique_ptr<SpellChecker> mChecker;
  QString mDictionary;
  QString mUserDictionary;
  QSet<QString> mIgnored;
  int mRevision = 0;
};

#endif
