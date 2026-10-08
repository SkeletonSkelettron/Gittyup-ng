//
//          Copyright (c) 2022, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef AMENDDIALOG_H
#define AMENDDIALOG_H

#include "QmlDialog.h"
#include "git/Signature.h"
#include <QDateTime>

struct ContributorInfo {
  enum class SelectedDateTimeType { Current, Manual, Original };
  QString name;
  QString email;
  QDateTime commitDate;
  SelectedDateTimeType commitDateType;
};

struct AmendInfo {
  ContributorInfo authorInfo;
  ContributorInfo committerInfo;
  QString commitMessage;
};

// The author or the committer of the amended commit.
class AmendContributor : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString title READ title CONSTANT)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
  Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY changed)
  // A ContributorInfo::SelectedDateTimeType.
  Q_PROPERTY(int dateType READ dateTypeValue WRITE setDateTypeValue NOTIFY
                 changed)
  Q_PROPERTY(QString dateText READ dateText WRITE setDateText NOTIFY changed)
  Q_PROPERTY(bool dateValid READ isDateValid NOTIFY changed)
  Q_PROPERTY(QString originalDate READ originalDate CONSTANT)

public:
  AmendContributor(const QString &title, const git::Signature &signature,
                   QObject *parent = nullptr);

  QString title() const { return mTitle; }

  QString name() const { return mName; }
  void setName(const QString &name);

  QString email() const { return mEmail; }
  void setEmail(const QString &email);

  ContributorInfo::SelectedDateTimeType dateType() const { return mDateType; }
  void setDateType(ContributorInfo::SelectedDateTimeType type);
  int dateTypeValue() const { return static_cast<int>(mDateType); }
  void setDateTypeValue(int type);

  // The date that is used when it's set manually.
  QDateTime manualDate() const;
  void setManualDate(const QDateTime &date);
  QString dateText() const { return mDateText; }
  void setDateText(const QString &text);
  bool isDateValid() const;

  QString originalDate() const;

  ContributorInfo info() const;

signals:
  void changed();

private:
  QString mTitle;
  git::Signature mSignature;
  QString mName;
  QString mEmail;
  ContributorInfo::SelectedDateTimeType mDateType =
      ContributorInfo::SelectedDateTimeType::Current;
  QString mDateText;
};

// Change the author, the committer and the message of the last commit.
// qrc:/qml/AmendDialog.qml draws it.
class AmendDialog : public QmlDialog {
  Q_OBJECT

  Q_PROPERTY(QObject *author READ authorObject CONSTANT)
  Q_PROPERTY(QObject *committer READ committerObject CONSTANT)
  Q_PROPERTY(QString message READ commitMessage WRITE setCommitMessage NOTIFY
                 messageChanged)
  Q_PROPERTY(bool acceptable READ isAcceptable NOTIFY acceptableChanged)

public:
  AmendDialog(const git::Signature &author, const git::Signature &committer,
              const QString &commitMessage, QWidget *parent = nullptr);

  AmendInfo getInfo() const;

  AmendContributor *author() const { return mAuthor; }
  AmendContributor *committer() const { return mCommitter; }
  QObject *authorObject() const { return mAuthor; }
  QObject *committerObject() const { return mCommitter; }

  QString commitMessage() const { return mMessage; }
  void setCommitMessage(const QString &message);

  bool isAcceptable() const;

signals:
  void messageChanged();
  void acceptableChanged();

private:
  AmendContributor *mAuthor;
  AmendContributor *mCommitter;
  QString mMessage;
};

#endif
