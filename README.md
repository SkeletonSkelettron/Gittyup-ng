Gittyup-ng
==================================

Gittyup-ng is a graphical Git client designed to help you understand and manage your source code history.
It continues [Gittyup](https://github.com/Murmele/Gittyup) with a new interface written in QML.
Windows installers are on the [releases page](https://github.com/SkeletonSkelettron/Gittyup-ng/releases);
on other systems, build it from source by following the directions [below](#how-to-build).

The changes that Gittyup-ng makes to Gittyup were made by an AI: Claude, by Anthropic.
See [what Gittyup-ng changes](#what-gittyup-ng-changes).

Gittyup is a continuation of the [GitAhead](https://github.com/gitahead/gitahead) client.

Table of contents
=================
<!--ts-->
   * [What Gittyup-ng Changes](#what-gittyup-ng-changes)
   * [Features](#features)
   * [How to Get Help](#how-to-get-help)
   * [Build Environment](#build-environment)
   * [Dependencies](#dependencies)
   * [How to Build](#how-to-build)
   * [How to Install](#how-to-install)
   * [How to Contribute](#how-to-contribute)
   * [License](#license)
<!--te-->

What Gittyup-ng Changes
---------------
> **Made by AI.** All of the changes below were made by Claude Opus 5.5, an AI model by
> [Anthropic](https://www.anthropic.com), working in [Claude Code](https://claude.com/claude-code)
> at the request of the maintainer of this fork: the code, the tests, the build of the Windows packages
> and this description. The commits say so with a `Co-Authored-By: Claude` line.

Gittyup-ng starts from the master branch of Gittyup of 24 September 2026. On top of it:

### A new interface in QML

* The Qt Widgets interface is replaced by QML, in a style like GitKraken's, with dark and light Kraken themes.
  The main window is a single QML scene: the tool bar, the sidebar with branches, remotes, tags and pull requests,
  repository tabs that are reordered by dragging, the welcome page, the commit graph, the details of commits,
  the diff, the activity log, the menu bar, the context menus and the tool tips.
* Every dialog is in QML: branches, tags, checkout, merge, clone, commit messages and amends, fetch, pull and push,
  submodules, ignore patterns, commit templates, accounts, credentials, settings with the hotkeys,
  repository settings, external tools, plugins, pull requests, about and updates.
* Files are shown and edited in a QML text editor instead of Scintilla, with a find bar, blame in a margin like
  GitKraken's, the wrapping and indentation settings, and spell checking of commit messages.
  Diffs show images before and after a change.
* Syntax highlighting covers more languages: TypeScript, JSON, Dart, Dockerfile, PowerShell, TOML, Gradle,
  Protocol Buffers, Nix, Elixir, Haskell, F#, Julia, R and Zig.

### Features like GitKraken's

* **Merge editor**: conflicts are resolved by taking lines or whole conflicts from either side into an output
  that can be edited. The panes scroll together, the output waits while the conflicts scroll by, each side has
  a color of its own, and the syntax is highlighted.
* **Undo and redo** (Ctrl+Z) of the actions that move HEAD, branches, tags or upstreams, like commits, resets,
  merges and checkouts.
* **Drag and drop**: dropping a branch onto another branch fast-forwards, merges or rebases, and dropping it onto
  a remote pushes it.
* **Interactive rebase**: commits are picked, reworded, squashed, dropped and reordered in place of the graph.
* **Solo and hide branches** in the graph.
* **Command palette** (Ctrl+P) to find commands, branches, tags, files and repositories.
* **Pull requests** of GitHub, GitLab and Gitea in the sidebar.
* **Pull** a branch that isn't checked out from its context menu.
* **Signed commits** with GPG, SSH or X.509 keys, as `commit.gpgsign` and `gpg.format` say.

### Fixes

* SSH tries every key of `~/.ssh/config`, of the settings and the default keys, and only asks for the passphrase
  of a key that has one, naming the key.
* Text that isn't ASCII, like Georgian, is read and saved as UTF-8 on Windows instead of in the code page of
  the system.
* Repositories that git opens also open when libgit2 refuses them because of who owns them, and the warning
  about a repository that doesn't open says why.

### Windows packages

* The Windows installer and zip are cross-built with MinGW on Linux and ship the QML modules they need.
* The installer doesn't start the application when it finishes: it would run as the administrator who installed it,
  who doesn't own the repositories of the user.

### The name

* The application is called Gittyup-ng and has an identifier of its own, so it runs beside Gittyup.
  The first time it runs, it starts with the settings and the data of Gittyup, and leaves them for Gittyup.

### Tests

* New tests load every QML view and cover the merge editor, undo and redo, dragging branches, interactive rebase,
  signing, pulling branches, SSH keys and opening repositories.

Features
---------------
To get an overview of the current features please have a look at the [GitHub Page](https://murmele.github.io/Gittyup/)

How to Get Help
---------------

Ask questions about building or using Gittyup on
[Stack Overflow](http://stackoverflow.com/questions/tagged/gittyup) by
including the `gittyup` tag. Remember to search for existing questions
before creating a new one.

Report bugs in Gittyup by opening an issue in the
[issue tracker](https://github.com/Murmele/gittyup/issues).
Remember to search for existing issues before creating a new one.

If you still need help, check out our Matrix channel
[Gittyup:matrix.org](https://matrix.to/#/#Gittyup:matrix.org).

Build Environment
-----------------

* C++11 compiler
  * Windows - MSVC >= 2017 recommended
  * Linux - GCC >= 6.2 recommended
  * macOS - Xcode >= 10.1 recommended
* CMake >= 3.19
* Ninja (optional)

Dependencies
------------

External dependencies can be satisfied by system libraries or installed
separately. Included dependencies are submodules of this repository. Some
submodules are optional or may also be satisfied by system libraries.

**External Dependencies**

* Qt (required >= 6.4)

**Included Dependencies**

* libgit2 (required)
* cmark (required)
* git (only needed for the credential helpers)
* libssh2 (needed by `libgit2` for SSH support)
* openssl (needed by `libssh2` and `libgit2` on some platforms)

Note that building `OpenSSL` on Windows requires `Perl` and `NASM`.

How to Build
------------

**Initialize Submodules**

    git submodule init
    git submodule update --depth 1

**Build OpenSSL**

    # Start from root of gittyup repo.
    cd dep/openssl/openssl

Windows:

    perl Configure VC-WIN64A
    nmake

macOS (Intel):

    ./Configure darwin64-x86_64-cc no-shared
    make
    
macOS (Apple Silicon)

    ./Configure darwin64-arm64-cc no-shared
    make
    
Linux:

    ./config -fPIC
    make

**Configure Build**

    # Start from root of gittyup repo.
    mkdir -p build/release
    cd build/release
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..

If you have Qt installed in a non-standard location, you may have to
specify the path to Qt by passing `-DCMAKE_PREFIX_PATH=<path-to-qt>`
where `<path-to-qt>` points to the Qt install directory that contains
`bin`, `lib`, etc.

**Build**
```
    ninja
```
    
### A Convenient Shell Script for Ubuntu is available [here](https://raw.githubusercontent.com/Murmele/Gittyup/master/pack/buildUbuntu.sh), and will install all the necessary prerequisites, and build a release version for immediate use.

How to Install
-----------------
### Windows

Download the installer or the zip from the [releases page](https://github.com/SkeletonSkelettron/Gittyup-ng/releases).
Start Gittyup-ng from the Start menu after installing it.

### Linux and macOS

Build Gittyup-ng from source by following the directions [above](#how-to-build).
The packages on Flathub, the AUR and Homebrew install Gittyup, not Gittyup-ng.

How to Contribute
-----------------

We welcome contributions of all kinds, including bug fixes, new features,
documentation and translations. By contributing, you agree to release
your contributions under the terms of the license.

Contribute by following the typical
[GitHub workflow](https://docs.github.com/en/get-started/quickstart/github-flow)
for pull requests. Fork the repository and make changes on a new named
branch. Create pull requests against the `master` branch. Follow the
[seven guidelines](https://chris.beams.io/posts/git-commit/) to writing a
great commit message.

Prior to committing a change, please use `cl-fmt.sh` to ensure your code
adheres to the formatting conventions for this project. You can also use the
`setup-env.sh` script to install a pre-commit hook which will automatically
run `clang-format` against all modified files.

Prior to pushing a change, please ensure you run the unit tests to avoid any
regressions. These are run using `ctest` in `<build-dir>`.

License
-------

Gittyup-ng and its predecessors Gittyup and GitAhead are licensed under the MIT license. See LICENSE.md for details.
