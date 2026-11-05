# Git ოპერაციების ახალი სრული აუდიტი

**თარიღი:** 2026-10-30, Asia/Tbilisi. **პროექტი:** Gittyup-ng 0.0.6.
**შემოწმებული commit:** `73fdd1d6a418ffc69f70d7f0918617b4925cec39`.
შემოწმების დასაწყისში tracked სამუშაო ხე სუფთა იყო. წინა აუდიტების დასკვნები თავიდან გადამოწმდა; ძველი ლოგები მიმდინარე შედეგების ნაცვლად არ გამოყენებულა.

## ძირითადი შედეგი

მიმდინარე ვერსიაში დადასტურდა **12 ხარვეზი: 3 — P1, 9 — P2**. სამი P1 ეხება submodule-ის ლოკალური commit-ების ობიექტთა ბაზის წაშლასა და branch-ის ახალი ისტორიის გადაწერას. თითოეული ხარვეზი განმეორდა მიმდინარე ბიბლიოთეკებზე, დროებით რეპოზიტორიებში.

**არსებული 43 ტესტიდან საბოლოოდ 43-ვე წარმატებით დასრულდა:** 40 შეზღუდულ გარემოში, ხოლო ქსელის/GPG-ის გამო თავდაპირველად ჩავარდნილი სამი — შესაბამისი წვდომით განმეორებისას. ეს არ არის მტკიცებულება, რომ ყველა ოპერაცია სწორია: ქვემოთ დადასტურებული სცენარები არსებულ ტესტებში არ იყო დაფარული.

წინა ანგარიშების კონკრეტული გამოსწორებული სცენარები მიმდინარე ვერსიაში აღარ განმეორდა. ქვემოთ მოცემული N01–N12 ახალი სრული შემოწმების პუნქტებია და ძველი F01–F12-ის ნომრები არ არის. Upstream-only undo კვლავ განზრახ არ არის მხარდაჭერილი და დოკუმენტაცია ამას სწორად ასახავს; იგი 12 ხარვეზში არ შედის.

აპლიკაციის წყაროები, მომხმარებლის ტესტები და რეალური Git ისტორია არ შეცვლილა. დამატებულია მხოლოდ ეს ანგარიში, დიაგნოსტიკური პროგრამები და მტკიცებულებები.

| ID | პრიორიტეტი | პრობლემა | ძირითადი ადგილი |
| --- | --- | --- | --- |
| N01 | P1 | Submodule-ის deinitialize შლის embedded `.git`-სა და უნიკალურ ლოკალურ ისტორიას | `src/git/Submodule.cpp:45–48` |
| N02 | P1 | მოძველებული amend სამიზნე გადაწერს ახალ branch ისტორიას | `src/git/Commit.cpp:318–320` |
| N03 | P1 | ხელმოწერისას გადაადგილებული ref შემდეგ უპირობოდ გადაიწერება | `src/git/Signing.cpp:254–270` |
| N04 | P2 | Metadata lock-ის გამო უარყოფილი undo მაინც ცვლის ფაილებსა და index-ს | `src/ui/UndoHistory.cpp:215,233` |
| N05 | P2 | Cached tracking ref-ის მიხედვით push ქსელური შემოწმების გარეშე გამოტოვებულია | `src/ui/RepoView.cpp:1981–1984` |
| N06 | P2 | უარყოფილი push-ის შემდეგ მაინც ინიშნება upstream | `src/git/Remote.cpp:644–647` |
| N07 | P2 | Fetch-ის Cancel CLI პროცესს არ აჩერებს და GUI-ს ბლოკავს | `src/ui/RepoView.cpp:720–727` |
| N08 | P2 | პირველი commit-ის undo შეთავაზებულია, მაგრამ ვერ სრულდება | `src/git/RefState.cpp:239` |
| N09 | P2 | Submodule-ის სახელის regex სხვა submodule-საც აუქმებს | `src/git/Submodule.cpp:40–43` |
| N10 | P2 | Untracked CRLF ფაილის ნაწილობრივი staging უგულებელყოფს LF ნორმალიზაციას | `src/ui/DiffModel.cpp:145–154,543–556` |
| N11 | P2 | Empty commit-ის Reword მას ჩუმად შლის | `src/git/Rewrite.cpp:130–132` |
| N12 | P2 | libgit2 filter იყენებს გლობალურ ბრძანებას repo override-ის ნაცვლად | `src/git/Filter.cpp:129,169–184` |

P1 აღნიშნავს პირველ რიგში გამოსასწორებელ მონაცემთა/ისტორიის მთლიანობის პრობლემას; P2 — ფუნქციურ ხარვეზს კონკრეტულ პირობებში.

## დადასტურებული ხარვეზები

### N01 — Deinitialize შეიძლება committed ისტორიას შლიდეს [P1]

**პირობა:** submodule ძველი ფორმატისაა — `sm/.git` რეალური directory-ა და არა parent-ის `.git/modules/...`-ზე მიმთითებელი ფაილი. Child-ს აქვს ლოკალური commit, რომელიც მის origin-ზე არ არის. Parent-იც ამ commit-ს მიუთითებს, ამიტომ parent და child ორივე სუფთაა.

`Submodule::deinitialize()` child working directory-ს `removeRecursively()`-ით შლის (`Submodule.cpp:45–48`), რაც embedded ობიექტთა ბაზასაც წაშლის. UI-ს დადასტურება (`ConfigDialog.cpp:411–430`) working directory-სა და uncommitted ცვლილებებს აღწერს; ამ შემთხვევაში არც uncommitted ცვლილებაა და არც committed ისტორიის დაკარგვის ცალკე გაფრთხილება.

**შედეგი:** აღარ არსებობს არც `sm/.git`, არც parent-ის `.git/modules/sm`. უნიკალური commit-ის ობიექტი parent-სა და origin-შიც არ არის. იგივე სუფთა fixture-ზე native `git submodule deinit --force` ჯერ უსაფრთხოდ გადააქვს ბაზა `.git/modules/sm`-ში და commit-ს ინარჩუნებს.

**მტკიცებულება:** [submodule_deinit.log](evidence/submodule_deinit.log).

```text
native_deinit_exit=0 archived_commit=1 child_git_exists=0
project_deinit child_git_exists=0 archived_git_exists=0 parent_has_local_commit=0 parent_clean=1
```

**გამოსწორება/ტესტი:** გამოიყენოს Git-ის deinit/absorbgitdirs ლოგიკა ან წაშლამდე უსაფრთხოდ გადაიტანოს embedded repository. Regression-ში child-ის origin-ზე არარსებული commit უნდა გადარჩეს clean submodule-ის deinitialize-ს.

### N02 — Stale amend ახალ ისტორიას გადაწერს [P1]

**პირობა:** amend-ის სამიზნედ შენახულია commit A, შემდეგ branch წინ მიდის B-ზე ან მომხმარებელი სხვა branch-ზე გადადის. `AmendDialog` არამოდალურია და accepted callback ძველ commit-ს ინახავს (`RepoView.cpp:2566–2575`).

`Commit::amend` ქმნის replacement commit-ს captured commit-ის parent-ებით და შემდეგ მიმდინარე HEAD-ს expected-old შემოწმების გარეშე ცვლის (`Commit.cpp:318–320`). Probe-ში A იყო root commit, ამიტომ B-ის შემდეგ stale A-ს amend-მაც zero-parent replacement შექმნა. B branch-ის ისტორიაში აღარ არის. ობიექტი დაუყოვნებლივ ფიზიკურად არ იშლება; იკარგება მისი ადგილი branch-ის აქტიურ ისტორიაში.

**მტკიცებულება:** [commit_edges.log](evidence/commit_edges.log): `STALE_AMEND native=-1 project=1 head_parent_count=0 newer_in_history=0`. Bundled libgit2-ის `git_commit_amend(A,"HEAD")` იმავე stale მოთხოვნას უარყოფს. დიალოგის ხელით კლიკი არ შესრულებულა — API ქცევა დინამიკურად, UI-ს მისადგომი გზა კოდით დადასტურდა.

**გამოსწორება/ტესტი:** amend-ს ჰქონდეს intended ref-ის სახელი და expected original OID; ცვლილება შესრულდეს მხოლოდ მათი დამთხვევისას, ატომურად. ტესტმა უნდა შეცვალოს HEAD commit/branch გახსნილი amend-ის შემდეგ და შეამოწმოს უარი და არსებული ისტორიის შენარჩუნება.

### N03 — Signing-ისას გარე ref ცვლილება იკარგება [P1]

Unsigned commit-ის native libgit2 გზა parent/ref თანხვედრას ამოწმებს. Signed გზა კი ხელმოწერის შემდეგ იძახებს `Signing::updateReference`-ს, რომელიც symbolic HEAD-ს ხელახლა ხსნის და ref-ს უპირობოდ ცვლის (`Signing.cpp:254,262–270`). Signer-ზე ლოდინისას სხვა Git პროცესს შეუძლია branch-ის გადაადგილება.

**მტკიცებულება:** [commit_edges.log](evidence/commit_edges.log). გამოყენებულია ნამდვილი დროებითი ed25519 key და SSH signer wrapper, რომელიც ref-ს წინასწარ გადაადგილებს, შემდეგ ნამდვილ `ssh-keygen`-ს იძახებს. Native `git verify-commit` მიღებულ ხელმოწერას ამოწმებს, მაგრამ პროექტის commit concurrent commit-ს branch-ის ისტორიიდან მაინც აგდებს:

```text
SIGNING_CONCURRENT valid=1 signature=SSH verified=1 concurrent_in_history=0 signed_parent_is_base=1
SIGNING_CONCURRENT_NATIVE exit=128 head_is_concurrent=1
```

Native `git commit` იმავე wrapper-ით expected-old mismatch-ს ხედავს და გარე ref-ს ინარჩუნებს. Wrapper concurrency-ის მომენტს დეტერმინისტურად ქმნის; შედეგად ხელმოწერა რეალურია და პრობლემა signer-ის წარუმატებლობით არ აიხსნება.

**გამოსწორება/ტესტი:** signer-მდე დაფიქსირდეს intended ref და ძველი OID; ხელმოწერის შემდეგ გამოიყენოს ატომური compare-and-swap/transaction. შემოწმდეს symbolic HEAD-ის ცვლილებაც. Signed commit-ის ტესტმა signer-ის გაშვებისას ref გადაადგილოს და მოითხოვოს უარი.

### N04 — უარყოფილი undo partial checkout-ს ტოვებს [P2]

სუფთა `topic`/A მდგომარეობაში checkout-ის undo უნდა დაბრუნდეს სხვა branch-ის B commit-ზე. Probe ქმნის `HEAD.lock`-ს, როგორც სხვა Git პროცესის metadata lock-ს. `UndoHistory` ჯერ ცვლის worktree/index-ს (`UndoHistory.cpp:215`), შემდეგ HEAD-ის აღდგენა ვერ სრულდება (`233`). `RefState` rollback მხოლოდ refs-ს ეხება (`RefState.cpp:192`).

**შედეგი:** undo აბრუნებს `false`-ს, HEAD რჩება `topic`-ზე, მაგრამ ფაილი და ინდექსი უკვე B-ს შეიცავს და ჩნდება მოულოდნელი staged ცვლილება. [ref_remote_failures.log](evidence/ref_remote_failures.log): `HEAD_LOCK_UNDO before=A before_status= undo=0 head=topic file=B index=B status=M file.txt`.

**ზუსტი შეზღუდვა:** native `git switch`-საც აქვს partial checkout-ის იგივე შედეგი ამ lock-ის შემთხვევაში. Finding ეხება GUI undo-ს წარუმატებლობის შემდეგ index/worktree-ის აღდგენის არქონას; native Git-ის უკეთეს ქცევად არ არის წარმოდგენილი. Probe native კონტროლის შემდეგ ცალკე აბრუნებს სუფთა A fixture-ს და მხოლოდ მერე იძახებს UI undo-ს.

**გამოსწორება/ტესტი:** წინასწარი შემოწმების გარდა, late failure-ზე index/worktree-იც აღდგეს ან ოპერაცია ერთ დაცულ ტრანზაქციად შესრულდეს. Regression-მა metadata lock-ით გამოწვეული failure-ის შემდეგ საწყისი HEAD/content/index შეადაროს.

### N05 — Stale tracking ref push-ს ჩუმად გამოტოვებს [P2]

`RepoView.cpp:1981–1984` local ref-ს cached upstream ref-ს ადარებს და ტოლობისას ასრულებს `Everything up-to-date.`-ით, რეალურ remote-ს არ უკავშირდება.

**გამეორება:** გაკეთდეს tracked push; remote-ზე branch სხვა პროცესმა წაშალოს; აპლიკაციიდან შესრულდეს push ახალი fetch-ის გარეშე. პროექტი წარმატებულ შეტყობინებას წერს, remote branch კი არ აღდგება. Native push იმავე წყაროდან branch-ს თავიდან ქმნის.

**მტკიცებულება:** [ref_remote_failures.log](evidence/ref_remote_failures.log): `STALE_PUSH remote_branch_exists=0 logged_up_to_date=1`; native კონტროლი: `exit=0 remote_branch_exists=1`.

**გამოსწორება/ტესტი:** explicit push-ის შედეგი remote-ის რეალურ პასუხს დაეყრდნოს; cached tracking equality საბოლოო წარმატების დასადგენად არ გამოიყენოს. ტესტში remote-ის branch შეიცვალოს/წაიშალოს ბოლო fetch-ის შემდეგ.

### N06 — Push-ის უარყოფის შემდეგ upstream მაინც იცვლება [P2]

`Remote::push` porcelain-ში server rejection-ს პოულობს და callbacks-ში წერს, მაგრამ `rejected=true`-ისას nonzero exit-ს წარმატებულ `Result(0)`-ად ტოვებს (`Remote.cpp:624–647`). UI შემდეგ success path-ს ასრულებს და upstream-ს ნიშნავს (`RepoView.cpp:2065–2071`).

**მტკიცებულება:** local bare remote-ის pre-receive hook push-ს უარყოფს; remote უცვლელია, upstream კი previously-unset მდგომარეობიდან ინიშნება. Native `git push -u` იმავე hook-ით upstream-ს არ ცვლის. [ref_remote_failures.log](evidence/ref_remote_failures.log): `REJECTED_PUSH_NATIVE exit=1 upstream_set=0`; პროექტი: `remote_unchanged=1 upstream_set=1 logged_rejection=1`.

უარყოფა ლოგში **ჩანს** — finding არ ამტკიცებს, რომ შეცდომა დამალულია. პრობლემა არის უარყოფის შემდეგ წარმატებული ოპერაციის გვერდითი ეფექტი.

**გამოსწორება/ტესტი:** თითოეული მოთხოვნილი ref-ის წარმატება სწორად გადაიცეს UI-ში; უარყოფილი ref-ისთვის upstream არ დაინიშნოს. ტესტმა ერთდროულად შეამოწმოს remote OID, rejection log და unchanged tracking config.

### N07 — Fetch-ის Cancel CLI პროცესს არ აჩერებს [P2]

`cancelRemoteTransfer()` canceled flag-ს აყენებს და GUI thread-ზე `waitForFinished()`-ს იძახებს (`RepoView.cpp:720–727`). CLI fetch არ იღებს cancellation token-ს/process handle-ს; callbacks-ის transfer შემოწმება მხოლოდ დასრულების შემდეგ ხდება (`Remote.cpp:531–539`) და მისი `false` პასუხიც არ აჩერებს უკვე დასრულებულ პროცესს.

**მტკიცებულება:** local upload-pack helper იწყება და ობიექტების გაგზავნამდე 2 წამით ჩერდება. სწორედ ამ მომენტში სრულდება Cancel. მეთოდი დაახლოებით 2 წამს ბლოკავს, შემდეგ tracking refs ახლდება, ხოლო ლოგში წერია `Fetch canceled.`. [ref_remote_failures.log](evidence/ref_remote_failures.log): `cancel_wait_ms=1999 tracking_updated=1 logged_canceled=1`. დრო გაშვებებს შორის ოდნავ განსხვავდება.

**გამოსწორება/ტესტი:** CLI პროცესს ჰქონდეს რეალური შეწყვეტის მექანიზმი, GUI thread არ ელოდოს მას ბლოკირებით და cancelled/completed შედეგი რეალურ მდგომარეობას ასახავდეს. Regression გამოიყენოს კონტროლირებადი გაჩერებული local transport.

### N08 — პირველი commit-ის undo ვერ სრულდება [P2]

ცარიელ repository-ში პირველი commit-ის შემდეგ `UndoHistory` მოქმედებას სთავაზობს. მაგრამ ძველი unborn მდგომარეობის აღდგენისას `git_branch_delete` მიმდინარე HEAD branch-ის წაშლას უარყოფს (`RefState.cpp:239`), ამიტომ undo ვერ აბრუნებს repository-ს unborn მდგომარეობაში.

**მტკიცებულება:** [ref_remote_failures.log](evidence/ref_remote_failures.log): `FIRST_COMMIT_UNDO offered=1 undo=0 unborn=0`.

**გამოსწორება/ტესტი:** initial-commit undo-ს ჰქონდეს სპეციალური უსაფრთხო გზა, რომელიც HEAD-ს symbolic/unborn მდგომარეობაში ტოვებს და commit-ის შიგთავსს staged სახით ინარჩუნებს. ან მოქმედება არ შეთავაზდეს, თუ იგი განზრახ არ არის მხარდაჭერილი. ტესტი დაიწყოს სრულიად ცარიელი repository-ით.

### N09 — Submodule-ის regex სხვა სახელებსაც ემთხვევა [P2]

`Submodule.cpp:40–43` submodule-ის სახელს regex-ში escaping-ისა და მკაფიო საზღვრების გარეშე ჩასვამს. სწორი სახელები `a.b` და `axb` ორივე ემთხვევა `submodule.a.b.*` შაბლონს.

**მტკიცებულება:** [submodule_regex.log](evidence/submodule_regex.log). მხოლოდ `a.b`-ის deinitialize-ის შემდეგ ორივეს URL config აღარ არსებობს (`git config --get` exit 1), თუმცა `axb`-ის worktree დარჩენილია.

**გამოსწორება/ტესტი:** ზუსტი subsection-ის წაშლა ან regex-ის escaping/anchoring; უპირატესად Git-ის deinit ბრძანების გამოყენება. Regression მოიცავდეს ერთმანეთის მსგავს regex-მეტასიმბოლოებიან სახელებს.

### N10 — ნაწილობრივი staging CRLF ნორმალიზაციას არღვევს [P2]

`.gitattributes` შეიცავს `*.txt text eol=lf`-ს. ახალი untracked `file.txt` შეიცავს `first\r\nsecond\r\n`-ს. `DiffModel` untracked შიგთავსს raw სახით კითხულობს (`145–154`), ნაწილობრივ არჩეულ ხაზებს index content-ში აერთიანებს (`543–556`) და `Index::add`-ს აწვდის; `hash-object --stdin` path-ის შესაბამის clean conversion-ს არ ასრულებს (`Index.cpp:327–338`).

**მტკიცებულება:** რეალური `MainWindow` + `DiffModel::toggleLine` გზა first ხაზს CRLF-ით ინახავს: hex `66697273740d0a`. Native `git add` იგივე ფაილში LF-ს ინახავს: `66697273740a7365636f6e640a`. [partial_stage_crlf.log](evidence/partial_stage_crlf.log).

**გამოსწორება/ტესტი:** partial diff/reconstruction-ში content-ის canonical/clean ფორმა თანმიმდევრულად გამოიყენოს. ბოლო ნაბიჯზე filter-ის ბრმად დამატებამ უკვე ნორმალიზებული tracked buffers შეიძლება ორჯერ გაფილტროს. Regression მოიცავდეს untracked CRLF ტექსტსა და აქტიურ attributes-ს.

### N11 — Empty commit-ის Reword მას შლის [P2]

`Rewrite.cpp:130–132` non-Squash ნაბიჯს გამოტოვებს, თუ resulting tree წინა commit-ის tree-ს ემთხვევა. ეს განზრახ empty commit-ის Reword-საც ეხება, მიუხედავად იმისა, რომ მომხმარებელს Drop არ აურჩევია.

**მტკიცებულება:** base A-ს შემდეგ `--allow-empty` marker-ის Reword აბრუნებს თვით A-ს (`result_is_base=1`). Native interactive rebase ინარჩუნებს ერთ commit-ს ახალი `renamed marker` შეტყობინებით. [commit_edges.log](evidence/commit_edges.log): `EMPTY_REWORD valid=1 result_is_base=1`; native: `count=1 message=renamed marker`.

**გამოსწორება/ტესტი:** გაირჩეს თავდაპირველად empty commit და replay-ისას redundant გამხდარი patch. Reword-მა intentional empty commit შეინარჩუნოს. Regression შეამოწმოს commit count, parent და message.

### N12 — Repo-level filter override არ გამოიყენება [P2]

`Filter::init()` startup-ზე global clean/smudge/required კონფიგურაციას ინახავს (`Filter.cpp:169–184`), ხოლო თითოეული stream ამ cached ბრძანებას იყენებს (`129`), active repository-ის override-ს არ კითხულობს.

**გამეორება:** გლობალური filter `token`-ს `global`-ად გარდაქმნის; repository-ის იგივე filter override კი `local`-ად. Native `git cat-file --filters` აბრუნებს `local`-ს, პროექტის libgit2 filter — `global`-ს. CLI-სთვის ნამდვილად სუფთა worktree-ში repository-ის rebase უარყოფილია `unstaged changes exist in workdir`-ით; native rebase იმავე fixture-ზე წარმატებულია.

**მტკიცებულება:** [filter_config_scope.log](evidence/filter_config_scope.log):

```text
CLI_LOCAL_SMUDGE=local
PROJECT_SMUDGE=global
CLI_BEFORE_REBASE_STATUS=
PROJECT_REBASE_INIT_ERROR=1 state=0 error=unstaged changes exist in workdir
CLI_REBASE_EXIT=0
```

დადასტურებულია არასწორი გარდაქმნა და rebase-ის უსაფუძვლო უარყოფა; ფაილების დაზიანება ამ probe-ით არ დადასტურებულა.

**გამოსწორება/ტესტი:** effective clean/smudge/required კონფიგურაცია stream-ის active repository-დან წაიკითხოს; globally shared driver state repo-specific მნიშვნელობებით არ გადაიწეროს. Regression მოიცავდეს global და local override-ების განსხვავებას და ორ repository-ს განსხვავებული drivers-ით.

## შესრულებული შემოწმებები

### Build და არსებული ტესტები

გარემო: Linux/Fedora 44, Debug, Git 2.56.0, Qt 6.11.2, CMake 4.3.0, bundled libgit2. დეტალები: [environment.txt](evidence/environment.txt), [snapshot.json](evidence/snapshot.json).

```sh
cmake --build build --target check_build -j 4
ctest --test-dir build --output-on-failure --timeout 60 -j 1 -E '^ui_screenshot$'
```

Build წარმატებულია (`ninja: no work to do`). საწყის გაშვებაში 40/43 passed; ორი GitHub-dependent ტესტი DNS-ის/ქსელის გამო ვერ დასრულდა, GPG ტესტს კი agent-ის დაწყება გაუჭირდა: [ctest-sandbox.log](evidence/ctest-sandbox.log), [gpg-keygen-sandbox.log](evidence/gpg-keygen-sandbox.log).

სამი ტესტი ხელახლა გაეშვა ქსელისა და gpg-agent-ის შესაბამისი წვდომით:

```sh
ctest --test-dir build --output-on-failure --timeout 120 -j 1 \
  -R '^(branches_panel|referencelist|signing)$'
```

**სამივე წარმატებულია**, მათ შორის რეალური public GitHub fetch/clone და GPG keygen/signature verification. საბოლოო შედეგი: **43 განსხვავებული CTest target წარმატებულია**, ორი გაშვების ჯამური დაფარვით. [ctest-environment-retry.log](evidence/ctest-environment-retry.log).

`ui_screenshot` გამოტოვებულია როგორც ლოკალური screenshot helper. `config` target კონფიგურაციაში გამორთულია (`GITTYUP_CI_TESTS=OFF`); გლობალური პარამეტრების შემცვლელი რეჟიმი არ ჩართულა. ეს ორი პუნქტი 43-ის რაოდენობაში არ შედის.

### დამოუკიდებელი დიაგნოსტიკური პროგრამები

შენახულია 16 C++ source და 17 განსხვავებული probe გაშვების შედეგი: 11 ძველი სცენარების ხელახალი შემოწმებისთვის, 6 ახალი findings-ის/კონტროლების ჯგუფისთვის. თითოეული probe რამდენიმე შემთხვევას შეიძლება შეიცავდეს. ყველა საბოლოო პროგრამა process return code 0-ით დასრულდა. ეს ნიშნავს დიაგნოსტიკის დასრულებას და არა მასში ნაჩვენები ოპერაციების სისწორეს.

Probe-ები დაკომპილირდა იმავე პროექტის მიმდინარე ბიბლიოთეკებთან. გამოყენებულია მხოლოდ დროებითი repositories, local remotes/worktrees, კონტროლირებადი hooks/transports და დროებითი signing keys. UI-დამოკიდებული შემთხვევები იყენებს ნამდვილ `MainWindow`/`RepoView`/`UndoHistory`/`DiffModel` ობიექტებს offscreen რეჟიმში. AmendDialog-ის UI გზა სტატიკურად დადგენილია; API ქცევა დინამიკურადაა გამეორებული.

| Source | რას ამოწმებს | შედეგი |
| --- | --- | --- |
| [submodule_deinit.cpp](probes/submodule_deinit.cpp) | Embedded database და unique commit | [log](evidence/submodule_deinit.log) |
| [submodule_regex.cpp](probes/submodule_regex.cpp) | მსგავსი submodule სახელები | [log](evidence/submodule_regex.log) |
| [partial_stage_crlf.cpp](probes/partial_stage_crlf.cpp) | რეალური partial-stage UI გზა | [log](evidence/partial_stage_crlf.log) |
| [commit_edges.cpp](probes/commit_edges.cpp) | Stale amend, signed race, empty reword და native controls | [log](evidence/commit_edges.log) |
| [ref_remote_failures.cpp](probes/ref_remote_failures.cpp) | Undo failure/initial commit, stale/rejected push, cancellation | [log](evidence/ref_remote_failures.log) |
| [filter_config_scope.cpp](probes/filter_config_scope.cpp) | Effective local filter და rebase | [log](evidence/filter_config_scope.log) |

განმეორება, რეპოზიტორიის root-იდან:

```sh
python3 docs/git-audit-2026-10-30-full/run_probes.py
# მხოლოდ ახალი finding-ის კონკრეტული ჯგუფი:
python3 docs/git-audit-2026-10-30-full/run_probes.py --only commit_edges
```

Runner ამოწმებს შემოწმებული snapshot-ის commit-სა და tracked diff-ს; წყაროების შეუსაბამობისას ჩერდება. იღებს compile/link flags-ს არსებული Linux/Ninja build-ის `test_remotes` target-იდან. ბინარები და ახალი logs იწერება `/tmp/gittyup-audit-probes-*`-ში; პროგრამა საქაღალდის ზუსტ სახელს ბოლოს ბეჭდავს. წყაროებს, production tests-სა და რეალურ ისტორიას არ ცვლის. დროებითი commit hash-ები და cancellation-ის მილიწამები გაშვებებს შორის იცვლება.

## წინა ანგარიშების გამოსწორებების კონტროლი

| ძველი პუნქტი | მიმდინარე შედეგი |
| --- | --- |
| F01: untracked file overwrite | SAFE უარყოფილია; untracked content შენარჩუნებულია |
| F02: stale index write | გარე `git add`-ით staged ფაილი სხვა ფაილის stage-ის შემდეგაც რჩება |
| F03: cherry-pick Cancel/staged input | Staged ფაილიც და ignored submodule-იც ოპერაციას წინასწარ ბლოკავს |
| F04: მეორე merge მიმდინარე merge-ისას | მეორე merge უარყოფილია; არასწორი parent-ით commit აღარ სრულდება |
| F05: fetched tag-ის წაშლა undo-ით | მოგვიანებით მიღებული tag შენარჩუნებულია |
| F06: occupied worktree branch fast-forward | Ref-ის გადაადგილება უარყოფილია; ჩვეულებრივი occupied checkout-ის დაცვაც მოქმედებს |
| F07: missing conflict stages | Modify/delete და add/add სწორად კლასიფიცირდება |
| F08: სხვა remote-ის destination | Push იყენებს არჩეული remote-ისთვის სწორ branch სახელს |
| F09: upstream-only undo | განზრახ არ არის მხარდაჭერილი; README:62–63 სწორია; სხვა undo მოგვიანებით upstream-ს ინარჩუნებს |
| F10: rebase committer override | Override committer შენარჩუნებულია |
| F11: nested ref rename undo | ძველი სახელი აღდგება, undo წარმატებულია |
| F12: optional filter failure | Passthrough აბრუნებს საწყის content-ს, assertion აღარ ხდება |
| R01: undo სხვა worktree-ის დაკავებულ branch-ზე | Symbolic და detached HEAD რეჟიმებში უარისას ფაილი/index უცვლელია |
| R02: staged file/deleted directory SAFE checkout | სამივე სცენარი და staged rename source დაცულია |
| R03: ignored submodule staged guard | `pick=0`, staged gitlink შენარჩუნებულია |
| R04: upstream-only action | იგივე დოკუმენტირებული შეზღუდვა; სხვა მოქმედების undo-სგან config დაცულია |

კონტროლები: [baseline-run.log](evidence/baseline-run.log), [checkout_edges.log](evidence/checkout_edges.log), [rename_source.log](evidence/rename_source.log), [submodule_staging.log](evidence/submodule_staging.log), [undo_worktree.log](evidence/undo_worktree.log), [undo_worktree_detached.log](evidence/undo_worktree_detached.log), [upstream_only.log](evidence/upstream_only.log).

ძველი კონკრეტული worktree-preflight ხარვეზის გასწორება და ახალი N04 ერთმანეთს არ ეწინააღმდეგება: N04 არის მოგვიანებით, metadata write-ის ეტაპზე წარმოშობილი failure, რომლის შემდეგ checkout უკვე შესრულებულია.

## დაფარვა და პრაქტიკული საზღვრები

| სფერო | შესრულებული სამუშაო |
| --- | --- |
| Status/diff/path handling | Porcelain parsing, rename-source handling, literal pathspecs, safe directory-prefix checks; შესაბამისი ტესტები და დროებითი repo controls |
| Whole/partial staging, conflicts, modes | CLI/libgit2 index სინქრონიზაცია, add/reset, partial buffers, conflict-side მოქმედებები; normal tests და CRLF finding |
| Checkout/reset/worktrees | Branch/file checkout, detached HEAD, soft/mixed/hard/reset-merge გზების კოდის განხილვა; undo/reset/worktree tests და metadata-lock control |
| Commit/amend/signing | ჩვეულებრივი commit/amend, SSH/GPG signing/verification, stale amend და signed concurrency |
| Merge/rebase/revert/cherry-pick | Parents, state guard, conflicts, continue/abort, staged preconditions, override identity; არსებული tests და ძველი probe-ები |
| Interactive rewrite | Pick/reword/drop/reorder/squash/conflict გზები; არსებული tests და intentional empty commit კონტროლი |
| Undo/redo/refs | First commit, fetched tags, nested refs, later upstream, occupied worktrees და late failure |
| Branch/tag/remote CRUD | ჩვეულებრივი create/delete/rename/update; შესაბამისი tests, მეორე remote-ის push destination |
| Fetch/pull/push | Local remotes, pull fast-forward/divergence, public HTTPS fetch, stale tracking, rejecting server hook და cancellation |
| Clone | Public HTTPS clone არსებული `referencelist` ტესტით; clone-ის callbacks/credentials კოდის განხილვა |
| Stash | Push/apply/pop/drop და failure/notification გზების განხილვა; არსებული lifecycle/conflict tests |
| Submodules | Init/update/staging/settings/deinit; არსებული tests და embedded database/multiple-name probes |
| Configuration/filters | Scope/write/read კოდის განხილვა, optional/required filter tests, local override-ის რეალური rebase სცენარი |
| LFS | Wrapper/state/cache კოდის განხილვა; runtime არ შემოწმებულა, რადგან `git-lfs` გარემოში არ არის დაყენებული |

ეს არის მიმდინარე Linux Debug ვერსიის ფუნქციური/კოდის აუდიტი. Windows/macOS/Release runtime, SHA-256 repositories-ის ცალკე matrix, X.509 signing, private remote-ის რეალური SSH/OAuth ავტორიზაცია, LFS სერვერი და hooks-ის ყველა კომბინაცია არ შემოწმებულა. Public HTTPS წვდომის ტესტების წარმატება ამ განსხვავებულ გარემოებს არ ფარავს. ეს შეზღუდვები კონკრეტული დადასტურებული findings-ის მტკიცებულებას არ ცვლის.

რეკომენდებული სამუშაო რიგია N01/N02/N03, შემდეგ დანარჩენი findings-ის regression ტესტები და შესაბამისი შესწორებები. ოპერაციის `true/false` პასუხის გარდა ტესტებმა უნდა შეადაროს refs, reachable history, index bytes, worktree და config. სრული არსებული suite-ის წარმატება ახალი მონაცემთა-დაკარგვის სცენარების ტესტებს ვერ ანაცვლებს.
