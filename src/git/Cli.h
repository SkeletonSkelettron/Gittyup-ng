//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Misha Sulikashvili
//

#ifndef CLI_H
#define CLI_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QStringList>
#include <functional>
#include "RunResult.h"

namespace git {

class Cli {
public:
  // Stops git when 'canceled' returns true, which it asks while git runs.
  RunResult run(const QStringList &arguments, const QByteArray &input = {},
                const QMap<QString, QString> &envs = {},
                const std::function<bool()> &canceled = {});

  explicit Cli(const QString &repositoryPath);
  static bool isAvailable();

private:
  static QString gitPath();
  QString mRepositoryPath;
};

} // namespace git

#endif
