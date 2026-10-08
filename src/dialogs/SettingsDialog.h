//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "QmlDialog.h"
#include <QStringList>
#include <QVariantList>

// The application settings. qrc:/qml/SettingsPage.qml draws them in
// sections. The page reads and writes settings by the names of Setting::Id
// and Prompt::Kind, and git settings by their keys in the global config.
class SettingsDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(int section READ section WRITE setSection NOTIFY sectionChanged)
  Q_PROPERTY(QVariantList languages READ languages CONSTANT)
  Q_PROPERTY(QStringList credentialStores READ credentialStores CONSTANT)
  Q_PROPERTY(QString credentialStoresInfo READ credentialStoresInfo CONSTANT)
  Q_PROPERTY(QStringList themes READ themes NOTIFY themesChanged)
  Q_PROPERTY(QString theme READ theme CONSTANT)
  Q_PROPERTY(bool themeEditable READ isThemeEditable NOTIFY themesChanged)
  Q_PROPERTY(QStringList fonts READ fonts CONSTANT)
  Q_PROPERTY(QStringList encodings READ encodings CONSTANT)
  Q_PROPERTY(bool updateDownloadVisible READ isUpdateDownloadVisible CONSTANT)
  Q_PROPERTY(bool terminalVisible READ isTerminalVisible CONSTANT)
  Q_PROPERTY(QString terminalInstallText READ terminalInstallText NOTIFY
                 terminalChanged)
  Q_PROPERTY(bool terminalInstallEnabled READ isTerminalInstallEnabled NOTIFY
                 terminalChanged)
  Q_PROPERTY(bool singleInstanceVisible READ isSingleInstanceVisible CONSTANT)
  Q_PROPERTY(QVariantList hotkeys READ hotkeys NOTIFY hotkeysChanged)

public:
  // Keep in sync with SettingsPage.qml.
  enum Index {
    General,
    Diff,
    Tools,
    Window,
    Editor,
    Update,
    Plugins,
    Misc,
    Hotkeys,
    Terminal
  };

  SettingsDialog(Index index, QWidget *parent = nullptr);

  static void openSharedInstance(Index index = General);

  int section() const { return mSection; }
  void setSection(int section);

  // Application settings by the name of their Setting::Id.
  Q_INVOKABLE QVariant setting(const QString &id) const;
  Q_INVOKABLE bool settingBool(const QString &id) const;
  Q_INVOKABLE int settingInt(const QString &id) const;
  Q_INVOKABLE QString settingString(const QString &id) const;
  Q_INVOKABLE void setSetting(const QString &id, const QVariant &value);

  // Prompts by the name of their Prompt::Kind.
  Q_INVOKABLE bool prompt(const QString &kind) const;
  Q_INVOKABLE void setPrompt(const QString &kind, bool prompt);
  Q_INVOKABLE QString promptText(const QString &kind) const;

  // Global git config values. An empty value removes the key.
  Q_INVOKABLE QString gitConfig(const QString &key) const;
  Q_INVOKABLE bool gitConfigBool(const QString &key) const;
  Q_INVOKABLE void setGitConfig(const QString &key, const QVariant &value);

  Q_INVOKABLE bool wrapLines() const;
  Q_INVOKABLE void setWrapLines(bool wrap);
  Q_INVOKABLE bool ignoreWhitespace() const;
  Q_INVOKABLE void setIgnoreWhitespace(bool ignore);

  // The index of the character encoding, where 0 is the system locale.
  Q_INVOKABLE int encoding() const;
  Q_INVOKABLE void setEncoding(int index);
  Q_INVOKABLE int diffContext() const;
  Q_INVOKABLE void setDiffContext(int lines);

  QVariantList languages() const;
  Q_INVOKABLE int language() const;
  Q_INVOKABLE void setLanguage(int index);

  QStringList credentialStores() const;
  QString credentialStoresInfo() const;
  Q_INVOKABLE bool storeCredentials() const;
  Q_INVOKABLE void setStoreCredentials(bool store, const QString &helper);
  Q_INVOKABLE QString credentialStore() const;

  // External diff and merge tools.
  Q_INVOKABLE QStringList tools(const QString &type) const;
  Q_INVOKABLE QString tool(const QString &type) const;
  Q_INVOKABLE void setTool(const QString &type, const QString &name);
  Q_INVOKABLE void configureTools(const QString &type);

  QStringList themes() const;
  QString theme() const;
  bool isThemeEditable() const;
  Q_INVOKABLE void setTheme(const QString &name);
  Q_INVOKABLE void addTheme(const QString &name);
  Q_INVOKABLE void editTheme();

  QStringList fonts() const;
  QStringList encodings() const;

  bool isUpdateDownloadVisible() const;
  Q_INVOKABLE void checkForUpdates();

  bool isTerminalVisible() const;
  QString terminalInstallText() const;
  bool isTerminalInstallEnabled() const;
  Q_INVOKABLE void toggleTerminalInstall();

  bool isSingleInstanceVisible() const;

  Q_INVOKABLE void editConfigFile();
  Q_INVOKABLE void showPrivacyPolicy();
  Q_INVOKABLE void configurePlugins();
  // Hotkeys by their index in the list, set from a key and its modifiers.
  QVariantList hotkeys() const;
  Q_INVOKABLE QString keyText(int key, int modifiers) const;
  Q_INVOKABLE QString hotkeyConflicts(int index, int key, int modifiers) const;
  Q_INVOKABLE void setHotkey(int index, int key, int modifiers);
  Q_INVOKABLE void clearHotkey(int index);
  Q_INVOKABLE void resetHotkey(int index);

signals:
  void sectionChanged();
  void themesChanged();
  void terminalChanged();
  void configChanged();
  void hotkeysChanged();

private:
  void refreshViews();

  int mSection = General;
};

#endif
