# Git ოპერაციების სისწორის აუდიტი

თარიღი: **2026-10-29**, Asia/Tbilisi. პროექტი: **Gittyup-ng 0.0.6**.
შემოწმებული branch: `git-cli-migration`.
შემოწმებული commit: `54f2028125bdae69d463427660af43acdbe4028d`.

## შედეგი

დადასტურდა **12 ხარვეზი: 6 — P1, 6 — P2**. ყველაზე საყურადღებოა დაუკომიტებელი ფაილის გადაწერა/წაშლა, ინდექსში უკვე დამატებული ცვლილებების დაკარგვა და არასწორი merge ისტორიის შექმნა. ჩვეულებრივი სცენარების ტესტების უმრავლესობა წარმატებით სრულდება, მაგრამ ქვემოთ აღწერილ შემთხვევებს არსებული ტესტები ვერ აფიქსირებს.

არსებული 43 CTest ტესტიდან **40 წარმატებულია**, **3 ვერ დასრულდა წარმატებით**: ორი GitHub-ზე წვდომაზეა დამოკიდებული; GPG ტესტი გასაღების შექმნისას, პროექტის ხელმოწერის კოდამდე, ვერ უშვებს `gpg-agent`-ს. სრული შედეგები შენახულია [ctest.log](evidence/ctest.log)-ში.

ეს არის შემოწმების ანგარიში. აპლიკაციის წყაროები არ შეცვლილა; დამატებულია მხოლოდ ანგარიში, მტკიცებულებები და განმეორებადი დიაგნოსტიკური პროგრამები. შემოწმების დაწყებისას სამუშაო ხე სუფთა იყო. საზიანო სცენარები შესრულდა მხოლოდ დროებით რეპოზიტორიებში.

პრიორიტეტები: **P1** — პირველ რიგში გამოსასწორებელი, რადგან საფრთხეს უქმნის ფაილებს, ინდექსის შიგთავსს ან ისტორიის სისწორეს; **P2** — ფუნქციური ხარვეზი კონკრეტულ პირობებში. ჩამონათვალი არ ნიშნავს, რომ სხვა ხარვეზები გამორიცხულია.

| ID | პრიორიტეტი | დადასტურებული პრობლემა | ძირითადი ადგილი |
| --- | --- | --- | --- |
| F01 | P1 | უსაფრთხო file checkout გადაწერს untracked ფაილს | `src/git/Repository.cpp:1414` |
| F02 | P1 | stage ოპერაცია მოძველებული ინდექსით შლის სხვა staged ცვლილებებს | `src/git/Index.cpp:285` |
| F03 | P1 | cherry-pick-ის გაუქმება შლის მანამდე staged ახალ ფაილს | `src/ui/RepoView.cpp:1841` |
| F04 | P1 | მიმდინარე merge-ის დროს მეორე merge არასწორ parent-ს აკომიტებს | `src/git/Repository.cpp:1169` |
| F05 | P1 | undo შლის მოქმედების შემდეგ fetch-ით მიღებულ tag-ს | `src/git/RefState.cpp:206` |
| F06 | P1 | branch-ის fast-forward არღვევს სხვა worktree-ის მდგომარეობას | `src/git/Reference.cpp:109` |
| F07 | P2 | modify/delete კონფლიქტი არასწორად კლასიფიცირდება | `src/git/Index.h:41` |
| F08 | P2 | სხვა remote-ზე push იყენებს ძველი upstream-ის branch-ის სახელს | `src/git/Remote.cpp:658` |
| F09 | P2 | მხოლოდ upstream-ის ცვლილება undo ისტორიაში არ ხვდება | `src/git/RefState.cpp:115` |
| F10 | P2 | rebase კარგავს მითითებულ committer name/email-ს | `src/git/Repository.cpp:1229` |
| F11 | P2 | `feature` → `feature/nested` rename-ის undo ვერ სრულდება | `src/git/RefState.cpp:165` |
| F12 | P2 | optional filter-ის ჩავარდნა Debug აპლიკაციას წყვეტს | `src/git/Filter.cpp:83` |

## მეთოდი და გარემო

- კოდის შემოწმება: Git CLI/libgit2 საზღვარი; index/status/staging; checkout/reset; stash; branch/tag/ref; fetch/push/pull; commit/merge/rebase/cherry-pick/revert; undo/redo და UI-ს შესაბამისი გამოძახებები.
- არსებული Debug build-ის გადამოწმება: `cmake --build build --target check_build -j 4` — წარმატებული, `ninja: no work to do`.
- Linux / Fedora 44; Git **2.56.0**; Qt **6.11.2**; CMake **4.3.0**; პროექტის bundled libgit2.
- სრული ტესტების გაშვება: `ctest --test-dir build --output-on-failure --timeout 180 -j 1 -E '^ui_screenshot$' --output-log /tmp/gittyup-git-audit-ctest.log`.
- ოთხი დამოუკიდებელი C++ probe დაკომპილირდა და გაეშვა არსებული პროექტის ბიბლიოთეკებთან. შეიქმნა მხოლოდ ადგილობრივი დროებითი რეპოზიტორიები და ადგილობრივი remotes/worktree-ები.
- UI-ს გზა დადგენილია კოდის წაკითხვით. probe-ები უშუალოდ ამ გზებზე გამოყენებულ API-ებს იძახებს; F05/F09/F11 დამატებით იყენებს რეალურ `MainWindow`/`UndoHistory` ობიექტებს offscreen რეჟიმში. GUI-ში ხელით კლიკებით შემოწმება არ ჩატარებულა.

## დადასტურებული ხარვეზები

### F01 — უსაფრთხო file checkout გადაწერს untracked ფაილს [P1]

**პირობა:** ფაილი `f` არსებობს ძველ commit-ში, შემდეგ commit-ში წაშლილია, სამუშაო საქაღალდეში კი იგივე სახელით ახალი untracked ფაილია. ძველი ვერსიის დაბრუნება იძახებს `checkout(old, ..., {"f"}, SAFE | DONT_UPDATE_INDEX)`-ს.

`Repository.cpp:1414–1432` ამოწმებს მხოლოდ `git diff`-ით ნაპოვნ unstaged tracked ცვლილებებს. untracked ფაილი ამ სიაში არ ხვდება და `git restore` (`1451–1458`) მას პირდაპირ გადაწერს. ეს გზა გამოიყენება ისტორიული ფაილის checkout-ისას: `src/ui/RepoView.cpp:2206`.

**მტკიცებულება:** `safe path checkout result=1 contents=old tracked`. იგივე პირობებში bundled libgit2-ის `git_checkout_tree` იმავე flags-ით აბრუნებს `-13`-ს და ტოვებს `precious untracked` შიგთავსს. იხ. [index_worktree.log](evidence/index_worktree.log).

**შესწორების მიმართულება:** SAFE გზამ უნდა შეამოწმოს untracked შეჯახებებიც, სანამ `restore` რაიმეს გადაწერს. საჭიროა regression ტესტი, რომელიც ადასტურებს ფაილის შიგთავსის შენარჩუნებას და ოპერაციის უარყოფას.

### F02 — stage ოპერაცია დისკზე არსებულ ინდექსს მოძველებული ასლით გადაწერს [P1]

**პირობა:** აპლიკაციას უკვე აქვს `Index` ობიექტი; შემდეგ სხვა Git პროცესი stage-ში ამატებს `g`-ს; აპლიკაციაში stage-ში ემატება `f`.

`Index::setStaged` ჯერ ასრულებს `git_index_write(d->index)`-ს (`src/git/Index.cpp:285`) და შემდეგ უშვებს CLI `git add`/`reset`-ს. მოძველებული in-memory ინდექსი ამ შუალედში დისკზე განახლებულ ინდექსს გადაწერს.

**მტკიცებულება:** `before stage f: g`, შემდეგ `after stage f: f`; `g` ინდექსში აღარ არის. ეს არის staged მდგომარეობის/ვერსიის დაკარგვა; probe-ში სამუშაო საქაღალდის `g` ფაილი არ წაშლილა. იხ. [index_worktree.log](evidence/index_worktree.log).

**შესწორების მიმართულება:** CLI staging-მდე მოძველებული ინდექსის ბრმად ჩაწერა უნდა გამოირიცხოს; libgit2-ით საჭირო ცვლილებები უნდა შეთანხმდეს დისკის მიმდინარე ინდექსთან. ტესტში გარე `git add` უნდა შესრულდეს აპლიკაციის refresh-სა და staging-ს შორის; სასურველია ნაწილობრივ staged ფაილიც.

### F03 — cherry-pick-ის Cancel/Abort შლის მანამდე staged ახალ ფაილს [P1]

**პირობა:** ახალი `unrelated` ფაილი უკვე staged არის. შემდეგ იწყება commit-ის cherry-pick, რომელიც სხვა ფაილს ცვლის; მომხმარებელი აუქმებს commit-ის შეტყობინების დიალოგს.

`src/git/Repository.cpp:1293` იყენებს `cherry-pick --no-commit`-ს და staged ინდექსის სისუფთავეს არ ამოწმებს. დიალოგის უარყოფა (`src/ui/RepoView.cpp:1839–1841`) იძახებს `mergeAbort`-ს; ის გადადის `Commit::resetMerge`-ზე (`src/git/Commit.cpp:337–339`), ანუ `git reset --merge HEAD`-ზე. საწყისი ინდექსის snapshot არ ინახება და ახალი staged ფაილიც იშლება.

**მტკიცებულება:** `CHERRY_PICK_CANCEL success=1 aborted=1 unrelated_file_exists=0 cached_after=`. Probe უშუალოდ იმავე pick/abort API-ებს ასრულებს; თავად დიალოგი არ გაუხსნია. დამატებითი სცენარი აჩვენებს, რომ ავტომატური commit-ისას `unrelated` ფაილიც cherry-picked commit-ში ხვდება. იხ. [merge_rebase.log](evidence/merge_rebase.log).

**შესწორების მიმართულება:** ამ UI ოპერაციას სჭირდება სუფთა staged ინდექსის წინაპირობა ან ოპერაციამდე არსებული index/worktree მდგომარეობის ზუსტი აღდგენა Cancel/Abort-ზე. regression ტესტმა უნდა შეინარჩუნოს წინასწარ staged ახალი და შეცვლილი ფაილები. მსგავსი precondition საჭიროა revert-ის გზაზეც შესამოწმებლად; აქ დინამიკურად დადასტურებულია cherry-pick.

### F04 — მეორე merge მიმდინარე merge-ის შედეგს სხვა parent-ით აკომიტებს [P1]

**პირობა:** მიმდინარე branch-სა და ორ სხვა branch-ს აქვთ დამოუკიდებელი ცვლილებები. პირველი merge სრულდება `NoCommit`-ით; მეორე branch-ის merge იწყება პირველის დასრულებამდე.

Git მეორე merge-ს უარყოფს, მაგრამ `Repository::merge` წარმატებას ადგენს მხოლოდ `state() == MERGE`-ით (`src/git/Repository.cpp:1167–1173`). პირველი merge-ის `MERGE_HEAD` ჯერ არსებობს, ამიტომ მეორე გამოძახებაც `true`-ს აბრუნებს. UI წარმატების გზაზე `commit(msg, upstream)`-ს იძახებს მეორე მოთხოვნილი parent-ით (`src/ui/RepoView.cpp:1583`).

**მტკიცებულება:** `NESTED_MERGE first=1 second=1 committed=1 files=a,main,one second_parent=<two> expected_one=<one> wrong_two=<two>`. ისტორიაში `two` merged ჩანს, მისი ფაილი კი არ არის; `one`-ის ფაილი არსებობს შესაბამისი parent-ის გარეშე. იხ. [merge_rebase.log](evidence/merge_rebase.log).

**UI ხელმისაწვდომობა:** `MergeDialog` იძლევა `NoCommit`-ს; `RepoView.cpp:1556–1558` ტოვებს merge-ს მიმდინარე მდგომარეობაში; `src/ui/MenuBar.cpp:1130` Merge-ს მხოლოდ HEAD-ის ვალიდურობის მიხედვით რთავს. მეორე merge-ის დაწყებისას state guard არ არის.

**შესწორების მიმართულება:** დაუმთავრებელი ოპერაციისას ახალი merge უნდა უარყოფილ იქნას; CLI-ის შეცდომა არსებული merge state-ისგან უნდა განირჩეს. commit-მდე მოთხოვნილი parent უნდა ემთხვეოდეს მოქმედ `MERGE_HEAD`-ს.

### F05 — undo შლის მოგვიანებით fetch-ით მიღებულ tag-ს [P1]

**პირობა:** ისტორიაში ჩაიწერა მოქმედება, მაგალითად commit. შემდეგ fetch-ით მოდის ახალი tag. ამის შემდეგ სრულდება ძველი მოქმედების undo.

`src/ui/UndoHistory.cpp:139–144` fetch-ით დამატებულ tag-ს განზრახ არ წერს ახალ undo მოქმედებად. `apply` ამოწმებს მხოლოდ თავდაპირველი მოქმედების შეცვლილ refs-ს (`195–197`), ხოლო `RefState::restore` აღადგენს მთელ ძველ snapshot-ს და შლის ყველა მიმდინარე ref-ს, რომელიც მასში არ იყო (`src/git/RefState.cpp:206–227`).

**მტკიცებულება:** `fetch=0 fetched-tag-before-undo=1 undo=1 fetched-tag-after-undo=0`. გამოყენებულია რეალური `UndoHistory`. იხ. [refs_remotes.log](evidence/refs_remotes.log).

**შესწორების მიმართულება:** undo-მ მხოლოდ კონკრეტული მოქმედებით შეცვლილი refs/config უნდა აღადგინოს. regression ტესტმა ძველი commit-ის undo/redo-ის დროს უნდა შეინარჩუნოს შემდეგ მიღებული tag.

### F06 — fast-forward უგულებელყოფს სხვა linked worktree-ში checkout-ს [P1]

`Reference::setTarget` იყენებს `git update-ref`-ს (`src/git/Reference.cpp:109–126`) და არ ამოწმებს, branch სხვა worktree-ში ხომ არ არის checkout-ებული. `pullBranch` (`src/ui/RepoView.cpp:1373`) და drag/drop fast-forward (`src/ui/RefDrop.cpp:176`) მხოლოდ მიმდინარე view-ის HEAD-ს ითვალისწინებს.

**მტკიცებულება:** დროებით linked worktree-ში `git branch -f linked TARGET` სწორად სრულდება შეცდომით `128`, ხოლო პროექტის `setTarget` წარმატებულია. linked worktree-ის HEAD გადაადგილდება, index/worktree კი ძველი რჩება: `linked-file-exists=0 linked-status=D  only-in-new-commit.txt`. იხ. [refs_remotes.log](evidence/refs_remotes.log).

**შესწორების მიმართულება:** branch-ის გადაადგილებამდე უნდა შემოწმდეს ყველა linked worktree. სხვა worktree-ში checkout-ებული branch-ის ცვლილება უნდა უარყოფილ იქნას ან იმ worktree-ის უსაფრთხო განახლებით შესრულდეს.

### F07 — არასრული conflict stages არ ითვლება კონფლიქტად [P2]

`Index::Conflict::isValid` მოითხოვს ancestor/ours/theirs-ის სამივე ჩანაწერს (`src/git/Index.h:41–42`). modify/delete კონფლიქტს ერთ-ერთი მხარე არ აქვს, add/add-ს კი ancestor შეიძლება არ ჰქონდეს. ამიტომ `Index::isStaged` არ უბრუნებს `Conflicted`-ს (`src/git/Index.cpp:203–206`).

**მტკიცებულება modify/delete-ზე:** `state=1 Conflicted=3 hasConflicts=1 conflict.isValid=0`. UI-ს `FileContextMenu.cpp:300–309` შესაბამისად აღარ აჩვენებს `Take Ours`/`Take Theirs`-ს. იხ. [index_worktree.log](evidence/index_worktree.log).

**შესწორების მიმართულება:** კონფლიქტის არსებობა ინდექსის conflict entry-ებით უნდა დადგინდეს და სამივე მხარის არსებობაზე არ იყოს დამოკიდებული. ტესტებმა უნდა მოიცვას modify/delete, delete/modify და add/add როგორც API-ში, ისე მენიუს ხელმისაწვდომობაში.

### F08 — სხვა remote-ზე push არასწორ დანიშნულების branch-ს ირჩევს [P2]

**პირობა:** ადგილობრივი `master` მიჰყვება `origin/trunk`-ს; მომხმარებელი იმავე branch-ს აგზავნის `backup` remote-ზე, კონკრეტული destination-ის გარეშე.

`RepoView.cpp:1910–1912` უცხო remote-ის upstream-ს ასუფთავებს, მაგრამ `Remote::push` ხელახლა კითხულობს `branch.master.merge`-ს და იყენებს `trunk`-ს remote-ის დამთხვევის შემოწმების გარეშე (`src/git/Remote.cpp:658–661`).

**მტკიცებულება ადგილობრივ bare remote-ზე:** `push-to-backup result=0 source=master backup-trunk=1 backup-source=0`. შეიქმნა `backup/trunk`, ადგილობრივი branch-ის შესაბამისი სახელი კი არ შეიქმნა. იხ. [refs_remotes.log](evidence/refs_remotes.log).

**შესწორების მიმართულება:** upstream-ის destination გამოიყენოს მხოლოდ მაშინ, როდესაც `branch.<name>.remote` არჩეულ remote-ს ემთხვევა; სხვა შემთხვევაში გამოიყენოს UI-ში ნაჩვენები/არჩეული დანიშნულება. ტესტმა რეალური push უნდა შეასრულოს მეორე ადგილობრივ remote-ზე.

### F09 — მხოლოდ upstream-ის შეცვლა undo-ში არ იწერება [P2]

`RefState::operator==` ადარებს `mUpstreams`-საც (`src/git/RefState.cpp:148–150`), მაგრამ `changes()` ამოწმებს მხოლოდ HEAD/ref OID-ებს (`115–131`). ამიტომ `UndoHistory::check` განსხვავებულ snapshot-ებს ხედავს, შემდეგ ცარიელ ცვლილებებს პოულობს და მოქმედებას აგდებს (`src/ui/UndoHistory.cpp:143–144`).

**მტკიცებულება:** `upstream snapshots equal=0 changed refs=0`; რეალური `Branch::setUpstream`-ისა და `history.check()`-ის შემდეგ: `set-upstream=local-upstream canUndo=0`. იხ. [refs_remotes.log](evidence/refs_remotes.log).

**შესწორების მიმართულება:** ცვლილებების სიამ და შემოწმებამ upstream config-იც უნდა გაითვალისწინოს. საჭიროა set/unset/change upstream-ის ცალკე undo/redo ტესტები.

### F10 — rebase კარგავს committer override-ს [P2]

`Repository::rebase` ქმნის `Rebase` ობიექტს მითითებული სახელითა და email-ით (`src/git/Repository.cpp:1221`), მაგრამ შემდეგ იძახებს `rebaseContinue`-ს (`1229`). ის ხელახლა ხსნის `Rebase`-ს (`1234`, `1198`) override-ების გარეშე. `Rebase::commit` შედეგად კონფიგურაციის identity-ს იყენებს (`src/git/Rebase.cpp:76–77`).

**მტკიცებულება:** მოთხოვნილია `Override User <override@example.test>`, მიღებულია `Configured User <configured@example.test>`; rebase წარმატებით დასრულდა, `state=0`. იხ. [merge_rebase.log](evidence/merge_rebase.log).

**შესწორების მიმართულება:** continuation-ის ყველა გამოძახებამ უნდა შეინარჩუნოს მითითებული identity. ტესტები საჭიროა როგორც უკონფლიქტო, ისე კონფლიქტის შემდეგ გაგრძელებული rebase-ისთვის.

### F11 — nested branch rename-ის undo ვერ სრულდება [P2]

**პირობა:** `feature` წარმატებით გადაერქვა `feature/nested`-ად. undo ძველი ref-ის შექმნას ცდილობს ახლის წაშლამდე.

`RefState::restore` ჯერ ქმნის refs-ს (`src/git/RefState.cpp:165–177`), შემდეგ შლის ზედმეტებს (`206–227`). `refs/heads/feature` და `refs/heads/feature/nested` ამ რიგით ერთდროულად ვერ იარსებებს; წარმოიქმნება file/directory კონფლიქტი.

**მტკიცებულება რეალური UndoHistory-ით:** `nested-rename=1 undo=0 old-name-exists=0 renamed-exists=1`. იხ. [refs_remotes.log](evidence/refs_remotes.log).

**შესწორების მიმართულება:** rename-ის აღდგენის რიგი უნდა ითვალისწინებდეს ref-prefix კონფლიქტებს და ჩავარდნისას ნაწილობრივ ცვლილებებსაც აკონტროლებდეს. საჭიროა ორივე მიმართულების rename/undo/redo სცენარები.

### F12 — optional filter-ის შეცდომა Debug build-ს წყვეტს [P2]

გლობალური filter-ის `required=false`, `clean=cat`, `smudge=false` კონფიგურაციით, libgit2-ის filter pipeline-ში `stream_close` შედის შეუსრულებელ passthrough გზაში და იძახებს `assert(false)`-ს (`src/git/Filter.cpp:71–84`).

**მტკიცებულება:** რეალური პროექტის filter-ის `git_filter_list_apply_to_buffer`-ით გამოყენება სრულდება `Assertion 'false' failed`-ით და process return code `-6`-ით (SIGABRT). ეს ეხება libgit2-ის filter გზას, რომელსაც clone/rebase კვლავ იყენებს; CLI checkout-ის წარმატებული `Filter` ტესტი ამ გზას არ ამოწმებს. იხ. [optional_filter.log](evidence/optional_filter.log).

Optional filter-ის non-zero exit-ისას უცვლელი შიგთავსის გატარების მოთხოვნა აღწერილია თვით პროექტის Git submodule-ის დოკუმენტაციაშიც: `dep/git/git/Documentation/gitattributes.txt:426–430`.

**შესწორების მიმართულება:** შენარჩუნდეს საწყისი stream content და optional filter-ის შეცდომისას შესრულდეს passthrough. Release build-ის ქცევა არ შემოწმებულა; დადასტურებული შედეგი ეხება მიმდინარე Debug build-ს. საჭიროა optional და required filter failure-ის ცალკე ტესტები უშუალოდ libgit2 pipeline-ზე.

## არსებული ტესტების შედეგები და შეზღუდვები

| შემოწმება | შედეგი | განმარტება |
| --- | --- | --- |
| `check_build` | წარმატებული | არსებული build მიმდინარე წყაროებთან თანხვედრილია |
| 43 CTest ტესტი | 40 passed, 2 failed, 1 timeout | სრული stdout/stderr: [ctest.log](evidence/ctest.log) |
| `branches_panel` | failed | `Could not resolve host: github.com` |
| `referencelist` | timeout, 180 წამი | GitHub clone-ზე დამოკიდებული ტესტი მხოლოდ accepted სიგნალს ელოდება; failure-ზე ლოდინიდან ვერ გამოდის (`test/referencelist.cpp:53–57`) |
| `signing` | failed, `gpg()` | გასაღების შექმნა ვერ უშვებს `gpg-agent`-ს; ჩავარდნა `test/signing.cpp:151`-ზეა, აპლიკაციის GPG signing-მდე |
| დამოუკიდებელი GPG keygen | exit 2 | იგივე გარემოს შეცდომა: `No agent running`; [gpg-keygen.log](evidence/gpg-keygen.log) |
| 4 დამატებითი probe | დადასტურდა ზემოთ აღწერილი ქცევა | პირველი სამი დასრულდა 0-ით; optional filter დასრულდა მოსალოდნელი SIGABRT-ით |

გამოტოვებულია `ui_screenshot`: ეს არის მიმდინარე checkout-ში არსებული ლოკალური screenshot helper, არა Git correctness ტესტი. `config` ტესტი build-ში არ არის ჩართული (`GITTYUP_CI_TESTS` გამორთულია); გლობალური Git კონფიგურაციის შემცვლელი ტესტი არ გაშვებულა.

არსებული წარმატებული ტესტები მოიცავს ჩვეულებრივ index/staging, checkout/branch, merge/rebase, interactive rebase, undo/refdrop, local pull, tag/remote CRUD, stash, revert, submodule, status refresh და SSH signing სცენარებს. მათი წარმატება არ ფარავს ზემოთ აღწერილ კონფლიქტურ/შუალედურ მდგომარეობებს.

არ შემოწმებულა Windows/macOS runtime, Release build, რეალური GitHub/GitLab/Gitea ავტორიზაცია, ქსელური clone/fetch-ის წარმატებული დასრულება, LFS სერვერთან სრული ციკლი და ყველა შესაძლო პარალელური ოპერაცია. Revert-ის worker-thread index access (`src/git/Commit.cpp:269`) დამატებით race-analysis-ს საჭიროებს; დაზიანება ან crash არ დადასტურებულა და 12 ხარვეზში არ შედის.

უარყოფილი ჰიპოთეზა: ამ გარემოს Git 2.56.0 წარმატებული `revert --no-commit`-ისას ქმნის `REVERT_HEAD`-ს (`REVERT result=1 state=2 REVERT_HEAD=1`). მისი არარსებობა ხარვეზად არ არის დარეგისტრირებული.

## მტკიცებულებების განმეორება

რეპოზიტორიის root-იდან, არსებული **Linux/Ninja Debug** build-ით:

```sh
cmake --build build --target check_build -j 4
python3 docs/git-audit-2026-10-29/run_probes.py
```

სხვა build directory-სთვის runner-ს აქვს `--build-dir /absolute/path` პარამეტრი. runner იღებს compile/link flags-ს არსებული `test_remotes` target-იდან და ოთხივე შენახულ probe-ს ცალ-ცალკე აკომპილირებს. ბინარები და ახალი logs იწერება `/tmp/gittyup-audit-probes-*`-ში; ზუსტი საქაღალდე ბოლოს იბეჭდება. Git ცვლილებები სრულდება `QTemporaryDir` რეპოზიტორიებში. runner თიშავს core dump-ებს, რადგან ბოლო probe გამიზნულად აღწევს დადასტურებულ assertion-ს.

**runner-ის exit 0 არ ნიშნავს, რომ ხარვეზები გამოსწორდა.** ეს დიაგნოსტიკური დემონსტრაციებია; შეადარეთ შიგთავსი და return code-ები ზემოთ აღწერილ სწორ ქცევას. დროებითი commit hash-ები გაშვებებს შორის განსხვავდება.

წყაროები და შედეგები:

- [index_worktree.cpp](probes/index_worktree.cpp) → F01/F02/F07 → [log](evidence/index_worktree.log)
- [merge_rebase.cpp](probes/merge_rebase.cpp) → F03/F04/F10 და revert control → [log](evidence/merge_rebase.log)
- [refs_remotes.cpp](probes/refs_remotes.cpp) → F05/F06/F08/F09/F11 → [log](evidence/refs_remotes.log)
- [optional_filter.cpp](probes/optional_filter.cpp) → F12 → [log](evidence/optional_filter.log)
- [run_probes.py](run_probes.py) — კომპილაცია/გაშვება

რეკომენდებული რიგი: ჯერ F01/F02/F03 ფაილებისა და staged მდგომარეობის დასაცავად; შემდეგ F04/F05/F06 ისტორიისა და refs/worktree თანხვედრისთვის; ბოლოს P2 ფუნქციური ხარვეზები და regression ტესტების დამატება. ახალი ტესტები უნდა ამოწმებდეს საწყისი შიგთავსის/ინდექსის შენარჩუნებასაც და არა მხოლოდ ოპერაციის დაბრუნებულ მნიშვნელობას.
