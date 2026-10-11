//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Misha Sulikashvili
//

#ifndef CLI_H
#define CLI_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include "RunResult.h"

namespace git {

class Cli {
public:
  RunResult run(const QStringList &arguments, const QByteArray &input = {});
  explicit Cli(const QString &repositoryPath);
  static bool isAvailable();

private:
  static QString gitPath();
  QString mRepositoryPath;
};

} // namespace git

#endif
