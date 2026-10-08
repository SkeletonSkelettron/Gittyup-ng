//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "SpellCheck.h"
#include "qml/QmlSupport.h"
#include "RepoView.h"
#include "SpellChecker.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include <QActionGroup>
#include <QFile>
#include <QLocale>
#include <QMenu>
#include <QRegularExpression>

namespace {

const QString kDictKey = "commit.spellcheck.dict";

// Words are letters with inner apostrophes. Words with digits or
// underscores are usually identifiers, which aren't checked.
const QRegularExpression kWordPattern("[\\w']+");

bool isCheckable(const QString &word) {
  for (QChar ch : word) {
    if (ch.isDigit() || ch == '_')
      return false;
  }

  return word.size() > 1;
}

} // namespace

SpellCheck::SpellCheck(const git::Repository &repo, RepoView *view)
    : QObject(view), mRepo(repo), mView(view) {
  mUserDictionary = Settings::userDir().path() + "/user.dic";
  QFile userDict(mUserDictionary);
  if (!userDict.exists() && userDict.open(QIODevice::WriteOnly))
    userDict.close();

  // Use the configured dictionary, or the one of the system language.
  QString name = mRepo.appConfig().value<QString>(kDictKey, "system");
  QStringList dicts = dictionaries();
  QString dictionary;
  if (name != "none") {
    QString system = QLocale::system().name();
    for (const QString &dict : dicts) {
      if (name != "system" ? dict.startsWith(name) : dict.startsWith(system))
        dictionary = dict;
    }

    // Ignore the country of the system language.
    if (dictionary.isEmpty() && name == "system") {
      for (const QString &dict : dicts) {
        if (dict.startsWith(system.left(2))) {
          dictionary = dict;
          break;
        }
      }
    }
  }

  setDictionary(dictionary, false);
}

SpellCheck::~SpellCheck() {}

QVariantList SpellCheck::misspelled(const QString &text, int revision) const {
  Q_UNUSED(revision)

  QVariantList ranges;
  if (!mChecker)
    return ranges;

  QRegularExpressionMatchIterator it = kWordPattern.globalMatch(text);
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    QString word = match.captured().remove(QRegularExpression("^'+|'+$"));
    if (!isCheckable(word) || mIgnored.contains(word) ||
        mChecker->spell(word))
      continue;

    int start = match.capturedStart() + match.captured().indexOf(word);
    ranges.append(QVariantMap{{"start", start}, {"length", word.size()}});
  }

  return ranges;
}

void SpellCheck::showMenu(const QString &field, const QString &text,
                          int position, qreal x, qreal y) {
  Q_UNUSED(x)
  Q_UNUSED(y)

  QMenu menu;
  Word word = wordAt(text, position);
  bool misspelled = mChecker && word.start >= 0 && isCheckable(word.text) &&
                    !mIgnored.contains(word.text) &&
                    !mChecker->spell(word.text);

  if (misspelled) {
    QStringList suggestions = mChecker->suggest(word.text);
    for (const QString &suggestion : suggestions.mid(0, 8)) {
      QAction *action = menu.addAction(suggestion, this, [=] {
        emit replaceRequested(field, word.start, word.length, suggestion);
      });
      QFont font = action->font();
      font.setBold(true);
      action->setFont(font);
    }

    if (suggestions.isEmpty())
      menu.addAction(tr("No Suggestions"))->setEnabled(false);

    menu.addSeparator();
    menu.addAction(tr("Ignore \"%1\"").arg(word.text), this, [this, word] {
      mIgnored.insert(word.text);
      mChecker->ignoreWord(word.text);
      ++mRevision;
      emit changed();
    });
    menu.addAction(tr("Add to User Dictionary"), this, [this, word] {
      mChecker->addToUserDict(word.text);
      ++mRevision;
      emit changed();
    });
    menu.addSeparator();
  }

  // Choose the dictionary.
  QMenu *languages = menu.addMenu(tr("Spell Check Language"));
  QActionGroup *group = new QActionGroup(languages);
  QAction *none = languages->addAction(tr("None"));
  none->setCheckable(true);
  none->setChecked(mDictionary.isEmpty());
  group->addAction(none);
  connect(none, &QAction::triggered, this, [this] { setDictionary(QString()); });

  languages->addSeparator();
  for (const QString &dict : dictionaries()) {
    QAction *action = languages->addAction(dictionaryText(dict));
    action->setCheckable(true);
    action->setChecked(dict == mDictionary);
    group->addAction(action);
    connect(action, &QAction::triggered, this,
            [this, dict] { setDictionary(dict); });
  }

  menu.addAction(tr("Edit User Dictionary"), this,
                 [this] { mView->openEditor(mUserDictionary); });

  QmlSupport::execMenu(&menu, QCursor::pos());
}

SpellCheck::Word SpellCheck::wordAt(const QString &text, int position) const {
  QRegularExpressionMatchIterator it = kWordPattern.globalMatch(text);
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    if (position >= match.capturedStart() && position <= match.capturedEnd()) {
      QString word = match.captured().remove(QRegularExpression("^'+|'+$"));
      Word result;
      result.start = match.capturedStart() + match.captured().indexOf(word);
      result.length = word.size();
      result.text = word;
      return result;
    }
  }

  return Word();
}

void SpellCheck::setDictionary(const QString &name, bool save) {
  mDictionary = name;
  mChecker.reset();
  if (!name.isEmpty()) {
    QString path = Settings::dictionariesDir().filePath(name);
    auto checker = std::make_unique<SpellChecker>(path, mUserDictionary);
    if (checker->isValid()) {
      mChecker = std::move(checker);
    } else {
      mDictionary.clear();
    }
  }

  if (save)
    mRepo.appConfig().setValue(kDictKey,
                               mDictionary.isEmpty() ? "none" : mDictionary);
  ++mRevision;
  emit changed();
}

QStringList SpellCheck::dictionaries() const {
  QStringList names =
      Settings::dictionariesDir().entryList({"*.dic"}, QDir::Files, QDir::Name);
  names.replaceInStrings(".dic", "");
  return names;
}

QString SpellCheck::dictionaryText(const QString &name) const {
  // Convert the language_COUNTRY format of the file name.
  QLocale locale(name);
  QString language = QLocale::languageToString(locale.language());
  if (language == "C")
    return name;

  QString territory = QLocale::territoryToString(locale.territory());
  return territory != "Default" ? QString("%1 (%2)").arg(language, territory)
                                : language;
}
