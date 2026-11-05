# Claude-ის Git შესწორებების დამოუკიდებელი შემოწმება

თარიღი: **2026-10-30**, Asia/Tbilisi. შემოწმებული ვერსია: **`ed2e86c8c919f99fcbb2f78a8d953030d14e9569`**. Git-ის შესწორებების ბოლო commit: `7ce0cf185dd5960e47540fe26f12fcb6e561bef8`; მათ შორის დამატებულია მხოლოდ ვერსიის ნომერი და changelog. თავდაპირველი აუდიტის ბაზა: `73fdd1d6a418ffc69f70d7f0918617b4925cec39`.

**Claude-მა შედეგები შეინახა** [შესწორებების ანგარიშში](../git-audit-2026-10-30-fixes/README.md): არსებობს before/after ლოგები, სრული CTest-ის ლოგი, გარემოს აღწერა და N01–N12-ის შესაბამისი 10 commit. ცვლილებები და ახალი regression ტესტები გადავამოწმე კოდის წაკითხვით, სრული ნაკრების დამოუკიდებელი გაშვებით და დამატებითი diagnostic პროგრამებით.

**შედეგი: 11 პუნქტი დახურულია შემოწმებულ სცენარებში; N04 ნაწილობრივაა გასწორებული.** საწყისი 12 სცენარის regression ტესტები გადის, მაგრამ N04-ის ახალ rollback-ს შეუძლია უკვე staged ცვლილების წაშლა. შესაბამისად, Claude-ის ანგარიშის განცხადება „12-ვე პუნქტი დახურულია“ ზედმეტად ფართოა. ცალკე დადასტურდა required clean filter-ის ჩავარდნისას partial staging-ის Debug crash; მისი წარმოშობა ამ შესწორებებით არ დასტურდება.

## დამოუკიდებელი ვალიდაცია

- `cmake --build build --target check_build -j4` — exit **0**, build განახლებულია.
- მიმდინარე `ed2e86c8`-ზე სრული CTest — **44/44 passed**, exit **0**, 49.78 წმ. [სრული ლოგი](evidence/ctest-current.log). გამოტოვებულია მხოლოდ `ui_screenshot`, ლოკალური helper. ქსელისა და დროებითი GPG გარემოს საჭიროების გამო სრული ნაკრები გაშვებულია sandbox-ის გარეთ.
- მანამდე `7ce0cf18`-ზეც — **44/44 passed**. [პირველი გაშვების ლოგი](evidence/ctest-7ce0cf18.log).
- შვიდი diagnostic პროგრამა ხელახლა დაკომპილირდა მიმდინარე build-ის ბიბლიოთეკებთან; ყველა დასრულდა exit **0**-ით. ეს ნიშნავს, რომ სცენარები შესრულდა; პროდუქტის სისწორე ფასდება ლოგში ნაჩვენები მდგომარეობებით. მერვე, ცალკე გაშვებული crash probe, მოსალოდნელად დასრულდა **SIGABRT / -6**-ით.
- `git diff --check` — exit **0**. შემოწმების ბოლოს tracked ფაილები უცვლელია; დაემატა მხოლოდ ეს აუდიტის მასალები. [snapshot](evidence/snapshot.json), [ვალიდაციის ჩანაწერი](evidence/validation.json), [გარემო](evidence/environment.txt).

## პუნქტების მდგომარეობა

| პუნქტი | დასკვნა | დამოუკიდებლად დადასტურებული ქცევა / მტკიცებულება |
|---|---|---|
| N01 — submodule-ის embedded `.git` იკარგებოდა | დახურულია | deinit იყენებს native Git-ს და არქივდება `.git/modules/sm`; პროექტის regression ტესტი ამოწმებს უნიკალური commit-ის შენარჩუნებასაც. [probe](evidence/submodule_deinit.log) |
| N02 — stale amend ახალ ისტორიას ცვლიდა | დახურულია | `accepted=0`, ახალი tip შენარჩუნებულია. [probe](evidence/cas_review.log) |
| N03 — signing-ისას კონკურენტული ref იკარგებოდა | დახურულია | signing helper ცვლის tip-ს; commit უარყოფილია და კონკურენტული tip რჩება. [probe](evidence/cas_review.log) |
| N04 — უარყოფილი undo ფაილებს/index-ს ცვლიდა | **ნაწილობრივ; P2 რჩება** | სუფთა საწყისი მდგომარეობა აღდგება, მაგრამ წინასწარ staged სამიზნე-კონტენტი იკარგება. [საწყისი შემთხვევა](evidence/ref_remote_failures.log), [ახალი regression](evidence/rollback_regression.log) |
| N05 — stale remote-tracking ჩანაწერი push-ს აჩერებდა | დახურულია | სერვერზე წაშლილი branch რეალურად იგზავნება; ცრუ `Everything up-to-date` აღარ ჩანს. [probe](evidence/ref_remote_failures.log) |
| N06 — მთლიანად უარყოფილი push upstream-ს ნიშნავდა | დახურულია საწყის სცენარში | remote უცვლელია, upstream არ ინიშნება, rejection ლოგში ჩანს. შერეული წარმატების ზღვარი ქვემოთაა. [probe](evidence/ref_remote_failures.log) |
| N07 — fetch-ის Cancel GUI-ს აყოვნებდა | დახურულია შემოწმებულ Linux სცენარში | Cancel დაბრუნდა **0 ms**-ში, tracking ref არ განახლდა, ლოგში `Fetch canceled.`. სრული ნაკრების ახალი `transfer` ტესტიც გადის. [probe](evidence/ref_remote_failures.log) |
| N08 — პირველი commit-ის undo არ მუშაობდა | დახურულია | `undo=1`, branch unborn ხდება, ფაილი index-ში რჩება. [probe](evidence/ref_remote_failures.log) |
| N09 — submodule `a.b` სხვა `axb`-ს unregister-ს უკეთებდა | დახურულია | მხოლოდ არჩეული submodule-ის config იშლება; მეორე ინარჩუნებს config-სა და სამუშაო საქაღალდეს. [probe](evidence/submodule_regex.log) |
| N10 — ახალი CRLF ფაილის partial stage ატრიბუტს უგულებელყოფდა | დახურულია საწყის სცენარში | partial და სრული staging-ის blob-ებში LF-ია. [probe](evidence/partial_stage_crlf.log) |
| N11 — empty commit-ის Reword commit-ს შლიდა | დახურულია | შედეგი ახალი მოქმედი commit-ია, ახალი შეტყობინებითა და სწორი parent-ით. [probe](evidence/cas_review.log) |
| N12 — filter-ის repo-local override არ გამოიყენებოდა | დახურულია საწყის სცენარში | CLI და პროექტი ორივე `local`-ს იყენებს; rebase ინიციალიზდება და ტოვებს სწორ კონტენტსა და სუფთა სტატუსს. [probe](evidence/filter_config_scope.log) |

## F01 — [P2] უარყოფილი undo წინასწარ staged მდგომარეობას შლის

კოდი: [UndoHistory.cpp:237](/home/misha/repos/Gittyup/src/ui/UndoHistory.cpp:237), კონკრეტულად 237–238. შესწორება: `c44b21b9`.

Refs-ის აღდგენის ჩავარდნის შემდეგ rollback აკეთებს `checkout(from.headId(), ..., base=commit)`. ეს ძველი HEAD-ის tree-ს აღადგენს, თუმცა ოპერაციამდე index და სამუშაო ფაილი შეიძლება HEAD-ისგან განსხვავდებოდეს. თავდაპირველი index/worktree-ის snapshot არ ინახება.

გამეორების სცენარი რეალური `MainWindow` / `RepoView` / `UndoHistory` კოდით:

1. `topic` მიუთითებს A commit-ზე, მეორე branch — B-ზე. ჩაწერილია მეორე branch-იდან `topic`-ზე checkout-ის undo ისტორია.
2. მომხმარებელი `topic`-ზე ფაილს B კონტენტით ცვლის და staging-ს აკეთებს. ოპერაციამდე HEAD=A, index=B, სამუშაო ფაილი=B.
3. არსებობს `.git/HEAD.lock` — refs-ის ჩაწერა ვერ სრულდება.
4. Undo-ს საწყისი SAFE checkout B-ს იღებს, რადგან index უკვე სამიზნეს ემთხვევა. Ref-ის აღდგენა lock-ის გამო ვერ სრულდება.
5. ახალი rollback index-სა და სამუშაო ფაილს A-თი ცვლის. Undo აბრუნებს `false`-ს, HEAD ისევ `topic`-ზეა, მაგრამ წინასწარ staged ცვლილება გაქრა და სტატუსი სუფთაა.

```text
STAGED_ROLLBACK_NATIVE exit=128 head=topic file=B index=B status=M  file.txt
STAGED_ROLLBACK before_file=B before_index=B before_status=M  file.txt undo=0 head=topic after_file=A after_index=A after_status=
```

Native `git switch` კონტროლი ზუსტად ამ fixture-ში ინარჩუნებს B/B მდგომარეობას; probe ამოწმებს, რომ კონტროლს fixture არ შეუცვლია UI undo-ს გაშვებამდე. წყარო: [rollback_regression.cpp](probes/rollback_regression.cpp); [ლოგი](evidence/rollback_regression.log).

**გავლენა:** მომხმარებლის staged და სამუშაო მდგომარეობა იკარგება უარყოფილი ოპერაციისას. დადასტურებული შემთხვევაა index/worktree-ის კონტენტის სამიზნესთან დამთხვევა; თვითნებური unstaged ცვლილებების დაკარგვას არ ვამტკიცებ. B ამ სცენარში აღდგენადია სამიზნე commit-იდან, ამიტომ ეს მუდმივი object/history დაკარგვის მტკიცებულება არ არის.

**საჭირო შესწორება:** უარყოფისას აღდგეს ოპერაციამდე არსებული index და შესაცვლელი ფაილების რეალური კონტენტი/რეჟიმები, ან refs-ის ჩაწერის შესაძლებლობა უზრუნველყოფილი იყოს checkout-მდე. ძველი HEAD-ის checkout საკმარისი rollback არ არის. დაემატოს regression ტესტი წინასწარ staged B/B მდგომარეობით და `HEAD.lock`-ით.

Claude-ის ანგარიშის [61-ე სტრიქონი](/home/misha/repos/Gittyup/docs/git-audit-2026-10-30-fixes/README.md:61) ამბობს: „ადგილობრივი ცვლილებები შენარჩუნებულია.“ ეს მტკიცება ამ შემთხვევაში არ სრულდება; N04-ის სტატუსიც უნდა გახდეს „ნაწილობრივ გასწორებული“ სრულ შესწორებამდე.

## F02 — [P2] required clean filter-ის ჩავარდნისას partial staging Debug build-ს აჩერებს

ეს ცალკე აღმოჩენილი ხარვეზია; N10-ის საწყისი CRLF შემთხვევა გასწორებულია. Claude-ის ცვლილებით ამ crash-ის წარმოშობა **არ დასტურდება**.

რეგისტრირებული filter-ის კონფიგურაციაა `clean=false`, `smudge=cat`, `required=true`; ახალ ტექსტურ ფაილში ორი სტრიქონია. Native `git add file.txt` აბრუნებს **128**-ს და index-ს არ ცვლის. UI-ში ერთი სტრიქონის staging-ისას მიმდინარე Debug პროცესი სრულდება assertion-ით:

```text
native_add_exit=128 cached_paths=
cleanContent_fallback_hex=66697273740a7365636f6e640a
Assertion `start - 1 + numberLines <= image.length()' failed.
process_returncode=-6
```

მიზეზი: [Repository.cpp:1184](/home/misha/repos/Gittyup/src/git/Repository.cpp:1184)-ის `cleanContent()` filter-ის შეცდომისას აბრუნებს საწყის კონტენტს და diff stageable რჩება. შემდეგ `workdirId()`-ის ჩავარდნა აბრუნებს invalid ID-ს; [DiffModel.cpp:552](/home/misha/repos/Gittyup/src/ui/DiffModel.cpp:552)-ზე blob-ის ვალიდურობა არ მოწმდება, ხოლო 555–558-ზე ცარიელ preimage-ზე patch-ის გამოყენება [Patch.cpp:389](/home/misha/repos/Gittyup/src/git/Patch.cpp:389)-ის assertion-ს იწვევს.

**საჭირო შესწორება:** required filter-ის შეცდომა აშკარად გადაეცეს UI-ს; staging შეწყდეს filter/blob-ის ჩავარდნისას, index/worktree-ის ცვლილებისა და crash-ის გარეშე. დაემატოს რეალური partial-stage regression ტესტი ჩავარდნილი required filter-ით.

[მიმდინარე ლოგი](evidence/required_filter_partial-current.log), [probe](probes/required_filter_partial.cpp). დამატებითი კონტროლი ცვლის მხოლოდ `DiffModel.cpp`-ს `73fdd1d6`-ის ვერსიით, მიმდინარე headers/libraries-თან: იგივე crash მეორდება. ეს აჩვენებს, რომ ახალი untracked normalization ამ ჩავარდნისთვის აუცილებელი არ არის. **ეს hybrid კონტროლი სრული ისტორიული build არ არის**, ამიტომ მისგან მთელი ძველი ვერსიის ქცევის დასკვნა არ გამომაქვს. [შედარების მეტამონაცემები](evidence/required_filter_partial-comparison.json), [hybrid ლოგი](evidence/required_filter_partial-hybrid.log), [პორტატული recipe](probes/compare_old_model.py). Release build-ის ქცევა და მონაცემების დაკარგვა არ შემოწმებულა/დადასტურებულა.

## ანგარიშში სწორად აღწერილი საზღვრები

- **N06:** თუ branch იგზავნება, მაგრამ tag უარყოფილია, პროექტი branch-ის upstream-ს არ ნიშნავს. Native Git ნიშნავს. დამოუკიდებელმა [probe-მ](evidence/rollback_regression.log) ეს დაადასტურა; Claude-ის ანგარიში ამ შეზღუდვას უკვე უთითებს. ეს საწყისი მთლიანად უარყოფილი push-ის defect-ისგან განსხვავებული შემთხვევაა.
- **N02/N03:** signing-ისას HEAD-ის სხვა branch-ზე გადართვა იმავე OID-ით commit-ს ახალ მიმდინარე branch-ზე წერს. Native Git-ის კონტროლიც იმავე შედეგს იძლევა. [ლოგი](evidence/cas_review.log). ამის საფუძველზე დარჩენილ regression-ს არ ვაცხადებ.
- **N07:** შემოწმდა Linux fetch cancellation. Push cancellation და Windows პროცესის შეჩერება ამ შედეგებით არ დასტურდება; ანგარიშში ეს საზღვრები აღწერილია.
- **N12:** ზემოთ შემოწმებულია უკვე რეგისტრირებული filter-ის repo-local override. მხოლოდ repository-ში განსაზღვრული, გლობალურად არარეგისტრირებული driver-ის პრობლემა ანგარიშშიც ღიადაა აღწერილი; ეს ცალკე მხარდაჭერის ზღვარია.

## განმეორება

მიმდინარე HEAD და tracked diff უნდა ემთხვეოდეს [snapshot.json](evidence/snapshot.json)-ს; runner ძველ აუდიტს არ ცვლის და სხვა ვერსიაზე გაშვებას უარყოფს. საჭიროა Linux-ის უკვე აგებული Ninja/Debug build. ყველა Git ცვლილება სრულდება დროებით repository-ებში; binary/object/log არტეფაქტები იქმნება `/tmp`-ში.

```sh
cd /home/misha/repos/Gittyup
python3 docs/git-audit-2026-10-30-claude-review/run_probes.py
python3 docs/git-audit-2026-10-30-claude-review/run_probes.py --only required_filter_partial
python3 docs/git-audit-2026-10-30-claude-review/probes/compare_old_model.py
```

Runner-ის exit 0 მხოლოდ diagnostic გაშვების დასრულებას ნიშნავს. `required_filter_partial`-ზე runner განზრახ ელის SIGABRT-ს, რათა დადასტურებული defect დაფიქსირდეს; ეს პროდუქტის ტესტის წარმატება არ არის. ჩვეულებრივ გაშვებაში crash probe გამოტოვებულია. Git-ის production/test ფაილები ამ შემოწმებით არ შეცვლილა; Claude-ისა და თავდაპირველი აუდიტის ანგარიშები შენარჩუნებულია.
