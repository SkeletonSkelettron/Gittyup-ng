//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Misha Sulikashvili

#ifndef RUNRESULT_H
#define RUNRESULT_H

#include <QByteArray>

namespace git {

struct RunResult {
  QByteArray output;
  QByteArray errors;
  int exitCode = -1;
  // git was stopped before it finished, see Cli::run().
  bool canceled = false;
};

} // namespace git

#endif
