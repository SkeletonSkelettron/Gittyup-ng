# Git აუდიტის N01–N12 შესწორებები

**თარიღი:** 2026-10-30, Asia/Tbilisi. **საფუძველი:** commit `73fdd1d6a418ffc69f70d7f0918617b4925cec39`.
**ანგარიში, რომელსაც ეს პასუხობს:** [git-audit-2026-10-30-full](../git-audit-2026-10-30-full/README.md).

## შედეგი

**12-ვე პუნქტი დახურულია:** თითოეული კოდშია გასწორებული და regression ტესტით დადასტურებული. ყოველი ახალი ტესტი `73fdd1d6`-ის წყაროებზე ვარდება — 12 ჩავარდნა, თითო ტესტზე ერთი ([before-fix.log](evidence/before-fix.log)). შესწორებულ წყაროებზე იგივე ტესტები გადის ([after-fix.log](evidence/after-fix.log)). იმავე ტესტ-პროგრამების ძველი ტესტები ორივე შემთხვევაში გადის.

**სრული ნაკრები: 44/44, `ctest`-ის exit code 0** ([ctest.log](evidence/ctest.log)). 43 ძველი ტესტია და ერთი ახალი — `transfer`. `ui_screenshot` გამოტოვებულია, როგორც ლოკალური helper.

ცვლილებები **10 commit-ადაა დაკომიტებული**, თითო პუნქტზე ერთი ([Commit-ები](#commit-ები)). N01/N09 და N02/N03 თითო commit-ს იზიარებენ, რადგან ერთი ცვლილებით სწორდებიან. ახალი commit-ები `73fdd1d6`-ის თავზეა, არსებული ისტორია არ შეცვლილა; push არ გაკეთებულა. თითოეულ commit-მდე სრული ნაკრები გაეშვა და `ctest`-ის exit code 0 იყო. შესწორებამდე tracked სამუშაო ხე სუფთა იყო. აუდიტების ძველი საქაღალდეები, მათი probes და მტკიცებულებები ხელუხლებელია.

`run_probes.py` `73fdd1d6`-ის snapshot-ზეა მიბმული და შეცვლილ ხეზე ჩერდება. ამიტომ სცენარები ახალ კოდზე ტესტებით გადამოწმდა: ტესტები probe-ების პირობებს იმეორებენ — embedded submodule, stale amend, signer wrapper, `HEAD.lock`, pre-receive hook, ნელი upload-pack, CRLF attributes, empty commit, repo-level filter.

| ID | P | სტატუსი | ძირითადი ცვლილება | Regression ტესტი | შესწორებამდე |
| --- | --- | --- | --- | --- | --- |
| N01 | P1 | დახურულია | `Submodule::deinitialize()` → `git submodule deinit --force` | `TestSubmodule::deinitializeKeepsHistory` | ლოკალური commit-ის ობიექტი `.git/modules/sm`-ში არ არის (exit 128) |
| N02 | P1 | დახურულია | amend HEAD-ს მხოლოდ მაშინ ცვლის, თუ ის ისევ amend-ის commit-ზეა | `TestAmend::testAmendStale` | stale amend-მა ახალი commit ისტორიიდან ამოაგდო |
| N03 | P1 | დახურულია | signed commit-ის ref compare-and-swap-ით იცვლება | `TestSigning::concurrentUpdate` | signer-ის დროს გადაადგილებული ref გადაიწერა |
| N04 | P2 | დახურულია | ref-ების აღდგენის ჩავარდნისას undo ფაილებსა და index-ს აბრუნებს | `TestUndo::lockedHead` | `HEAD.lock`-ის დროს `a.txt` წაიშალა, HEAD კი არ შეიცვალა |
| N05 | P2 | დახურულია | push-ს cached tracking ref აღარ გამოტოვებს | `TestTransfer::pushStaleTracking` | „Everything up-to-date.“, remote branch არ აღდგა |
| N06 | P2 | დახურულია | უარყოფილი ref-ის შემთხვევაში push შეცდომაა | `TestTransfer::pushRejected` | უარის მიუხედავად upstream დაინიშნა |
| N07 | P2 | დახურულია | Cancel git-ის პროცესს აჩერებს; GUI აღარ ელოდება | `TestTransfer::fetchCanceled` | Cancel ~5 წამს ელოდა (5048 ms) |
| N08 | P2 | დახურულია | პირველი commit-ის undo branch-ს unborn ხდის | `TestUndo::firstCommit` | `undo()` = false |
| N09 | P2 | დახურულია | იგივე, რაც N01: git ზუსტ section-ს შლის | `TestSubmodule::deinitializeOnlyItself` | `a.b`-ის deinitialize-მა `axb`-ის URL წაშალა |
| N10 | P2 | დახურულია | untracked text-ის diff git-ის შენახულ ფორმაშია | `TestIndex::stageLinesOfNewFile` | index-ში `first\r\n` |
| N11 | P2 | დახურულია | rewrite საწყისად ცარიელ commit-ს ინარჩუნებს | `TestInteractiveRebase::rewordEmpty` | reworded commit გაქრა, HEAD = base |
| N12 | P2 | დახურულია | filter-ის ბრძანებები repository-ის კონფიგურაციიდან იკითხება | `TestFilter::repositoryOverride` | local override-ის ნაცვლად `global` |

## პუნქტები

### N01 — deinitialize embedded `.git`-ს და ისტორიას შლიდა [P1] — დახურულია

**მიზეზი:** `Submodule::deinitialize()` submodule-ის directory-ს `removeRecursively()`-ით შლიდა. ძველი ფორმატის submodule-ში `sm/.git` ნამდვილი repository-ა, ამიტომ მასთან ერთად იშლებოდა origin-ზე არარსებული commit-ებიც.

**ცვლილება:** `src/git/Submodule.cpp`, `Submodule.h`. Deinitialize ახლა `git --literal-pathspecs submodule deinit --force -- <path>`-ით სრულდება. git embedded repository-ს ჯერ `.git/modules/<name>`-ში გადაიტანს, შემდეგ შლის working directory-ს და ამ submodule-ის კონფიგურაციას. `--force` ConfigDialog-ის არსებულ დადასტურებას შეესაბამება — დიალოგი უკვე აფრთხილებს, რომ uncommitted ცვლილებები დაიკარგება. git-ის შეცდომა log-ში `gitCommandFailed`-ით ჩანს.

**ტესტი:** `TestSubmodule::deinitializeKeepsHistory`. Clean submodule-ს embedded `.git` და origin-ზე არარსებული commit აქვს. deinitialize-ის შემდეგ URL config წაშლილია, directory ცარიელია, commit კი `git --git-dir=.git/modules/sm cat-file -e`-ით არსებობს. შესწორებამდე: `exitCode` 128. შემდეგ: გადის.

### N02 — stale amend ახალ ისტორიას გადაწერდა [P1] — დახურულია

**მიზეზი:** `Commit::amend` replacement commit-ს ქმნიდა და HEAD-ს უპირობოდ ცვლიდა. AmendDialog არამოდალურია, ამიტომ HEAD მანამდე შეიძლება წინ წასულიყო.

**ცვლილება:** `src/git/Commit.cpp`, `src/git/Signing.cpp/.h`. `Signing::updateReference()` ახლა იღებს მოსალოდნელ ძველ OID-ს და ref-ს `git_reference_create_matching`-ით ცვლის — შედარება lock-ის ქვეშ, ატომურად ხდება. Amend მოელის თავისივე commit-ს. თუ HEAD სხვაგანაა, amend უარყოფილია შეტყობინებით „'main' changed while the commit was made.“ და ისტორია უცვლელი რჩება.

**ტესტი:** `TestAmend::testAmendStale`. გაიხსნება `RepoView::promptToAmend`, შემდეგ კეთდება ახალი commit და მერე `AmendDialog::accept()`. HEAD ისევ ახალ commit-ზეა, მისი parent კი ძველი. API-ის `Repository::amend`-იც უარს ამბობს. შესწორებამდე: HEAD შეიცვალა. შემდეგ: გადის.

### N03 — signing-ის დროს გარე ref ცვლილება იკარგებოდა [P1] — დახურულია

**მიზეზი:** signed გზაზე `createCommit` ხელმოწერის შემდეგ ref-ს `updateReference`-ით უპირობოდ ცვლიდა. unsigned გზა — libgit2-ის `git_commit_create` — პირველ parent-ს ამოწმებს.

**ცვლილება:** იგივე `updateReference`. `createCommit` მოელის პირველ parent-ს; პირველი commit-ისთვის ref არ უნდა არსებობდეს, ამიტომ force-ის გარეშე იქმნება. symbolic HEAD ერთ დონეზე იხსნება, როგორც ადრე.

**ტესტი:** `TestSigning::concurrentUpdate`. გამოიყენება ნამდვილი ed25519 key და `gpg.ssh.program` wrapper, რომელიც HEAD-ს სხვა commit-ზე გადაიტანს და მერე ნამდვილ `ssh-keygen`-ს იძახებს. commit უარყოფილია და HEAD გარე commit-ზე რჩება. შესწორებამდე: commit წარმატებული იყო და გარე commit დაიკარგა. შემდეგ: გადის. Windows-ზე გამოტოვებულია, რადგან signer shell script-ია.

### N04 — უარყოფილი undo partial checkout-ს ტოვებდა [P2] — დახურულია

**მიზეზი:** `UndoHistory::apply` ჯერ checkout-ს აკეთებდა, შემდეგ refs-ს. `RefState`-ის rollback მხოლოდ refs-ს აბრუნებდა.

**ცვლილება:** `src/git/Repository.cpp/.h`, `src/ui/UndoHistory.cpp`. `Repository::checkout`-ს დაემატა არასავალდებულო `base`: commit, რომლის tree-ც index-სა და ფაილებშია, ნაგულისხმევად HEAD (`read-tree -m <base> <commit>`). თუ refs-ის აღდგენა ჩავარდა, undo თავდაპირველ commit-ს ახლად ჩაწერილი commit-ის თავზე აბრუნებს. ადგილობრივი ცვლილებები შენარჩუნებულია.

**ტესტი:** `TestUndo::lockedHead`. Checkout-ის undo `HEAD.lock`-ის დროს: undo = false, HEAD, `a.txt` და index უცვლელია, status სუფთაა. lock-ის მოხსნის შემდეგ undo წარმატებულია. შესწორებამდე: `a.txt` წაშლილი, HEAD ძველი. შემდეგ: გადის.

### N05 — stale tracking ref push-ს ჩუმად გამოტოვებდა [P2] — დახურულია

**მიზეზი:** `RepoView::push` local ref-ს cached remote-tracking ref-ს ადარებდა და ტოლობისას git-ს არ უშვებდა.

**ცვლილება:** `src/ui/RepoView.cpp`. shortcut წაშლილია. Push ყოველთვის remote-ს ეკითხება. როცა git „up to date“-ს იტყვის, success გზა იგივე „Everything up-to-date.“-ს წერს.

**ტესტი:** `TestTransfer::pushStaleTracking`. push -u-ის შემდეგ remote branch სხვა პროცესი შლის, შემდეგ push იწყება UI-დან. Remote branch თავიდან იქმნება იმავე commit-ზე, log-ში კი „Everything up-to-date.“ არ წერია. შესწორებამდე: „Everything up-to-date.“ ეწერა. შემდეგ: გადის.

### N06 — push-ის უარყოფის შემდეგ upstream ინიშნებოდა [P2] — დახურულია

**მიზეზი:** `Remote::push` non-fast-forward-ის გარდა სხვა უარყოფას (`[remote rejected]`, hook) წარმატებულ შედეგად აბრუნებდა. UI-ს success გზა კი upstream-ს ნიშნავდა.

**ცვლილება:** `src/git/Remote.cpp`. უარყოფილი ref-ის შემთხვევაში push შეცდომაა („The remote rejected the push.“). უარყოფილი refs ისევ log-ში ჩანს.

**ტესტი:** `TestTransfer::pushRejected`. bare remote-ის pre-receive hook უარს ამბობს. remote-ის OID უცვლელია, log-ში `[remote rejected]` წერია, upstream არ დაინიშნა. შესწორებამდე: upstream დაინიშნა. შემდეგ: გადის. Windows-ზე გამოტოვებულია, რადგან hook shell script-ია.

### N07 — fetch-ის Cancel CLI პროცესს არ აჩერებდა [P2] — დახურულია

**მიზეზი:** CLI fetch-ის შეწყვეტა შეუძლებელი იყო. `cancelRemoteTransfer()` GUI thread-ზე პროცესის დასრულებას ელოდა, log კი „Fetch canceled.“-ს წერდა მაშინაც, როცა fetch სრულად შესრულდა.

**ცვლილება:**
- **`Cli::run`** (`src/git/Cli.cpp/.h`, `RunResult.h`): არასავალდებულო `canceled` ფუნქცია. ლოდინისას ყოველ 50 ms-ში ამოწმებს და git-ს აჩერებს: ჯერ terminate, რომ git-მა თავისი lock-ები წაშალოს, 2 წამის შემდეგ kill. ავსებს `RunResult::canceled`-ს.
- **`Remote::fetch`:** იყენებს ამ ფუნქციას, ხოლო `Remote::Callbacks`-ს დაემატა `isCanceled()`. `RemoteCallbacks`-ის flag `std::atomic`-ია.
- **`RepoView`:** Cancel ღილაკი აღარ ელოდება; tab-ის დახურვა ისევ ელოდება. fetch/push-ის log „canceled“-ს მხოლოდ მაშინ წერს, როცა ოპერაცია მართლა არ დასრულებულა. pull merge-ს შეწყვეტილი fetch-ის შემდეგ არ იწყებს.

**ტესტი:** `TestTransfer::fetchCanceled`. Upload-pack helper 5 წამს ჩერდება, Cancel კი მის დაწყებისთანავე. მოლოდინები:
- ოპერაცია 3 წამში მთავრდება;
- log-ში წერია „Fetch canceled.“;
- tracking ref უცვლელია;
- შემდეგი ჩვეულებრივი fetch მუშაობს, ანუ lock-ები არ დარჩა.

შესწორებამდე: 5048 ms. შემდეგ: გადის. Windows-ზე გამოტოვებულია, რადგან helper shell script-ია.

### N08 — პირველი commit-ის undo ვერ სრულდებოდა [P2] — დახურულია

**მიზეზი:** unborn მდგომარეობის აღდგენისას `git_branch_delete` HEAD-ის branch-ის წაშლაზე უარს ამბობს.

**ცვლილება:** `src/git/RefState.cpp`. თუ HEAD-ის branch-ი იშლება და სამიზნე მდგომარეობაში HEAD იმავე, ახლა unborn branch-ზეა, ref `git_reference_delete`-ით იშლება. index უცვლელი რჩება, ამიტომ commit-ის ფაილები staged რჩება.

**ტესტი:** `TestUndo::firstCommit` სრულიად ცარიელი repository-ით. undo-ს შემდეგ:
- HEAD unborn-ია;
- `git status` = `A  first.txt`;
- commit-ის შეტყობინება ისევ შეთავაზებულია.

redo commit-ს აბრუნებს. შესწორებამდე: `undo()` = false. შემდეგ: გადის.

### N09 — submodule-ის regex სხვა submodule-ებსაც ემთხვეოდა [P2] — დახურულია

**მიზეზი:** `submodule\.<name>\..*` regex escaping-ის გარეშე იგებოდა.

**ცვლილება:** N01-ის ცვლილება. `git submodule deinit` მხოლოდ `submodule.<name>` section-ს შლის, regex აღარ არის.

**ტესტი:** `TestSubmodule::deinitializeOnlyItself`. `a.b`-ის deinitialize-ის შემდეგ `submodule.a.b.url` წაშლილია, `submodule.axb.url` და `axb`-ის ფაილები კი დარჩა. შესწორებამდე: `axb`-ის URL წაიშალა. შემდეგ: გადის.

### N10 — ნაწილობრივი staging CRLF ნორმალიზაციას არღვევდა [P2] — დახურულია

**მიზეზი:** tracked ფაილების diff-ში libgit2 სამუშაო ფაილს clean ფორმით იღებს. untracked ფაილს კი `DiffModel` raw bytes-ით კითხულობდა, და მონიშნული ხაზები index-ში CRLF-ით ხვდებოდა.

**ცვლილება:** `src/git/Repository.cpp/.h` — ახალი `Repository::cleanContent()`, libgit2-ის `GIT_FILTER_TO_ODB` filters, ანუ eol/attributes და clean drivers. `src/ui/DiffModel.cpp` მას untracked text-ისთვის იყენებს. ნორმალიზაცია ერთხელ, diff-ის აგებისას ხდება; `Index::add` უცვლელია, ამიტომ tracked buffers ორჯერ აღარ იფილტრება. untracked ფაილის discard მთლიან ფაილს შლის და ამ content-ს არ იყენებს.

**ტესტი:** `TestIndex::stageLinesOfNewFile`. `*.txt text eol=lf`-ით ახალ `first\r\nsecond\r\n`-ში `DiffModel::toggleLine` პირველ ხაზს მონიშნავს და index-ში `first\n` ხვდება. შესწორებამდე: `first\r\n`. შემდეგ: გადის.

### N11 — empty commit-ის Reword მას შლიდა [P2] — დახურულია

**მიზეზი:** `Rewrite::apply` non-Squash ნაბიჯს აგდებდა, თუ tree წინა commit-ისას ემთხვეოდა. ეს განზრახ ცარიელ commit-საც ეხებოდა.

**ცვლილება:** `src/git/Rewrite.cpp`. ნაბიჯი, რომლის commit-იც თავიდანვე ცარიელია, ანუ tree მისი parent-ისას ემთხვევა, აღარ იგდება — `git rebase` ასე ინარჩუნებს „start empty“ commit-ებს. replay-ის დროს ცარიელი გამხდარი commit-ები ისევ იგდება.

**ტესტი:** `TestInteractiveRebase::rewordEmpty` — InteractiveRebase-ის მოდელით, როგორც UI-ში. ცარიელი commit-ის Reword-ის შემდეგ HEAD ერთი commit-ია ახალი შეტყობინებით, base კი მისი ერთადერთი parent. შესწორებამდე: HEAD = base („Add x.txt“). შემდეგ: გადის.

### N12 — repo-level filter override არ გამოიყენებოდა [P2] — დახურულია

**მიზეზი:** `Filter::init()` startup-ზე გლობალურ clean/smudge/required-ს ინახავდა, ხოლო stream ამ cached მნიშვნელობებს იყენებდა.

**ცვლილება:** `src/git/Filter.cpp`.
- Stream ახლა stream-ის repository-ის ეფექტურ კონფიგურაციას კითხულობს (`git_repository_config_snapshot`), სადაც local გლობალურს ფარავს: `filter.<name>.clean`, `smudge` და `required`.
- `required` თითო stream-ზე ინახება.
- ცარიელი ბრძანებისას content უცვლელად გადის.

ამით libgit2-ის dirty-შემოწმება — მაგალითად rebase-ის წინ — სწორ clean ბრძანებას იყენებს.

**ტესტი:** `TestFilter::repositoryOverride`. ორ repository-ში ერთი და იმავე filter-ს განსხვავებული ბრძანებები აქვს:
- smudge `token`-ს override-იან repository-ში `local`-ად აქცევს, მეორეში — `global`-ად;
- clean ორივეს უკან `token`-ად აქცევს;
- `filter.<name>.required = true` მხოლოდ ერთ repository-ში აქცევს optional filter-ის ჩავარდნას შეცდომად.

შესწორებამდე: `global`. შემდეგ: გადის.

## შენიშვნები და საზღვრები

- **N02/N03:** შემოწმება მოსალოდნელ OID-ზეა. თუ HEAD სხვა branch-ზე გადავიდა, რომელიც იმავე commit-ზეა, commit/amend მიმდინარე branch-ზე ხვდება — ისტორია ამით არ იკარგება. intended ref-ის სახელი ცალკე არ ინახება.
- **N04:** ფაილები მხოლოდ checkout-ის რეჟიმის undo/redo-ში ბრუნდება, როცა თავდაპირველი commit არსებობს. თუ ფაილების დაბრუნებაც ჩავარდა, log-ში მხოლოდ refs-ის შეცდომა ჩანს.
- **N06:** რომელიმე ref-ის უარყოფა მთელ push-ს შეცდომად აქცევს, tag-ისაც. ამიტომ upstream არ ინიშნება მაშინაც, როცა მხოლოდ tag იყო უარყოფილი. git ასეთ დროს branch-ის upstream-ს ნიშნავს.
- **N07:**
  - შეწყვეტა მხოლოდ fetch-ს (და pull-ის fetch-ს) ეხება. push შეწყვეტისას ბოლომდე სრულდება, მაგრამ log ახლა სწორად წერს, რომ დასრულდა.
  - Windows-ზე `QProcess::terminate()` console პროცესს ვერ აჩერებს, ამიტომ 2 წამის შემდეგ kill-ით ჩერდება, git-ის lock-ების წაშლის გარეშე. ეს ლოდინი worker thread-ზეა.
- **N08:** `git_reference_delete` branch-ის კონფიგურაციას (`branch.<name>.*`) არ შლის. პირველ commit-ს ის ჩვეულებრივ არ აქვს.
- **N10:** untracked text ფაილის ჩვენებისას clean drivers ეშვება, მათ შორის LFS. tracked ფაილების diff ამას უკვე აკეთებდა.
- **N12:** filter, რომელიც მხოლოდ repository-ის კონფიგურაციაშია და გლობალურში არა, libgit2-ში კვლავ არ რეგისტრირდება — რეგისტრაცია startup-ზე გლობალური კონფიგურაციიდან ხდება. ეს აუდიტის პუნქტს არ ეხება.
- Windows-ზე ტესტები ცალკე არ გაშვებულა. shell script-ზე დამოკიდებული სამი ტესტი იქ გამოტოვებულია.

## გაშვებული შემოწმებები

```sh
cmake --build build
ctest --test-dir build --timeout 300 -j 1 -E '^ui_screenshot$' --output-on-failure   # offscreen
```

- [ctest.log](evidence/ctest.log): 44/44 passed, `ctest_exit=0`.
- [before-fix.log](evidence/before-fix.log): ახალი ტესტები `73fdd1d6`-ის წყაროებზე. ტესტები შესწორების patch-ის დროებით უკუგამოყენებით გაეშვა. 12 ჩავარდნა, თითო ახალ ტესტზე ერთი; ძველი ტესტები გადის.
- [after-fix.log](evidence/after-fix.log): იგივე ტესტ-პროგრამები შესწორებულ წყაროებზე, 0 ჩავარდნა.
- [environment.txt](evidence/environment.txt), [changed-files.txt](evidence/changed-files.txt).
- clang-format: ახალი და შეცვლილი ხაზები სუფთაა. ძველი ფორმატირების გადახრები არ შეცვლილა.

## Commit-ები

| Commit | პუნქტი | სათაური |
| --- | --- | --- |
| `8ca3a179` | N01, N09 | Deinitialize submodules with git submodule deinit |
| `401b3d44` | N02, N03 | Move HEAD only from the commit that a commit or an amend started from |
| `c44b21b9` | N04 | Change the files back when undoing fails after the checkout |
| `52dafbb3` | N05 | Push without trusting the remote-tracking branch |
| `35540fe9` | N06 | Fail pushes that the remote rejects |
| `9fcfa721` | N07 | Stop git when a fetch is canceled |
| `47c33eaa` | N08 | Undo the first commit of a repository |
| `9f5b8fc0` | N10 | Stage lines of new files with the line endings of the attributes |
| `ce523b27` | N11 | Keep empty commits when they are reworded |
| `7ce0cf18` | N12 | Read the commands of filters from the repository |

## შეცვლილი ფაილები

- **წყაროები:** `src/git/Cli.cpp`, `Cli.h`, `Commit.cpp`, `Filter.cpp`, `RefState.cpp`, `Remote.cpp`, `Remote.h`, `Repository.cpp`, `Repository.h`, `Rewrite.cpp`, `RunResult.h`, `Signing.cpp`, `Signing.h`, `Submodule.cpp`, `Submodule.h`; `src/ui/DiffModel.cpp`, `RemoteCallbacks.h`, `RepoView.cpp`, `UndoHistory.cpp`.
- **ტესტები:** `test/Filter.cpp`, `Submodule.cpp`, `amend.cpp`, `index.cpp`, `interactive_rebase.cpp`, `signing.cpp`, `undo.cpp`; ახალი `test/transfer.cpp` და მისი ხაზი `test/CMakeLists.txt`-ში.
