### v0.0.7 - 2026-10-30

#### Added

* Take Ours and Take Theirs in the context menu of conflicted files, after confirming it. A file that the chosen side deleted is deleted

#### Changed

* Merging runs git instead of libgit2, and merging and aborting a merge run in the background without freezing the window
* The references, the search index and the pull requests are read in the background when a repository opens

#### Fixed

* Double-clicking a branch, or dropping a branch onto another one and checking it out, could abort Gittyup-ng
* Listing the changed files after a big merge or checkout took minutes
* Aborting a merge failed when a file was changed on one side and deleted on the other
* Files that one side deleted or both sides added didn't show as conflicts
* Checking out files of a commit overwrote untracked files, staged changes and files in the way. It refuses now and lists them
* Staging files dropped what other git programs had staged since
* Cherry-picking and reverting committed staged changes with the commit, and canceling deleted them. They refuse staged changes now
* Merging before the current merge was concluded committed it with the wrong parent
* Amending a commit after the branch had moved on dropped the newer commits, and a signed commit could drop a commit that another program made while it was signed
* Fast-forwards, checkouts and undo moved or checked out branches that another worktree had checked out
* Undo deleted tags fetched since and reset upstreams changed since, couldn't undo renaming a branch into a folder of its old name or the first commit, and left the files changed when it failed
* Pushing to another remote than the one of the upstream pushed to the name of the upstream branch
* Push said "Everything up-to-date." without pushing when the remote-tracking branch was out of date, and a push that the remote rejected set the upstream
* Canceling a fetch froze the window until git finished
* Rebases ignored the committer that the commit settings override
* Deinitializing a submodule could delete the commits that only its repository had, and the settings of other submodules
* Staging lines of a new file kept CRLF line endings where the attributes ask for LF
* Rewording an empty commit dropped it
* Filters that fail although they aren't required stopped the operation, and filters ignored the commands of the repository's settings

----

### v0.0.6 - 2026-10-29

#### Added

* The Neon theme: neon pink, cyan and green on a deep night blue

#### Changed

* Staging single lines, writing the configuration, initializing and updating submodules and cherry-picking run git instead of libgit2

#### Fixed

* Closing a repository while a push, a fetch or an update of submodules ran crashed Gittyup-ng

----

### v0.0.5 - 2026-10-28

#### Changed

* Pushing a branch that tracks no remote branch yet makes it track the one it pushes to, like git push -u, instead of failing with "has no upstream branch"
* Fast-forwarding branches, moving a branch after an interactive rebase and Set Executable run git instead of libgit2

#### Fixed

* The New Branch, Checkout, Merge and Remote dialogs took up to 20 seconds to open in repositories with thousands of branches
* Branches that other programs checked out, committed to or deleted showed up only after refreshing. They show up when the window becomes active, and right away on Windows
* Failed checkouts showed an unrelated error, like "config value 'branch.x.merge' was not found". They say that local changes would be overwritten, or what git reports

----

### v0.0.4 - 2026-10-23

#### Added

* Delete remote branches from their context menu, after confirming it
* Filters in the titles of the Branch / Tag, Commit message, Author and SHA columns of the commit graph. The commits that don't match fade out instead of being hidden, the first match is selected, and Enter and Shift+Enter go to the next and the previous one
* Undo, Redo, Cut, Copy, Paste, Delete and Select All in the context menu of every text field, in the colors of the theme
* A spinner over the repository while checkouts, new branches, stashes, resets, reverts and discarding run

#### Changed

* The Kraken themes are called Teal

#### Fixed

* The window froze while checkouts, new branches, stashes, resets, reverts and discarding ran, and clicks meanwhile ran them again when they ended
* The references didn't update after deleting the upstream branch of a deleted branch
* The changelog of the About dialog was empty when Gittyup-ng ran from its build

----

### v0.0.3 - 2026-10-22

Gittyup-ng now runs git for the operations that change a repository, so git has to be installed: Git for Windows on Windows.

#### Added

* OneNord, Palenight and GitHub (Light Default, Dark Default and Dark Dimmed) themes
* Select the lines of a diff by dragging over them, and copy them with Ctrl+C
* Copy Branch Name and Copy Tag Name in the context menu of references
* Pull or force push when a push is rejected because the remote has new commits
* Failed operations open the log at the error, show the reason in a red banner and count the errors on the log button
* A splash screen while the windows load

#### Changed

* Staging, checking out, resetting, reverting, stashes, branches, tags, remotes, fetch and push run git instead of libgit2, and the errors of git are shown in the log
* git status lists the changed files of the workdir, which is faster in large repositories on Windows
* Merge conflicts are written in the diff3 style unless merge.conflictStyle is set, so they stay whole

#### Fixed

* Merging and aborting merges were slow in large repositories on Windows
* Discard File and the discard button of the file list did nothing for untracked files. They remove them after asking
* Actions in submenus of context menus, like Reset, ran twice
* The entries of context menus were hovered only after a click, and no longer while a submenu was open

----

### v0.0.2 - 2026-10-06

Bug Fix release

#### Fixed

* The check box of a conflict on the side that has no lines takes that side, and the conflict resolves without its lines. Before, clicking it did nothing.

----

### v0.0.1 - 2026-10-05

The first release of Gittyup-ng, which continues Gittyup 2.0.0 with a new interface. The releases below it are the ones of Gittyup.

#### Added

* A new interface in QML, in a style like GitKraken's, with dark and light Kraken themes
* Resolve conflicts in a merge editor, whose panes scroll together
* Undo and redo actions, like commits, resets, merges and checkouts
* Drag branches onto branches to fast-forward, merge or rebase, and onto remotes to push
* Rebase interactively, solo and hide branches, and find anything with Ctrl+P
* List the open pull requests of GitHub, GitLab and Gitea in the sidebar
* Pull branches that aren't checked out from their context menu
* Sign commits with GPG, SSH or X.509 keys
* Highlight TypeScript, JSON and more languages
* Add more information about the credential stores to give the user the possibility to easily see the tradeoffs of every credential store

#### Changed

* Gittyup-ng runs beside Gittyup and starts with its settings
* Windows packages are built with MinGW
* Autohide sidebar when opening repository

#### Fixed

* SSH tries every key and only asks for the passphrases of keys that have one
* Read and save UTF-8 text on Windows
* Open repositories that git opens, and say why others don't open
* Don't make empty commits when Commit is clicked again right after a commit
* Notice the changes of other programs on Windows and Linux
* Mark the side of a conflict that has no lines in the merge editor

----

### v2.0.0 - 2025-11-30

Bug Fix and Feature release

#### Added

* Add commit filter to show only the first parent in the commit list view
* Add backend support for SHA256 repos
* Fix email filter being not returning any results
* Add mmap based reading to speed up indexer

#### Changed

* Use Qt6
* Update libgit2 library

----

### v1.4.0 - 2024-04-24

Bug Fix and Feature release

#### Added

* UI(Commit List): Added a right-click menu entry to rename branches.
* UI(Main Menu): Added a menu-entry to rename the current branch.
* UI(Diff View): Added line wrapping option in Tools - Options - Diff
* Option to open Gittyup maximized at startup
* Hotkeys for Navigating CommitList
* Referring repos with their git-dir instead of their work-tree.
* Use columns for file name, directory, and state when files are shown as a list in TreeViews.
* Possibility to change language from the settings
* Linux: Publishing Appimage bundle
* Hide Untracked Files option to DoubleTreeWidget
* Add display options to the commit list

#### Changed

* UI(Commit List): Collapse multiple branch and tag right-click menu entries
                   into submenus. This affects the checkout and delete operations.
* Fix(Build System): Force usage of clang-format v13 to ensure consistent formatting.
* Fix: Commit highlighting
* Compact commit list: move commit message closer to commit graph
* Updating translations
* Close the branch drop-down list after the user checks out a branch.
* Fix: Delete timeline causes crash on windows
* Fix: Rebase: Use original commit message as default message
* Fix: Stage all button

----

### v1.3.0 - 2023-04-20

Performance Improvement and feature release

#### Added

* Colorized status badges
* Template: use first template as default template for the commit message
* Search function for the treeview
* Reworked credential store: add possibility to choose between different methods to store credentials

#### Changed

* Fix external diff in Flatpak build
* Fix windows credentials
* Fix force push to correct remote
* Fix tab title if more than three times a repository with the same name is opened
* Fix storing repository settings correctly, because otherwise they are not applied
* Fix language support
* Improved refresh velocity
* Fix storing and restoring current opened file when Gittyup refreshes
* Improved velocity for files with many hunks

----

### v1.2.2 - 2023-01-22

Bug fix release

#### Changed

* Fix flatpak install process

----

### v1.2.1 - 2023-01-22

Bug fix release

#### Added
* Possibility to hide avatar (Settings - Window - View - Show Avatars)
* Show log entry when a conflict during rebase happens

#### Changed

* Fix download url for flatpak and macos
* Fix Segmentation fault when ignoring files
* Fix discard of complete files and submodules
* Fix context menu entries
* Fix bytesize overflow
* Fix focus loose during scrolling in the Commitlist with the keyboard
* Do not crash when the repository is for some reason broken
* Fix crash if rebasing is not possible

----

### v1.2.0 - 2022-10-28

Bug fix and feature release

#### Added
* Add support for solving merge conflicts for whole files
* Solving binary conflicts directly in Gittyup
* Support rebasing with conflict solving
* Implement amending commits
* Possibility to init submodules after clone (Settings - General - Update submodules after pull and clone)
* Hiding menu bar (Application Settings - Window - Hide Menubar)
* Implement support for Gitea instances

#### Changed
* Fix Segmentation fault when using space to stage files
* Fix menubar color in dark theme
* Filter only branches, tags, remotes attached to selected commit
* Fix crash when global GIT config is invalid
* Fix crash when having errors while adding a remote account
* Fix updater on windows, macos and linux (flatpak)
* Fix discarding file leading to discarding submodule changes
* Fix rebase log messages during rebase
* Improve SSH config handling
* Application settings and repository settings can now be selected with a single settings button
* Use the full file context menu for the staging file list
* Fix Arch Linux build

----

### v1.1.2 - 2022-08-12

Bug fix release

#### Changed

* Fix bundled OpenSSL version incompatibility

----

### v1.1.1 - 2022-06-09

Bug fix release

#### Added
* Distinguish between commit author and committer
* Show image preview also for deleted files
* Official macOS release
* Show which kind of merge conflict occurred for each conflict

#### Changed
* Fix single line staging if not all hunks are loaded
* Fix cherrypick commit author
* Fix segmentation fault if submodule update fails
* Fix line staging with windows new lines
* Show first change in the diff view when loading
* Improved windows icon

----

### v1.1.0 - 2022-04-30

Second release of Gittyup

#### Added
* Button to directly access the terminal and the filebrowser
* Add support for running in single instance mode
* Customizable hotkeys
* Quick commit author overriding
* keyboard-interactive SSH auth
* Improved single line staging and replacing staging image to a more appropriate one
* Font customizing
* Options to switch between staging/unstaging treeview, single tree view and list view
* Do not automatically abort rebase if conflicts occur
* Add possibility to save file of any version on local system
* Add possibility to open a file of any version with default editor

----

### v1.0.0 - 2021-11-18

First version of the GitAhead Fork Gittyup

#### Added
* Staging of single lines
* Double tree view: Seeing staged and unstaged changes in different trees.
* Maximize History or Diff view by pressing Ctrl+M
* Ignore Pattern: Ability to ignore all files defined by a pattern instead of only one file
* Tag Viewer: When creating a new tag all available tags are visible. Makes it easier to create consistent tags.
* Commit Message template: Making it easier to write template based commit messages.

----
