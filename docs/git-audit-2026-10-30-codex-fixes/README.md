# დარჩენილი ორი P2 ხარვეზის შესწორება

თარიღი: **2026-10-30**, Asia/Tbilisi. ბაზა: **`ed2e86c8c919f99fcbb2f78a8d953030d14e9569`**; შემოწმებისას შესწორებები სამუშაო ხეში იყო. შესწორებულია დამოუკიდებელი შემოწმების **F01/N04** და **F02**. ძველი ანგარიშები აღწერს იმდროინდელ ვერსიებს და შენარჩუნებულია.

**ორივე დადასტურებული ხარვეზი გასწორებულია. აპლიკაცია აიგო; სრული CTest: 44/44 passed, exit 0.**

## F01 / N04 — უარყოფილი undo ინარჩუნებს რეალურ ადგილობრივ მდგომარეობას

[UndoHistory.cpp](/home/misha/repos/Gittyup/src/ui/UndoHistory.cpp) ძველი HEAD-ის checkout-ის ნაცვლად იყენებს კერძო [CheckoutSnapshot.h](/home/misha/repos/Gittyup/src/ui/CheckoutSnapshot.h) helper-ს. Checkout-მდე ინახება index-ის ზუსტი ასლი და შესაცვლელი ფაილების რეალური კონტენტი, უფლებები, symlink-ის საწყისი target, საქაღალდეები და არყოფნის მდგომარეობა.

Refs-ის აღდგენის ჩავარდნისას ბრუნდება ეს ასლი. ამიტომ სამიზნესთან დამთხვეული staged კონტენტი აღარ იკარგება; ასევე ინარჩუნებს staged/unstaged კომბინაციასა და ადგილობრივ წაშლას. Recovery ამოწმებს checkout-ის შემდეგ დამატებით ცვლილებებს და იღებს index lock-ს. აღდგენის შეცდომა ჩანს ლოგში; დროებითი backup და manifest ინახება მითითებულ მისამართზე ხელით აღდგენისთვის.

Whole-tree [Repository::checkout](/home/misha/repos/Gittyup/src/git/Repository.cpp) აშკარად გადასცემს `--no-recurse-submodules`-ს. ეს შეესაბამება ამ API-ის child repository-ების შეუცვლელად დატოვების ქცევას და snapshot-ის საზღვარს, მაშინაც, როცა მომხმარებელს `submodule.recurse=true` აქვს. Initialized submodule-ის gitlink-ის დამატება/წაშლა/გადაადგილება მის შიგთავსს არ ცვლის; helper child `.git`-ს არ აკოპირებს და არ შლის.

დამატებული [undo regression ტესტები](/home/misha/repos/Gittyup/test/undo.cpp):

- `lockedHeadLocalState` — ოთხი შემთხვევა: სუფთა მდგომარეობა, staged სამიზნე B/B, staged B + unstaged C, ადგილობრივად წაშლილი საქაღალდე. ყველა fixture ამოწმებს ფაილი↔საქაღალდის გადასვლებს, ახალი parent-ის მოცილებას, კონტენტსა და index-ის ზუსტ bytes-ს; Linux-ზე ასევე executable რეჟიმსა და broken relative symlink-ს.
- `lockedHeadSubmodule` — initialized submodule-ის gitlink-ის დამატება და წაშლა, `submodule.recurse=true`-ით, `HEAD.lock`-ის გამო ჩავარდნა და lock-ის მოხსნის შემდეგ წარმატებული undo/redo; child repository უცვლელია.

ოთხივე ძირითადი regression ძველ UndoHistory-ზე ვარდებოდა: [before ლოგი](evidence/undo-before.log). შესწორების შემდეგ ყველა ახალი undo შემთხვევა გადის: [after ლოგი](evidence/undo-focused-after.log).

თავდაპირველი დამოუკიდებელი reproducer-ის შედეგი:

```text
STAGED_ROLLBACK before_file=B before_index=B before_status=M  file.txt undo=0 head=topic after_file=B after_index=B after_status=M  file.txt
process_returncode=0
```

[სრული reproducer ლოგი](evidence/undo_local_state.log), [წყარო](probes/undo_local_state.cpp).

## F02 — ჩავარდნილი required filter უსაფრთხოდ აჩერებს staging-ს

[Repository::cleanContent](/home/misha/repos/Gittyup/src/git/Repository.cpp) აბრუნებს `std::optional<QByteArray>`-ს: filter-ის შეცდომა წარმატებული/raw კონტენტისგან გამოყოფილია. [DiffModel](/home/misha/repos/Gittyup/src/ui/DiffModel.cpp) აჩვენებს შეცდომას და staging-ს ბლოკავს. Partial staging ასევე ამოწმებს working-copy ID/blob-ის ვალიდურობას patch-ის გამოყენებამდე, თუ filter preview-ის ჩატვირთვის შემდეგ იწყებს ჩავარდნას.

დამატებული [index regression ტესტი](/home/misha/repos/Gittyup/test/index.cpp), `stageLinesRejectsCleanFailure`, ამოწმებს ორივე დროს: preview-ის ჩატვირთვისას და უკვე ჩატვირთული preview-ის შემდეგ. ორივე შემთხვევაში index-ის ზუსტი bytes, სხვა staged ფაილი და სამუშაო ფაილი უცვლელია. Filter-ის გამოსწორების შემდეგ ახალი preview ჩვეულებრივ staging-ს აკეთებს. ტესტი პორტატულ, დროებით რეგისტრირებულ libgit2 driver-ს იყენებს.

ძველ კოდზე პირველი შემთხვევა ვარდებოდა, მეორე SIGABRT-ით მთავრდებოდა: [before loading](evidence/before-when-loading.log), [before late failure](evidence/before-after-loading.log). შესწორების შემდეგ index-ის მთელი ნაკრები **10/10** გადის, მათ შორის თავდაპირველი CRLF კონტროლი: [ლოგი](evidence/index-after.log).

რეალური გარე `clean=false`, `required=true` driver-ით თავდაპირველი reproducer-იც ხელახლა დაკომპილირდა და გაეშვა. მხოლოდ diagnostic წყაროა მორგებული ახალ optional API-ზე. შედეგი: UI არ არის editable, შეცდომა ხილულია, staging ნორმალურად ბრუნდება და index-ში raw blob არ იქმნება:

```text
cleanContent_has_result=0
editable_before_attempt=0 notice=Cannot stage this file: Unable to filter the working copy.
stage_attempt_returned=1 cached_blob_exit=128 cached_blob_hex=
process_returncode=0
```

[სრული ლოგი](evidence/required_filter_partial.log), [წყარო](probes/required_filter_partial.cpp).

## საბოლოო ვალიდაცია და განმეორება

| შემოწმება | შედეგი |
|---|---|
| `cmake --build build --target check_build -j4` | exit 0 |
| `cmake --build build --target gittyup -j4` | exit 0; აგებულია `build/gittyup-ng` |
| სრული CTest, `ui_screenshot` helper-ის გარეშე | **44/44 passed**, exit 0, 51.93 წმ — [ლოგი](evidence/ctest.log) |
| ორი დამოუკიდებელი reproducer | ორივე exit 0; სწორი index/worktree მდგომარეობა |
| `git diff --check` | exit 0 |

შენახულია [ვალიდაციის ჩანაწერი](evidence/validation.json), [კოდის snapshot](evidence/snapshot.json) და [სრული patch](evidence/patch.diff), ახალი helper-ის ჩათვლით; patch კონტექსტის გარეშეა (`git diff --unified=0`). ტესტირება ჩატარდა Linux Debug build-ზე. Windows/macOS runtime ამ შემოწმებაში არ გაშვებულა. Recovery ვერ იძლევა წარმატების გარანტიას filesystem/index-lock შეცდომისას; ასეთ შემთხვევაში backup ინახება და ლოგი მის მისამართს აჩვენებს.

```sh
cd /home/misha/repos/Gittyup
cmake --build build --target check_build gittyup -j4
ctest --test-dir build --output-on-failure --timeout 120 -j1 -E '^ui_screenshot$'
python3 docs/git-audit-2026-10-30-codex-fixes/run_probes.py
```

Diagnostic runner ამოწმებს საწყისი workspace-ის snapshot-ს ან დაკომიტებული `src`/`test` tree-ების hash-ებს, build ფაილებსა და ახალი helper-ის SHA-256-ს; ყველა Git mutation დროებით repository-ებში ხდება. Exit 0 აღნიშნავს სცენარების შესრულებას — პროდუქტის სწორი მდგომარეობა დასტურდება ზემოთ მოყვანილი ლოგებითა და regression assertions-ით. კოდი და regression ტესტები შესრულებულია OpenAI Codex-ის მიერ მომხმარებლის მოთხოვნით.
