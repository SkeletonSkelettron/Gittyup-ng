//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "SettingsDialog.h"
#include "AboutDialog.h"
#include "ConfirmDialog.h"
#include "ExternalToolsDialog.h"
#include "ui/HotkeyManager.h"
#include "PluginsDialog.h"
#include "app/CustomTheme.h"
#include "conf/Settings.h"
#include "cred/CredentialHelper.h"
#include "git/Config.h"
#include "tools/ExternalTool.h"
#include "ui/FileEditor.h"
#include "ui/EditorWindow.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include "languages.h"
#include "update/Updater.h"
#include <QApplication>
#include <QDirIterator>
#include <QFontDatabase>
#include <QMetaEnum>
#include <QRegularExpression>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <array>

#ifdef Q_OS_UNIX
#include "cli/Installer.h"
#endif

namespace {

const std::array kEncodings{
    "Utf8",  "Utf16",   "Utf16LE", "Utf16BE",
    "Utf32", "Utf32LE", "Utf32BE", "Latin1",
};

template <typename T> bool lookup(const QString &name, T &value) {
  bool ok = false;
  int key = QMetaEnum::fromType<T>().keyToValue(name.toUtf8().constData(), &ok);
  value = static_cast<T>(key);
  return ok;
}

} // namespace

SettingsDialog::SettingsDialog(Index index, QWidget *parent)
    : QmlDialog(parent), mSection(index) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Settings"));
  setContent("SettingsPage");
}

void SettingsDialog::openSharedInstance(Index index) {
  static QPointer<SettingsDialog> dialog;
  if (dialog) {
    dialog->setSection(index);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    return;
  }

  dialog = new SettingsDialog(index);
  dialog->show();
}

void SettingsDialog::setSection(int section) {
  if (section == mSection)
    return;

  mSection = section;
  emit sectionChanged();
}

QVariant SettingsDialog::setting(const QString &id) const {
  Setting::Id value;
  return lookup(id, value) ? Settings::instance()->value(value) : QVariant();
}

bool SettingsDialog::settingBool(const QString &id) const {
  return setting(id).toBool();
}

int SettingsDialog::settingInt(const QString &id) const {
  return setting(id).toInt();
}

QString SettingsDialog::settingString(const QString &id) const {
  return setting(id).toString();
}

void SettingsDialog::setSetting(const QString &id, const QVariant &value) {
  Setting::Id setting;
  if (!lookup(id, setting))
    return;

  Settings::instance()->setValue(setting, value);

  // Restart the fetch timers.
  if (setting == Setting::Id::FetchAutomatically) {
    for (MainWindow *window : MainWindow::windows()) {
      for (int i = 0; i < window->count(); ++i)
        window->view(i)->startFetchTimer();
    }
  }
}

bool SettingsDialog::prompt(const QString &kind) const {
  Prompt::Kind value;
  return lookup(kind, value) && Settings::instance()->prompt(value);
}

void SettingsDialog::setPrompt(const QString &kind, bool prompt) {
  Prompt::Kind value;
  if (lookup(kind, value))
    Settings::instance()->setPrompt(value, prompt);
}

QString SettingsDialog::promptText(const QString &kind) const {
  Prompt::Kind value;
  return lookup(kind, value) ? Settings::instance()->promptDescription(value)
                             : QString();
}

QString SettingsDialog::gitConfig(const QString &key) const {
  return git::Config::global().value<QString>(key);
}

bool SettingsDialog::gitConfigBool(const QString &key) const {
  return git::Config::global().value<bool>(key);
}

void SettingsDialog::setGitConfig(const QString &key, const QVariant &value) {
  git::Config config = git::Config::global();
  if (value.typeId() == QMetaType::Bool) {
    config.setValue(key, value.toBool());
  } else if (value.toString().isEmpty()) {
    config.remove(key);
  } else {
    config.setValue(key, value.toString());
  }

  emit configChanged();
}

bool SettingsDialog::wrapLines() const {
  return Settings::instance()->isTextEditorWrapLines();
}

void SettingsDialog::setWrapLines(bool wrap) {
  Settings::instance()->setTextEditorWrapLines(wrap);
}

bool SettingsDialog::ignoreWhitespace() const {
  return Settings::instance()->isWhitespaceIgnored();
}

void SettingsDialog::setIgnoreWhitespace(bool ignore) {
  Settings::instance()->setWhitespaceIgnored(ignore);
}

int SettingsDialog::encoding() const {
  QString name = gitConfig("gui.encoding");
  for (int i = 0; i < static_cast<int>(kEncodings.size()); ++i) {
    if (name == kEncodings[i])
      return i + 1;
  }

  return 0;
}

void SettingsDialog::setEncoding(int index) {
  git::Config config = git::Config::global();
  if (index <= 0 || index > static_cast<int>(kEncodings.size())) {
    config.remove("gui.encoding");
  } else {
    config.setValue("gui.encoding", QString(kEncodings[index - 1]));
  }

  refreshViews();
}

int SettingsDialog::diffContext() const {
  return git::Config::global().value<int>("diff.context", 3);
}

void SettingsDialog::setDiffContext(int lines) {
  git::Config::global().setValue("diff.context", lines);
  refreshViews();
}

QVariantList SettingsDialog::languages() const {
  QVariantList languages;
  QMapIterator<const char *, const char *> it(Languages::languages);
  while (it.hasNext()) {
    it.next();
    languages.append(QVariantMap{{"text", tr(it.key())},
                                 {"code", QString(it.value())}});
  }

  return languages;
}

int SettingsDialog::language() const {
  QString code = Settings::instance()->value(Setting::Id::Language).toString();
  QVariantList languages = this->languages();
  for (int i = 0; i < languages.size(); ++i) {
    if (languages.at(i).toMap().value("code").toString() == code)
      return i;
  }

  return -1;
}

void SettingsDialog::setLanguage(int index) {
  QVariantList languages = this->languages();
  if (index >= 0 && index < languages.size())
    Settings::instance()->setValue(
        Setting::Id::Language, languages.at(index).toMap().value("code"));
}

QStringList SettingsDialog::credentialStores() const {
  QStringList stores;
  for (const auto &helper : CredentialHelper::getAvailableHelperInformation())
    stores.append(helper.name);
  return stores;
}

QString SettingsDialog::credentialStoresInfo() const {
  QStringList lines;
  for (const auto &helper : CredentialHelper::getAvailableHelperInformation())
    lines.append(QString("<b>%1</b> — %2").arg(helper.name, helper.description));
  return lines.join("<br>");
}

bool SettingsDialog::storeCredentials() const {
  return CredentialHelper::isHelperValid(credentialStore());
}

void SettingsDialog::setStoreCredentials(bool store, const QString &helper) {
  git::Config config = git::Config::global();
  if (store && !helper.isEmpty()) {
    config.setValue("credential.helper", helper);
  } else {
    config.remove("credential.helper");
  }

  delete CredentialHelper::instance();
  emit configChanged();
}

QString SettingsDialog::credentialStore() const {
  return gitConfig("credential.helper");
}

QStringList SettingsDialog::tools(const QString &type) const {
  QList<ExternalTool::Info> tools = ExternalTool::readGlobalTools(type) +
                                    ExternalTool::readBuiltInTools(type);

  QStringList names;
  for (const ExternalTool::Info &tool : tools) {
    if (tool.found && !names.contains(tool.name))
      names.append(tool.name);
  }

  std::sort(names.begin(), names.end());
  return names;
}

QString SettingsDialog::tool(const QString &type) const {
  return gitConfig(QString("%1.tool").arg(type));
}

void SettingsDialog::setTool(const QString &type, const QString &name) {
  git::Config::global().setValue(QString("%1.tool").arg(type), name);
  emit configChanged();
}

void SettingsDialog::configureTools(const QString &type) {
  ExternalToolsDialog *dialog = new ExternalToolsDialog(type, this);
  connect(dialog, &QDialog::finished, this, &SettingsDialog::configChanged);
  dialog->open();
}

QStringList SettingsDialog::themes() const {
  QStringList themes = {"Default"};

  // Predefined themes.
  QDir dir = Settings::themesDir();
  dir.setNameFilters({"*.lua"});
  QDirIterator it(dir);
  QStringList predefined;
  while (it.hasNext()) {
    it.next();
    QString name = it.fileInfo().baseName();
    if (name != "Default")
      predefined.append(name);
  }
  predefined.sort();
  themes += predefined;

  // User themes.
  bool exists = false;
  QDir userDir = CustomTheme::userDir(false, &exists);
  if (exists) {
    userDir.setNameFilters({"*.lua"});
    QDirIterator userIt(userDir);
    while (userIt.hasNext()) {
      userIt.next();
      QString name = userIt.fileInfo().baseName();
      if (!themes.contains(name))
        themes.append(name);
    }
  }

  return themes;
}

QString SettingsDialog::theme() const {
  QString theme =
      Settings::instance()->value(Setting::Id::ColorTheme).toString();
  return theme.isEmpty() ? QString("Default") : theme;
}

bool SettingsDialog::isThemeEditable() const {
  bool exists = false;
  QDir dir = CustomTheme::userDir(false, &exists);
  return exists && dir.exists(QString("%1.lua").arg(theme()));
}

void SettingsDialog::setTheme(const QString &name) {
  if (name == theme())
    return;

  Settings::instance()->setValue(Setting::Id::ColorTheme, name);

  ConfirmDialog dialog(this);
  dialog.setTitle(tr("Restart?"));
  dialog.setText(tr("The application must be restarted for "
                    "the theme change to take effect."));
  dialog.setInformativeText(tr("Do you want to restart now?"));
  dialog.setAcceptText(tr("Restart"));
  if (dialog.exec() != QDialog::Accepted)
    return;

  QTimer::singleShot(0, this, [this] {
    close();

    // Restart the app.
    QStringList args = qApp->arguments();
    QProcess::startDetached(args.takeFirst(), args);
    qApp->quit();
  });
}

void SettingsDialog::addTheme(const QString &name) {
  if (name.isEmpty())
    return;

  QDir dir = CustomTheme::userDir(true);
  QString path = dir.filePath(QString("%1.lua").arg(name));
  QFile::copy(Settings::themesDir().filePath("Dark.lua"), path);
  EditorWindow::open(path);
  close();
}

void SettingsDialog::editTheme() {
  if (!isThemeEditable())
    return;

  QDir dir = CustomTheme::userDir(false);
  EditorWindow::open(dir.filePath(QString("%1.lua").arg(theme())));
  close();
}

QStringList SettingsDialog::fonts() const { return QFontDatabase::families(); }

QStringList SettingsDialog::encodings() const {
  QStringList encodings = {tr("System Locale")};
  for (const char *encoding : kEncodings)
    encodings.append(encoding);
  return encodings;
}

bool SettingsDialog::isUpdateDownloadVisible() const {
#if !defined(Q_OS_LINUX) || defined(FLATPAK)
  return true;
#else
  // Packages are updated by the package manager on Linux.
  return false;
#endif
}

void SettingsDialog::checkForUpdates() { Updater::instance()->update(); }

bool SettingsDialog::isTerminalVisible() const {
#ifdef Q_OS_UNIX
  return true;
#else
  return false;
#endif
}

QString SettingsDialog::terminalInstallText() const {
#ifdef Q_OS_UNIX
  Settings *settings = Settings::instance();
  Installer installer(settings->value(Setting::Id::TerminalName).toString(),
                      settings->value(Setting::Id::TerminalPath).toString());
  return installer.isInstalled() ? tr("Uninstall") : tr("Install");
#else
  return QString();
#endif
}

bool SettingsDialog::isTerminalInstallEnabled() const {
#ifdef Q_OS_UNIX
  Settings *settings = Settings::instance();
  Installer installer(settings->value(Setting::Id::TerminalName).toString(),
                      settings->value(Setting::Id::TerminalPath).toString());
  return installer.isInstalled() || !installer.exists();
#else
  return false;
#endif
}

void SettingsDialog::toggleTerminalInstall() {
#ifdef Q_OS_UNIX
  Settings *settings = Settings::instance();
  Installer installer(settings->value(Setting::Id::TerminalName).toString(),
                      settings->value(Setting::Id::TerminalPath).toString());
  if (installer.isInstalled()) {
    installer.uninstall();
  } else if (!installer.exists()) {
    installer.install();
  }

  emit terminalChanged();
#endif
}

bool SettingsDialog::isSingleInstanceVisible() const {
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
  return true;
#else
  return false;
#endif
}

void SettingsDialog::editConfigFile() {
  if (EditorWindow *window = EditorWindow::open(git::Config::globalPath()))
    connect(window->editor(), &FileEditor::saved, this,
            &SettingsDialog::configChanged);
}

void SettingsDialog::showPrivacyPolicy() {
  AboutDialog::openSharedInstance(AboutDialog::Privacy);
}

void SettingsDialog::configurePlugins() {
  (new PluginsDialog(git::Repository(), this))->open();
}

QVariantList SettingsDialog::hotkeys() const {
  static QRegularExpression slashes("/+");

  QVariantList hotkeys;
  QVector<Hotkey> known = HotkeyManager::instance()->knownHotkeys();
  for (int i = 0; i < known.size(); ++i) {
    const Hotkey &hotkey = known.at(i);
    QString label = hotkey.label().replace(slashes, "/");
    int sep = label.lastIndexOf('/');
    QStringList groups;
    for (const QString &group : label.left(qMax(0, sep)).split('/'))
      groups.append(tr(group.toUtf8()));

    QKeySequence keys = hotkey.currentKeys();
    hotkeys.append(QVariantMap{
        {"index", i},
        {"group", groups.join(" › ")},
        {"label", tr(label.mid(sep + 1).toUtf8())},
        {"keys", keys.toString(QKeySequence::NativeText)},
        {"custom", keys != hotkey.defaultSequence()}});
  }

  return hotkeys;
}

QString SettingsDialog::keyText(int key, int modifiers) const {
  // Wait for a key that isn't a modifier.
  switch (key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
      return QKeySequence(QKeyCombination::fromCombined(modifiers))
          .toString(QKeySequence::NativeText);
  }

  QKeyCombination combination(Qt::KeyboardModifiers(modifiers),
                              static_cast<Qt::Key>(key));
  return QKeySequence(combination).toString(QKeySequence::NativeText);
}

QString SettingsDialog::hotkeyConflicts(int index, int key,
                                        int modifiers) const {
  QVector<Hotkey> known = HotkeyManager::instance()->knownHotkeys();
  QKeySequence keys(QKeyCombination(Qt::KeyboardModifiers(modifiers),
                                    static_cast<Qt::Key>(key)));
  QStringList conflicts;
  for (int i = 0; i < known.size(); ++i) {
    if (i != index && known.at(i).currentKeys() == keys)
      conflicts.append(known.at(i).label().section('/', -1));
  }

  return conflicts.join(", ");
}

void SettingsDialog::setHotkey(int index, int key, int modifiers) {
  QVector<Hotkey> known = HotkeyManager::instance()->knownHotkeys();
  if (index < 0 || index >= known.size())
    return;

  Hotkey hotkey = known.at(index);
  hotkey.setKeys(QKeySequence(QKeyCombination(Qt::KeyboardModifiers(modifiers),
                                              static_cast<Qt::Key>(key))));
  emit hotkeysChanged();
}

void SettingsDialog::clearHotkey(int index) {
  QVector<Hotkey> known = HotkeyManager::instance()->knownHotkeys();
  if (index < 0 || index >= known.size())
    return;

  Hotkey hotkey = known.at(index);
  hotkey.setKeys(QKeySequence());
  emit hotkeysChanged();
}

void SettingsDialog::resetHotkey(int index) {
  QVector<Hotkey> known = HotkeyManager::instance()->knownHotkeys();
  if (index < 0 || index >= known.size())
    return;

  Hotkey hotkey = known.at(index);
  hotkey.setKeys(hotkey.defaultSequence());
  emit hotkeysChanged();
}

void SettingsDialog::refreshViews() {
  for (MainWindow *window : MainWindow::windows()) {
    for (int i = 0; i < window->count(); ++i)
      window->view(i)->refresh();
  }
}
