//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef UPTODATEDIALOG_H
#define UPTODATEDIALOG_H

#include "dialogs/ConfirmDialog.h"

// Tell that no update is available.
class UpToDateDialog : public ConfirmDialog {
  Q_OBJECT

public:
  UpToDateDialog(QWidget *parent = nullptr);
};

#endif
