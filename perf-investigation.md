# Gittyup-ng: სინელის კვლევა (merge / abort)

თარიღი: 2026-10-13. მანქანა: სამსახურის Windows 11, SentinelOne EDR.

## პრობლემა

რეპო: `C:\cbs-front` (index-ში 27,371 ჩანაწერია, 4.77 MB). ორი ბრენჩის merge-ისას:

| | GitKraken | Gittyup (upstream) | Gittyup-ng |
|---|---|---|---|
| conflict-ების აღმოჩენა (merge) | 13 წმ | 6 წმ | **45–46 წმ** |
| merge abort | მყისიერად | 3 წმ | **18 წმ** |

სინელე QML-ზე მიგრაციისთანავე გაჩნდა, `git::Cli`-ის დანერგვამდე.

## რა გამოირიცხა

### 1. `git::Cli`: მიზეზი არ არის
- `git::Cli`-ს მხოლოდ ორი ადგილი იყენებს: `Index::setStaged` (`src/git/Index.cpp:146`, `:224`) და `Branch::rename` (`src/git/Branch.cpp:104`).
- merge-ის, conflict-ების, status-ის და abort-ის დროს git პროცესი ერთხელაც არ ეშვება.
- ამასთან სინელე Cli-მდეც არსებობდა.

### 2. merge-ის და abort-ის კოდი master-ის იდენტურია
| ფუნქცია | Gittyup-ng | Gittyup-master |
|---|---|---|
| `Repository::merge` | `src/git/Repository.cpp:918-929` | `:899-910` (ბაიტი ბაიტში) |
| `Commit::reset` | `src/git/Commit.cpp:310` | `:274` |
| `RepoView::merge` | `src/ui/RepoView.cpp:1357-1413` | `:1287-1343` |
| `RepoView::mergeAbort` | `src/ui/RepoView.cpp:1415-1480` | `:1345-1410` |
| `checkForConflicts` | `src/ui/RepoView.cpp:3014-3054` | განსხვავება მხოლოდ `mLogView` → `mLogPanel` |

notifier-ის სიგნალები (`referenceUpdated`, `stateChanged` …) git ფენაში ორივეგან ერთნაირად იგზავნება. ანუ დრო merge-ის **შემდეგ** იხარჯება, UI-ის ან მოდელების განახლებაზე.

### 3. Watcher-ის debounce: შემოწმდა, მიზეზი არ არის
- master (`src/watcher/RepositoryWatcher.cpp:30`): `mTimer.start();`. ეს debounce-ია, timer ყოველ ცვლილებაზე თავიდან იწყება.
- ng-ში იყო `if (!mTimer.isActive()) mTimer.start();`. ეს throttle-ია: refresh ყოველ 2 წამში, სანამ ცვლილებები გრძელდება.
- **შევცვალე `notifyChanged() { mTimer.start(); }`-ით და ისევ 46 წამი გამოვიდა.** ცვლილება დავტოვე, რადგან თავისთავად სწორია.
- `RepositoryWatcher_win.cpp:97-100`: ბუფერის გავსებისას (`ERROR_NOTIFY_ENUM_DIR`) ng თავიდან იძახებს `watch()`-ს. ეს **სწორია, არ წაშალო**. master ამ დროს უბრალოდ `return`-ს აკეთებს და watcher სამუდამოდ ჩერდება, ანუ master-ის ბაგია.

## შედეგი
| ცვლილება | merge |
|---|---|
| საწყისი | 46 წმ |
| + `PathFilter`-ის დაბრუნება | 35 წმ |
| + libgit2: `!defined(__MINGW32__)` წაშლილია | **8 წმ** |

## ✅ მთავარი მიზეზი (gdb stack sampling-ით ნაპოვნი): libgit2-ის MinGW build
merge-ის და abort-ის დროს main thread თითქმის მთლიანად ამ ჯაჭვშია:
```
RepoView::merge → git_merge → git_checkout_index → filesystem_iterator_advance_into
  → git_fs_path_diriter_stat → git_fs_path_lstat → GetFileAttributesExW
RepoView::mergeAbort → diffIndexToWorkdir → git_diff_index_to_workdir → (იგივე diriter_stat)
```
- `dep/libgit2/libgit2/src/util/fs_path.h:483` და `src/util/fs_path.c:1201`: `#if defined(GIT_WIN32) && !defined(__MINGW32__)`.
- MSVC-ზე (ოფიციალური Gittyup) libgit2 დირექტორიას `FindFirstFileExW`-ით კითხულობს. ამ ფუნქციას ფაილის ატრიბუტები თავადვე მოაქვს, ამიტომ თითო ფაილზე დამატებითი syscall არ სჭირდება.
- **MinGW-ზე ეს გზა გამორთულია.** იყენებს POSIX-ის `readdir`-ს და **თითო ფაილზე ცალკე `lstat`-ს** (`GetFileAttributesExW`).
- EDR-ის გამო ერთი stat ~0.6 ms ჯდება. 27 ათას ფაილზე ეს ~16 წმ-ია workdir-ის **ყოველ** სრულ სკანირებაზე. merge-ს და abort-ს რამდენიმე ასეთი სკანირება სჭირდება: checkout, diff, status.
- გამოსწორება: ორივე ადგილას წაშალე `&& !defined(__MINGW32__)`. თანამედროვე mingw-w64-ს `FindFirstFileExW` აქვს. libgit2 submodule-ია (Gittyup-ის fork), ამიტომ ცვლილება იქ უნდა დაკომიტდეს ან patch-ად შეინახო.
- Linux-ზე ეს პრობლემა არ არის, იქ POSIX გზა ნორმალურია. ამიტომ სახლში შეიძლება საერთოდ ვერ გაიმეორო.

## დარჩენილი ეჭვმიტანილები (გასაზომია)

### A. ფაილების სიის სრული თავიდან აწყობა (მთავარი ეჭვი)
- `src/ui/DetailView.cpp:163-170`: `indexChanged`-ზე იძახებს `mStagedFiles->refresh()`-ს და `mUnstagedFiles->refresh()`-ს.
- `ChangedFilesModel::refresh` (`ChangedFilesModel.cpp:38`) → `rebuild()` (`:54-147`) აკეთებს `beginResetModel`-ს. ყოველ ფაილზე `accepts()` + `isStaged` 3-მდე ჯერ იძახება (`:44`, `:71`, `:75`), მერე დირექტორიების ხე თავიდან იწყობა.
- `DetailView::updateButtons` (`DetailView.cpp:575-648`) ისევ ყველა ფაილს გადაუვლის.
- reset-ის შემდეგ QML-ის ListView ყველა delegate-ს თავიდან ქმნის.
- master: `DoubleTreeWidget.cpp:276` → `mDiffTreeModel->refresh(paths)`, ანუ მხოლოდ შეცვლილი ფაილები.
- `isStaged` ორივეგან იდენტურია, მაგრამ ფაილს ხსნის (HEAD, CRLF hash, submodule-ის შემოწმება). ამ მანქანაზე ფაილის stat/open ~0.6 ms ჯდება EDR-ის გამო. merge-ის შემდეგ შეცვლილი ფაილები ბევრია, ამიტომ ფასი = (refresh-ების რაოდენობა) × (ფაილების რაოდენობა) × (`isStaged`-ის გამოძახებები).

### B. `isIgnored` საერთო repo-ზე watcher-ის thread-იდან: ✅ დადასტურდა, გასწორებულია
- **`PathFilter`-ის დაბრუნების შემდეგ merge 46 → 35 წმ გახდა.** დარჩენილი ~29 წმ (master-თან, 6 წმ, შედარებით) სხვა მიზეზზე მოდის: A, C ან D.
- ng `src/watcher/RepositoryWatcher_win.cpp:121`: `repo.isIgnored(path)`. იყენებს იმავე `git_repository`-ს, რასაც status thread. libgit2 ერთი repo-ს ორი thread-იდან ერთდროულ გამოყენებას არ უძლებს.
- master-ში `PathFilter`-ია (`src/watcher/PathFilter.cpp`): repo-ს საკუთარ ასლს ხსნის და tracked ფაილებს სწორად ამოწმებს. ng-ში წაშლილია.
- დაბრუნების გზა:
  1. დააკოპირე `PathFilter.h/.cpp` master-იდან `src/watcher/`-ში.
  2. `watcher/CMakeLists.txt:12` → `add_library(watcher PathFilter.cpp RepositoryWatcher.cpp ${WATCHER_IMPL_FILE})`.
  3. `RepositoryWatcher_win.cpp`-ში:
     - `#include "PathFilter.h"`
     - `:32` `mRepo(repo)` → `mFilter(repo)`
     - `:54` `repo()` → `PathFilter &filter() { return mFilter; }`
     - `:112` წაშალე
     - `:121` → `watcher->filter().isRelevant(path)`
     - `:137` `git::Repository mRepo;` → `PathFilter mFilter;`
- ng-ის `git::Index`-ს `isTracked` და `read()` უკვე აქვს.

### C. Build-ის განსხვავება
- "Gittyup 6 წმ" სავარაუდოდ ოფიციალური release-ია: MSVC და Gittyup-ის თავისი libgit2.
- ng: gcc 16.2 (MSYS2 UCRT64), libgit2 dev `3b7ebbad0` (1.9.0, 2026-01-12), `CMAKE_BUILD_TYPE=Release`.
- ზუსტი შედარებისთვის `Gittyup-master` უნდა აიწყოს **იმავე toolchain-ით და იმავე libgit2-ით**. `C:\Gittyup-master` git რეპო არ არის და libgit2 submodule არ აქვს გადმოწერილი.

### D. სხვა ახალი QML მოდელები
`MergeModel.cpp`, `FileViewModel.cpp`, `RefsPanel.cpp`, ძლიერ შეცვლილი `RepoView.cpp`, `UndoHistory::check` (`UndoHistory.cpp:118`). ჯერ არ გაზომილა.

## შემდეგი ნაბიჯები

### 1. ჯერ: რეპროდუცირდება თუ არა Linux-ზე?
EDR სახლში არ არის, ამიტომ ფაილის stat ბევრად იაფია. cbs-front სახლში შეიძლება არ გქონდეს, ასეთ შემთხვევაში ააწყე სინთეზური რეპო: ~25–30 ათასი ფაილი, ორი ბრენჩი, ასობით შეცვლილი ფაილი და რამდენიმე conflict.
- **Linux-ზეც ბევრად ნელია master-თან შედარებით:** პრობლემა UI-ის ან მოდელების კოდშია (A, B ან D) და იქვე გამოიჭირე.
- **Linux-ზე არ ჩანს:** პრობლემა Windows-ზე, EDR-ზე ან build-ზეა დამოკიდებული (A × EDR ან C).

### 2. Profiling (stack sampling) — ყველაზე სწრაფი გზა
**Linux:**
```bash
perf record -g -p $(pidof gittyup-ng) -- sleep 60   # merge-ის დროს
perf report --no-children
```
ან hotspot. ან gdb-ით, ხელით:
```bash
for i in $(seq 30); do gdb -q -batch -p $(pidof gittyup-ng) -ex "thread apply all bt 30"; sleep 1; done > stacks.txt
```

**Windows (სამსახურში):** აპი **არა Administrator** shell-იდან უნდა გაეშვას, თორემ gdb ვერ უერთდება (`error 5: Access is denied`). exe stripped არ არის, ამიტომ gdb-ს ფუნქციების სახელები უჩანს.
```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"
P=$(tasklist //FI "IMAGENAME eq gittyup-ng.exe" //NH //FO CSV | cut -d'"' -f4)
for i in $(seq 40); do gdb -q -batch -nx -p $P -ex "set pagination off" -ex "thread apply all bt 30" > s_$i.txt 2>&1; sleep 1; done
```
შემდეგ ნახე, რომელი ფუნქციები ჩანს ყველაზე ხშირად main thread-ის (Thread 1) stack-ებში.

### 3. ან `QElapsedTimer`-ით გაზომვა
`QElapsedTimer` + `qDebug()` ჩასვი შემდეგ ადგილებში. დაითვალე, **რამდენჯერ** გამოიძახება თითოეული და **რამდენი** დრო მიაქვს:
- `Repository::merge`
- `CommitModel::startStatus` და status thread (`CommitList.cpp:191-218`)
- `ChangedFilesModel::rebuild` (`ChangedFilesModel.cpp:54`)
- `DetailView::updateButtons` (`DetailView.cpp:575`) და `DetailView::setDiff` (`:309`)
- `UndoHistory::check` (`UndoHistory.cpp:118`)
- `RepositoryWatcher`-ის timeout

### 4. A/B
`Gittyup-master` ააწყე იმავე gcc-ით და იმავე libgit2 commit-ით (`3b7ebbad`). ასე build-ის წილი UI-ის წილისგან გამოიყოფა.

## Staging-ის ცალკე პრობლემები (merge-თან კავშირი არ აქვს)
cbs-front-ზე გაზომილი:
- `Index::setStaged` ყოველ კლიკზე ერთ git პროცესს უშვებს, ფაილებზე ცალ-ცალკე არა. მაგრამ:
  - 1 ფაილის `git add`: ~268 ms, `git reset`: ~386 ms. 300 ფაილი: 1.48 / 0.95 წმ.
  - GUI თავისუფლდება მხოლოდ ბოლოს: `Cli.cpp:67` `waitForFinished(-1)`.
  - პროცესების ~2% ~5 წამით იჭედება (EDR).
- `Index.cpp:209`: ზედმეტი `git_index_write`, მეხსიერებაში არაფერი შეცვლილა. მთელ 4.77 MB-იან index-ს თავიდან წერს. გამოსწორება: `Index::setMode`-მა (`Index.cpp:78-86`) თავად ჩაწეროს, ან dirty flag გამოიყენე.
- `Index.cpp:225`: `git_index_read(d->index, true)` → `false`. stamp ისედაც შეიცვალა და მაინც თავიდან წაიკითხავს.
- `Cli.cpp:18`: `findExecutable("git")` პოულობს `cmd\git.exe`-ს. ეს launcher-ია და მეორე პროცესს უშვებს, რაც Windows-ზე ყოველ გამოძახებას ~50 ms-ს უმატებს. გამოსწორება: `mingw64\bin\git.exe`-ის გამოყენება, ან ერთხელ `git --exec-path` და შედეგის cache.
- staging-ის შემდეგ `DetailView` ორივე მოდელს სრულად აწყობს თავიდან (იხ. A).
- სასურველია `setStaged` GUI thread-იდან გავიდეს (QtConcurrent ან async QProcess).

## libgit2 → CLI მიგრაციის სტატუსი
- `Branch::rename`: გადაწერილია `git branch -m`-ით (`src/git/Branch.cpp:99-115`), შემოწმებულია.
- შემდეგი: `Branch::remove` (`git branch -d/-D`) → `Branch::setUpstream` (`--set-upstream-to` / `--unset-upstream`) → `setRebase` (სურვილისამებრ).
- დანარჩენი სია: `Repository::commit`/`amend`, `Commit::amend`/`revert`/`reset`, `createBranch`, `checkout`, `setHead`/`setHeadDetached`, ბოლოს `RefState::restore`.
