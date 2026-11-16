//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REMOTE_H
#define REMOTE_H

#include "Repository.h"
#include "Result.h"
#include <QSet>
#include <QStringList>
#include <QString>
#include <QSharedPointer>

struct git_remote;

namespace git {

class Id;
class Reference;

class Remote {
  Q_DECLARE_TR_FUNCTIONS(Remote)

public:
  class Callbacks {
  public:
    enum State { Transfer, Resolve, Update };

    Callbacks(const QString &url, const Repository &repo = Repository())
        : mUrl(url), mRepo(repo) {}

    QString url() const { return mUrl; }

    Repository repo() const { return mRepo; }

    virtual void sideband(const QString &text) {}

    virtual bool transfer(int total, int current, size_t bytes) { return true; }

    // git stops the transfers that are canceled, see Cli::run().
    virtual bool isCanceled() const { return false; }

    virtual void update(const QString &name, const Id &a, const Id &b) {}

    virtual void rejected(const QString &name, const QString &status) {}

  protected:
    QString mUrl;
    Repository mRepo;
    State mState = Transfer;
  };

  Remote();

  bool isValid() const { return !d.isNull(); }
  explicit operator bool() const { return isValid(); }

  QString name() const;
  void setName(const QString &name);

  QString url() const;
  void setUrl(const QString &url);

  Result fetch(Callbacks *callbacks, bool tags = false, bool prune = false);
  Result push(Callbacks *callbacks, const QStringList &refspecs);
  Result push(Callbacks *callbacks, const Reference &src,
              const QString &dst = QString(), bool force = false,
              bool tags = false);

  static Result clone(Callbacks *callbacks, const QString &url,
                      const QString &path, bool bare = false);

private:
  Remote(git_remote *remote);

  QSharedPointer<git_remote> d;

  friend class Branch;
  friend class Repository;
};

} // namespace git

Q_DECLARE_METATYPE(git::Remote);

#endif
