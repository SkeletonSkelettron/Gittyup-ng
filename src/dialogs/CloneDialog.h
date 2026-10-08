//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CLONEDIALOG_H
#define CLONEDIALOG_H

#include "QmlDialog.h"
#include "host/Repository.h"
#include <QFutureWatcher>

class LogEntry;
class LogPanel;
class RemoteCallbacks;

namespace git {
class Result;
}

// Clone or initialize a repository in steps: the remote URL, the location
// and the progress of the clone. qrc:/qml/CloneDialog.qml draws it.
class CloneDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(bool init READ isInit CONSTANT)
  Q_PROPERTY(int step READ step NOTIFY stepChanged)
  Q_PROPERTY(bool hosted READ isHosted CONSTANT)
  Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY changed)
  Q_PROPERTY(int protocol READ protocol WRITE setProtocol NOTIFY changed)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
  Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY
                 changed)
  Q_PROPERTY(bool bare READ isBare WRITE setBare NOTIFY changed)
  Q_PROPERTY(QString targetPath READ path NOTIFY changed)
  Q_PROPERTY(bool canContinue READ canContinue NOTIFY changed)
  Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
  Q_PROPERTY(bool failed READ hasFailed NOTIFY busyChanged)

public:
  enum Kind { Init, Clone };
  // Keep in sync with CloneDialog.qml.
  enum Step { RemoteStep, LocationStep, ProgressStep };

  CloneDialog(Kind kind, QWidget *parent = nullptr, Repository *repo = nullptr);
  ~CloneDialog() override;

  void accept() override;
  void reject() override;

  QString path() const;
  QString message() const;
  QString messageTitle() const;

  // The values of the steps: "url", "name", "path" and "bare".
  QVariant field(const QString &name) const;
  void setField(const QString &name, const QVariant &value);

  bool isInit() const { return mInit; }
  int step() const { return mStep; }
  bool isHosted() const { return mRepo; }

  QString url() const { return mUrl; }
  void setUrl(const QString &url);

  int protocol() const { return mProtocol; }
  void setProtocol(int protocol);

  QString name() const { return mName; }
  void setName(const QString &name);

  QString directory() const { return mDirectory; }
  void setDirectory(const QString &directory);

  bool isBare() const { return mBare; }
  void setBare(bool bare);

  bool canContinue() const;
  bool isBusy() const { return mWatcher; }
  bool hasFailed() const { return mFailed; }

  Q_INVOKABLE void next();
  Q_INVOKABLE void back();
  Q_INVOKABLE void browseUrl();
  Q_INVOKABLE void browseDirectory();

  // Start cloning into the location.
  void startClone();

signals:
  void changed();
  void stepChanged();
  void busyChanged();

private:
  void setStep(int step);
  void cancel();
  void error(LogEntry *entry, const QString &action, const QString &name,
             const QString &defaultError);

  bool mInit;
  Repository *mRepo;
  int mStep = RemoteStep;
  QString mUrl;
  int mProtocol = Repository::Https;
  QString mName;
  QString mDirectory;
  bool mBare = false;
  bool mFailed = false;

  LogEntry *mLogRoot;
  LogPanel *mLogPanel;
  RemoteCallbacks *mCallbacks = nullptr;
  QFutureWatcher<git::Result> *mWatcher = nullptr;
};

#endif
