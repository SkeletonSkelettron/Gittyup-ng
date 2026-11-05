# მოთხოვნილი ოთხი საკითხის გადამოწმება

თარიღი: **2026-10-30**, Asia/Tbilisi.
შემოწმებული commit: **`73fdd1d6a418ffc69f70d7f0918617b4925cec39`**.
Tracked სამუშაო ხე შემოწმების დასაწყისსა და ბოლოს სუფთაა; აპლიკაციის წყაროები და ტესტები არ შეცვლილა.

გადამოწმებულია მხოლოდ მომხმარებლის ბოლო შეტყობინებაში ჩამოთვლილი ოთხი სცენარი. ეს შედეგები არ არის მთელი პროექტის ახალი აუდიტი.

| საკითხი | შედეგი |
| --- | --- |
| P1: სხვა worktree-ის branch-ზე უარყოფილი undo ცვლის ფაილებს/index-ს | **გასწორებულია** — უარისას HEAD, ფაილი და ინდექსი უცვლელი რჩება |
| P2: SAFE checkout ცვლის staged ფაილის სამუშაო ასლს ან აღადგენს წაშლილ საქაღალდეს | **გასწორებულია** — ოპერაცია უარყოფილია და საწყისი მდგომარეობა შენარჩუნებულია |
| P2: `ignore=all` მალავს staged submodule-ს cherry-pick-ის guard-ისგან | **გასწორებულია** — pick უარყოფილია და staged gitlink შენარჩუნებულია |
| P2: upstream-ის ცვლილების undo არ მუშაობს | **ფუნქცია არ დამატებულა** — განზრახ არ ითვლება undo მოქმედებად; README ახლა ამ შეზღუდვას სწორად ასახავს |

## დადასტურება

**1. Worktree undo.** `UndoHistory.cpp:201–204` იძახებს `canRestore`-ს checkout-მდე. შემოწმებულია symbolic და detached HEAD: ორივეგან undo აბრუნებს `false`-ს; ფაილი და ინდექსი რჩება `A`, status ცარიელია. დანიშნულების branch სხვა worktree-შია checkout-ებული. მტკიცებულებები: [symbolic](evidence/undo_worktree.log), [detached](evidence/undo_worktree_detached.log).

**2. SAFE checkout.** `Repository.cpp:1493–1497` ითვალისწინებს staged ცვლილებებსაც; `1509–1512` directory-prefix-ს მისი მიმდინარე არსებობისგან დამოუკიდებლად ამოწმებს. დამოუკიდებელ probe-ში სამი შემთხვევა უარყოფილია: წაშლილი directory, staged ფაილი `DONT_UPDATE_INDEX`-ით და staged ფაილი default SAFE-ით. სამუშაო ასლი და cached შიგთავსი შენარჩუნებულია. იგივე სცენარებს bundled libgit2 უარყოფს `-13`-ით. მტკიცებულება: [checkout_edges.log](evidence/checkout_edges.log).

**3. Ignored submodule.** `Repository.cpp:1372–1374` ამატებს `--ignore-submodules=none`-ს. დამოუკიდებელ probe-ში `submodule.sm.ignore=all`-ის მიუხედავად `guard_with_ignore=1`, `pick=0`, `abort=0`, `staged_after=sm`. Abort მხოლოდ წარმატებული pick-ის შემდეგ გამოიძახებოდა, ამიტომ უარყოფის შემდეგ ხელოვნური reset არ შესრულებულა. `revert` unit test ფარავს მეორე კონფიგურაციასაც — `diff.ignoreSubmodules=all`. მტკიცებულება: [submodule_staging.log](evidence/submodule_staging.log).

**4. Upstream-only undo.** საწყისი upstream `upstream-one` იცვლება `upstream-two`-ზე. Snapshot-ები განსხვავდება, მაგრამ `changed-refs=0`, `canUndo=0`; upstream რჩება `upstream-two`. შესაბამისად ფუნქცია კვლავ არ არის რეალიზებული. `README.md:62–63` upstream-only undo-ს აღარ აცხადებს მხარდაჭერილად; აღწერს მხოლოდ branch-ის აღდგენისას მისი upstream-ის დაბრუნებას. ახალი `test/undo.cpp:238–277` ადასტურებს, რომ სხვა მოქმედების undo/redo მოგვიანებით შეცვლილ upstream-ს ინარჩუნებს. ეს დაცვის გასწორებაა და upstream-ის ცალკე undo-ს დამატებას არ ნიშნავს. მტკიცებულება: [upstream_only.log](evidence/upstream_only.log).

## გაშვებული შემოწმებები

```sh
cmake --build build --target test_undo test_fileContextMenu test_revert -j 4
ctest --test-dir build --output-on-failure --timeout 120 -j 1 -R '^(undo|fileContextMenu|revert)$'
python3 docs/git-audit-2026-10-30-four-issues/run_probes.py
```

- Build: წარმატებული; `ninja: no work to do`.
- **სამივე შესაბამისი CTest target წარმატებულია**: `fileContextMenu`, `undo`, `revert`; 2.39 წამი. [ctest.log](evidence/ctest.log).
- ხუთივე დიაგნოსტიკური გაშვება დასრულდა process return code 0-ით: worktree-ის ორი რეჟიმი, checkout-ის სამი სცენარი ერთ probe-ში, submodule და upstream-only. Return code 0 ნიშნავს probe-ის დასრულებას; upstream-ის მხარდაჭერის არქონა ცალკე შედეგია. [probe-run.log](evidence/probe-run.log).
- `git diff --check`: წარმატებული.

Probe-ები იყენებს მხოლოდ ადგილობრივ დროებით რეპოზიტორიებს. Runner ამოწმებს commit-სა და tracked diff-ს [snapshot.json](evidence/snapshot.json)-ის მიხედვით, შემდეგ კომპილაციის/link flags-ს იღებს არსებული Linux/Ninja build-ის `test_remotes` target-იდან. ბინარები და ახალი შედეგები ინახება `/tmp/gittyup-audit-probes-*`-ში.

შენახული წყაროები: [undo_worktree.cpp](probes/undo_worktree.cpp), [checkout_edges.cpp](probes/checkout_edges.cpp), [submodule_staging.cpp](probes/submodule_staging.cpp), [upstream_only.cpp](probes/upstream_only.cpp). გაშვების სკრიპტი: [run_probes.py](run_probes.py).
