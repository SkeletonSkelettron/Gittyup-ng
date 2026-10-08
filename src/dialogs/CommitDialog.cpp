//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CommitDialog.h"

CommitDialog::CommitDialog(const QString &message, Prompt::Kind kind,
                           QWidget *parent)
    : QmlDialog(parent), mKind(kind), mMessage(message),
      mRejectText(tr("Abort")) {
  setAttribute(Qt::WA_DeleteOnClose);

  switch (kind) {
    case Prompt::Kind::Merge:
      mTitle = tr("Merge commit message");
      mAcceptText = tr("Merge");
      break;

    case Prompt::Kind::Stash:
      mTitle = tr("Stash commit message");
      mAcceptText = tr("Stash");
      mRejectText = tr("Cancel");
      break;

    case Prompt::Kind::Revert:
      mTitle = tr("Revert commit message");
      mAcceptText = tr("Revert");
      break;

    case Prompt::Kind::CherryPick:
      mTitle = tr("Cherry-pick commit message");
      mAcceptText = tr("Cherry-pick");
      break;

    case Prompt::Kind::Directories:
    case Prompt::Kind::LargeFiles:
      Q_ASSERT(false);
      break;
  }

  setWindowTitle(mTitle);
  setContent("CommitDialog");
}

void CommitDialog::setMessage(const QString &message) {
  if (message == mMessage)
    return;

  mMessage = message;
  emit messageChanged();
}

QString CommitDialog::promptText() const {
  return Settings::instance()->promptDescription(mKind);
}

bool CommitDialog::prompt() const {
  return Settings::instance()->prompt(mKind);
}

void CommitDialog::setPrompt(bool prompt) {
  Settings::instance()->setPrompt(mKind, prompt);
  emit promptChanged();
}
