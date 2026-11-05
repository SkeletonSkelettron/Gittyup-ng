# Git ოპერაციების შესწორებების გადამოწმება

თარიღი: **2026-10-30**, Asia/Tbilisi.
შედარების საფუძველი: [2026-10-29 აუდიტი](../git-audit-2026-10-29/README.md).
შემოწმების დასაწყისში HEAD იყო `54f2028125bdae69d463427660af43acdbe4028d`; შეფასებულია მაშინ არსებული **22 ფაილის დაუკომიტებელი ცვლილებები** — 780 დამატებული და 103 წაშლილი ხაზი. ქვემოთ ყველა სტატუსი, ხაზის ნომერი და ტესტის შედეგი ამ შენახულ ვერსიას ეხება.

საბოლოო შემოწმებისას სამუშაო ხეში მხოლოდ 3 შეცვლილი ფაილი დარჩა, იგივე HEAD-ით. მომხმარებელმა დააზუსტა, რომ სჭირდება **თავდაპირველი 22 ფაილის ვერსიის შეფასება**. ამიტომ ანგარიში მოგვიანებით შეცვლილ სამუშაო ხეს არ აფასებს. თავდაპირველი patch და ლოგები შენარჩუნებულია; შემდგომი 3-ფაილიანი სხვაობის ასლიც ცალკეა შენახული: [workspace-at-finish.patch](evidence/workspace-at-finish.patch).

შემოწმებული ცვლილებების ზუსტი ასლი: [reviewed-changes.patch](evidence/reviewed-changes.patch).
SHA256: `1fb0b8e9cb59da8089cfc9dbf120654382d0701c6f6115e41d46a309befeb616`.

## შედეგი

თავდაპირველი **12 ხარვეზიდან 10-ის ზუსტი სცენარი აღარ მეორდება**. **F03 ნაწილობრივ გასწორებულია** — ჩვეულებრივი staged ფაილი დაცულია, მაგრამ ignored submodule-ის staged gitlink-ის ცვლილება შემოწმებას უვლის გვერდს. **F09 კვლავ გაუკეთებელია** — მხოლოდ upstream-ის შეცვლა undo-ში არ იწერება.

დამატებითი შემთხვევების შემოწმებამ აჩვენა, რომ სამუშაო დასრულებული ჯერ არ არის. ქვემოთ ოთხ ჯგუფადაა მოცემული დადასტურებული პრობლემები: **ერთი P1 და სამი P2**. ეს არ არის ახალი patch-ით შემოტანის მტკიცება: ნაწილი ადრე არსებული, პირველ აუდიტში დაუფარავი შემთხვევაა; ნაწილი შესწორების არასრული დაფარვაა.

**41 CTest ტესტიდან 40 წარმატებულია**. მხოლოდ `signing::gpg()` ვერ ქმნის გასაღებს გარემოში `gpg-agent`-ის გაშვების პრობლემის გამო. ახალი regression ტესტები გადის. ორი ადრე ჩავარდნილი ქსელური ტესტი და screenshot helper ამ გაშვებაში გამოტოვებულია.

აპლიკაციის წყაროები და მომხმარებლის ტესტები ამ გადამოწმებისას არ შეცვლილა. დამატებულია მხოლოდ ეს ანგარიში, მტკიცებულებები და დიაგნოსტიკური პროგრამები.

## დარჩენილი პრობლემები

### R01 — undo სხვა worktree-ის branch-ს წინასწარ არ ამოწმებს [P1]

**ადგილები:** `src/ui/UndoHistory.cpp:208–210`, `src/git/RefState.cpp:201–207`, `275–285`.

ახალი `isCheckedOutElsewhere` შემოწმება იცავს გადაადგილებულ branch refs-ს, მაგრამ checkout-ის undo ხშირად მხოლოდ `HEAD`-ს ცვლის. ამ დროს guard-ის `moved` სიაში დანიშნულების branch საერთოდ არ არის. `UndoHistory` ფაილებსა და ინდექსს ჯერ ცვლის, branch-ზე HEAD-ის მიბმას კი შემდეგ ცდილობს.

**გამეორება:** `master` მიუთითებს B commit-ზე, `topic` — A-ზე. აპლიკაციაში სრულდება checkout `master`-იდან `topic`-ზე. შემდეგ ცალკე worktree checkout-ს უკეთებს `master`-ს. პირველ view-ში checkout-ის undo იძახება.

- symbolic HEAD-იდან undo აბრუნებს `false`-ს, მაგრამ HEAD რჩება `topic`-ზე, ფაილი და ინდექსი კი უკვე B commit-ის შიგთავსს შეიცავს. საწყისი სუფთა რეპოზიტორია იღებს მოულოდნელ staged ცვლილებას.
- თუ პირველი checkout detached HEAD-ზე იყო, იგივე undo წარმატებით სრულდება და ორივე worktree ერთსა და იმავე `master` branch-ზე აღმოჩნდება.

**მტკიცებულებები:** [undo_worktree.log](evidence/undo_worktree.log), [undo_worktree_detached.log](evidence/undo_worktree_detached.log).

```text
checkout-before: head=topic file=A status= undo=Checkout topic
checkout-undo-occupied: worktree-add=0 undo=0 head=topic file=B index=B status=M  file.txt

detached case: worktree-add=0 undo=1 head=master
```

**შესწორების მიმართულება:** დანიშნულების branch-ის worktree ხელმისაწვდომობა შემოწმდეს ფაილების/index-ის შეცვლამდე, HEAD-only ცვლილების დროსაც. თუ შემდეგ ეტაპზე ოპერაცია მაინც ჩავარდა, აღდგეს index/worktree-ც — არსებული `RefState` rollback მხოლოდ refs-ს ეხება. regression ტესტმა ორივე HEAD რეჟიმში უნდა შეამოწმოს branch-ის, ფაილებისა და staged მდგომარეობის შენარჩუნება.

### R02 — SAFE file checkout ჯერ კიდევ ვერ ხედავს ზოგიერთ ადგილობრივ ცვლილებას [P2]

**ადგილები:** `src/git/Repository.cpp:1443–1451` და `1462–1466`.

თავდაპირველი untracked ფაილის გადაწერის შემთხვევა ახლა იბლოკება, მაგრამ ახალ guard-ს ორი ხვრელი აქვს:

1. **მხოლოდ staged ცვლილება.** `workdirChanges` `files` სიაში აბრუნებს მხოლოდ unstaged ცვლილებებს (`Repository.cpp:142–144`). index-only ცვლილება ინახება `states` map-ში, რომელსაც checkout-ის guard არ იყენებს. შედეგად წინასწარ staged `d/f`-ის ძველი ვერსიის checkout ცვლის სამუშაო ასლს. GUI-ის `SAFE | DONT_UPDATE_INDEX` გზაზე staged შიგთავსი ჯერ შენარჩუნებულია, მაგრამ მომხმარებლის სამუშაო ასლი იცვლება. API-ის default `SAFE` flags-ით იცვლება ინდექსიც და staged ვერსიაც იკარგება. ამ default გზის სხვა მიმდინარე GUI caller არ აღმოჩნდა; ეს უკანასკნელი შედეგი უშუალო API probe-ითაა დადასტურებული.
2. **ადგილობრივად წაშლილი საქაღალდე.** თუ tracked `d` საქაღალდე წაშლილია, `status` აჩვენებს `d/f`-ის წაშლას, ხოლო checkout ითხოვს `d`-ს. descendant შემოწმება დამოკიდებულია `QFileInfo(path).isDir()`-ზე; წაშლილი საქაღალდისთვის ეს `false`-ია. ოპერაცია უარის ნაცვლად ძველ ფაილს თავიდან ქმნის. Tree context menu-ს შეუძლია საქაღალდის გზა უშუალოდ გადასცეს (`src/ui/DetailView.cpp:826–834`).

**მტკიცებულება:** [checkout_edges.log](evidence/checkout_edges.log). იგივე პირობებს bundled libgit2-ის native SAFE checkout უარყოფს `-13`-ით და ადგილობრივ მდგომარეობას ინარჩუნებს.

```text
deleted directory: native=-13, content=<absent>; project=1, content=old
staged file, keep index: native=-13; project=1, worktree=old, index=precious staged
staged file, default SAFE: native=-13; project=1, worktree=old, index=old
```

**შესწორების მიმართულება:** SAFE guard-მა უნდა გაითვალისწინოს staged მდგომარეობაც და შერჩეული directory-prefix-ის ცვლილებები მისი მიმდინარე არსებობისგან დამოუკიდებლად. საჭიროა ეს სამი სცენარი regression ტესტებად.

### R03 — staged submodule gitlink-ს ignore კონფიგურაცია guard-ისგან მალავს [P2]

**ადგილი:** `src/git/Repository.cpp:1336`.

`stagedChangesError()` იყენებს `git diff --cached --quiet`-ს. `submodule.sm.ignore=all` კონფიგურაციით ეს ბრძანება `0`-ს აბრუნებს მაშინაც, როდესაც parent repository-ის ინდექსში submodule gitlink შეცვლილია. ამიტომ cherry-pick იწყება, Cancel/Abort კი წინასწარ staged gitlink-ს HEAD-ის ვერსიაზე აბრუნებს. child repository-ის ფაილები და commit შენარჩუნებულია; იკარგება parent-ის staged არჩევანი.

**მტკიცებულება:** [submodule_staging.log](evidence/submodule_staging.log).

```text
guard_without_ignore=1
guard_with_ignore=0 staged_before=sm
pick=1 abort=1 staged_after= status_after=M sm
```

Probe abort-ს მხოლოდ წარმატებული pick-ის შემდეგ იძახებს. ეს არ არის იმ შემთხვევის ხელოვნური გამეორება, როცა უარყოფილი pick-ის შემდეგ კლიენტი მაინც reset-ს უშვებს.

**შესწორების მიმართულება:** staged ცვლილებების წინაპირობა შესრულდეს `--ignore-submodules=none`-ით ან index/HEAD-ის ობიექტური შედარებით. იგივე helper-ს revert-ც იყენებს, ამიტომ ორივე ოპერაციის regression ტესტია საჭირო.

### R04 — F09: მხოლოდ upstream-ის შეცვლა კვლავ არ ექვემდებარება undo-ს [P2]

**ადგილები:** `src/git/RefState.cpp:116–132`, `src/ui/UndoHistory.cpp:143–144`, ახალი `test/undo.cpp:238–256`.

`RefState::operator==()` ითვალისწინებს `mUpstreams`-ს, მაგრამ `changes()` კვლავ მხოლოდ HEAD/ref OID-ებს ამოწმებს. `Branch::setUpstream`-ის შემდეგ snapshot-ები განსხვავდება, ცვლილებების სია კი ცარიელია და მოქმედება იკარგება. `README.md:62` upstream-ის undo/redo-ს კვლავ აღწერს მხარდაჭერილ შესაძლებლობად.

**მტკიცებულება:** [refs_remotes.log](evidence/refs_remotes.log): `upstream snapshots equal=0 changed refs=0`, `set-upstream=local-upstream canUndo=0`.

ახალი `laterUpstreams` ტესტი ადასტურებს, რომ ძველი მოქმედების undo-მ მოგვიანებით შეცვლილი upstream უნდა შეინარჩუნოს. ეს სასარგებლო დაცვაა, მაგრამ თვით upstream-ის ცვლილების undo-ს არ ამატებს. ტესტის კომენტარი upstream ცვლილებებს პირდაპირ გამორიცხავს მოქმედებებიდან.

**შესწორების მიმართულება:** user-initiated upstream ცვლილება ცალკე undo მოქმედებად ჩაიწეროს; ძველი მოქმედების undo-მ კი მოგვიანებით, დამოუკიდებლად შეცვლილი upstream არ გადაწეროს. ან შესაძლებლობის განზრახ გამორიცხვის შემთხვევაში შეიცვალოს პროდუქტის დოკუმენტაცია და F09 დარჩეს ცნობილ შეზღუდვად.

## წინა აუდიტის 12 პუნქტის სტატუსი

| პუნქტი | სტატუსი | დადასტურება |
| --- | --- | --- |
| F01: untracked file checkout | თავდაპირველი სცენარი გასწორებულია | SAFE უარყოფილია, `precious untracked` შენარჩუნებულია; დაკავშირებული R02 ჯერ რჩება |
| F02: stale index write | გასწორებულია | `g` ინარჩუნებს staging-ს `f`-ის დამატების შემდეგ |
| F03: cherry-pick cancel | ნაწილობრივ გასწორებულია | ჩვეულებრივი staged ახალი ფაილი pick-ს ბლოკავს და რჩება; submodule შემთხვევა R03-შია |
| F04: მეორე merge | გასწორებულია | `second=0 committed=0`; UI-ს state guard-იც აქვს |
| F05: fetched tag-ის წაშლა undo-ით | გასწორებულია | tag არსებობს undo-მდეც და შემდეგაც |
| F06: linked worktree branch fast-forward | თავდაპირველი სცენარი გასწორებულია | `setTarget-success=0`, მეორე worktree სუფთაა; checkout-undo-ს სხვა გზა R01-შია |
| F07: არასრული conflict stages | გასწორებულია | `state=3 Conflicted=3`; modify/delete, delete/modify, add/add ტესტები გადის |
| F08: სხვა remote-ის destination | გასწორებულია | `backup-trunk=0 backup-source=1`; matching remote-ის შემთხვევაც ტესტირდება |
| F09: upstream-only undo | არ გასწორებულა | R04; კვლავ `canUndo=0` |
| F10: rebase committer override | გასწორებულია UI გზაზე | მოთხოვნილი identity შენარჩუნებულია; clean/conflict continuation ტესტები გადის |
| F11: nested rename undo | გასწორებულია | `nested-rename=1 undo=1 old-name-exists=1 renamed-exists=0`; checked-out rename-ც ტესტირდება |
| F12: optional filter crash | გასწორებულია | process=0, filter apply=0, თავდაპირველი შიგთავსი შენარჩუნებულია; required failure ტესტიც გადის |

ძირითადი მტკიცებულებები: [index_worktree.log](evidence/index_worktree.log), [merge_rebase.log](evidence/merge_rebase.log), [refs_remotes.log](evidence/refs_remotes.log), [optional_filter.log](evidence/optional_filter.log).

## შესრულებული შემოწმებები

- `cmake --build build --target check_build -j 4` — წარმატებული, build მიმდინარე წყაროებთან თანხვედრილია (`ninja: no work to do`).
- `git diff --check` — წარმატებული.
- `ctest --test-dir build --output-on-failure --timeout 120 -j 1 -E '^(ui_screenshot|branches_panel|referencelist)$' --output-log /tmp/gittyup-review-2026-10-30-ctest.log` — **40/41 passed**, დრო 44.74 წამი; [ctest.log](evidence/ctest.log).
- ერთადერთი CTest failure: `signing::gpg`, `test/signing.cpp:151`, keygen exit 2. ცალკე დროებით `GNUPGHOME`-ში იგივე ბრძანებაც ვერ უშვებს `gpg-agent`-ს: [gpg-keygen.log](evidence/gpg-keygen.log). SSH signing-ის დანარჩენი შემთხვევები გადის.
- `branches_panel` და `referencelist` თავიდან არ გაშვებულა: წინა აუდიტში DNS/ქსელზე წვდომის გამო ჩაიჭრა. `ui_screenshot` ლოკალური screenshot helper-ია.
- შვიდი C++ source-იდან რვა probe გაშვება (symbolic და detached HEAD ვარიანტები ცალ-ცალკე) დაკომპილირდა მიმდინარე ბიბლიოთეკებთან; რვავე დასრულდა process return code 0-ით. ეს შედეგი ნიშნავს, რომ დიაგნოსტიკური პროგრამები გაეშვა — მათ მიერ გამოვლენილი პრობლემები ზემოთაა აღწერილი.

არ ჩატარებულა Windows/macOS/Release runtime შემოწმება ან რეალურ remote სერვერთან ავტორიზაციის სრული ციკლი. ძველი ანგარიშის შეზღუდვები ძალაში რჩება. პროექტის გარე ქსელური ოპერაციები ამ შემოწმებისას ადგილობრივი დროებითი remotes-ით ჩანაცვლდა მხოლოდ probe-ებში; მომხმარებლის ტესტების წყაროები არ შეცვლილა.

## მტკიცებულებების გამეორება

შემოწმებული 22 ფაილის patch-ის შესაბამისი წყაროებითა და Linux/Ninja build-ით, რეპოზიტორიის root-იდან:

```sh
cmake --build build --target check_build -j 4
python3 docs/git-audit-2026-10-30-review/run_probes.py
```

runner ჯერ ამოწმებს, რომ სამუშაო ხის diff ემთხვევა შენახულ patch-ს; შეუსაბამობისას ჩერდება და წყაროებს არ ცვლის. შემდეგ იღებს compile/link flags-ს არსებული `test_remotes` target-იდან. შედეგები და ბინარები იწერება `/tmp/gittyup-audit-probes-*`-ში. ყველა საზიანო სცენარი იყენებს დროებით რეპოზიტორიებს; production source-სა და რეალურ Git ისტორიას არ ცვლის. შემოწმების შემდეგ შეცვლილ 3-ფაილიან სამუშაო ხეზე runner-ის პირდაპირ გაშვება შეფასებულ ვერსიას აღარ გაიმეორებს.

პირველი აუდიტის merge/cherry-pick probe-ები ამ ანგარიშში ადაპტირებულია: commit/abort სრულდება მხოლოდ წინმსწრები ოპერაციის წარმატებისას. ძველი probe-ის უპირობო `commit`/`reset` გასწორებული guard-ის შემდეგ მცდარი დასკვნის წყარო იქნებოდა. პირველი ანგარიშის წყაროები და ძველი მტკიცებულებები შენარჩუნებულია.

| დიაგნოსტიკური source | დანიშნულება |
| --- | --- |
| [index_worktree.cpp](probes/index_worktree.cpp) | F01/F02/F07-ის ძველი სცენარები |
| [merge_rebase.cpp](probes/merge_rebase.cpp) | F03/F04/F10; გაგრძელება მხოლოდ წარმატების შემდეგ |
| [refs_remotes.cpp](probes/refs_remotes.cpp) | F05/F06/F08/F09/F11 |
| [optional_filter.cpp](probes/optional_filter.cpp) | F12 |
| [checkout_edges.cpp](probes/checkout_edges.cpp) | R02, bundled libgit2-თან შედარება |
| [submodule_staging.cpp](probes/submodule_staging.cpp) | R03 |
| [undo_worktree.cpp](probes/undo_worktree.cpp) | R01, ორი HEAD რეჟიმი |

სრული probe output: [probe-run.log](evidence/probe-run.log). გარემო და diff-ის იდენტიფიკაცია: [environment.txt](evidence/environment.txt).
