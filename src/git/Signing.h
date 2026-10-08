//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef GIT_SIGNING_H
#define GIT_SIGNING_H

#include "git2/commit.h"
#include "git2/types.h"
#include <QByteArray>
#include <QString>

namespace git {

// Commits signed like git signs them when 'commit.gpgsign' is set: with
// gpg, gpgsm or ssh-keygen as 'gpg.format' says, and the key of
// 'user.signingkey'.
namespace Signing {

enum Format { OpenPgp, X509, Ssh };

// Whether commits of 'repo' are signed.
bool isEnabled(git_repository *repo);

// Sign 'content' with the settings of 'repo'. 'committer' is the default
// key of gpg and gpgsm. Returns the signature, or sets 'error'.
QByteArray sign(git_repository *repo, const QByteArray &content,
                const git_signature *committer, QString *error);

// Create a commit like git_commit_create(), signed when commits of 'repo'
// are signed. Sets the error of libgit2 when signing fails.
int createCommit(git_oid *id, git_repository *repo, const char *updateRef,
                 const git_signature *author, const git_signature *committer,
                 const char *message, const git_tree *tree, size_t count,
                 const git_commit *parents[]);

// Point the reference 'name', or the branch it points to, at 'id'.
int updateReference(git_repository *repo, const char *name,
                    const git_oid *id, const char *log);

// Creates the commits of rebases, as git_rebase_options::commit_create_cb
// with the repository as payload.
int createRebaseCommit(git_oid *id, const git_signature *author,
                       const git_signature *committer, const char *encoding,
                       const char *message, const git_tree *tree,
                       size_t count, const git_commit *parents[],
                       void *payload);

// The kind of the signature of a signed commit: "GPG", "SSH" or "X.509",
// or an empty string.
QString signatureKind(git_repository *repo, const git_oid *commit);

} // namespace Signing

} // namespace git

#endif
