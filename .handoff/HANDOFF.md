# Handoff: develop vs stable1.0 "dumb down" comparison (2026-10-15)

## Task (from user)
"What's the diff between branch develop and stable1.0? Scope was to dumb down the programs.
Compare the dumbed-down programs: do they work or not?"

## Branch topology (verified)
```
f9e27b5 fic  (merge-base of develop & stable1.0)
 |- develop (LOCAL) 63dfedf: bad7181 fix map creation -> 12757f5 "$0 % Complete" -> 63dfedf roles
 |- 11dae01 Removed Freebuff (= main)
     |- stable1.0 ebf8d3d: 322494c added sample codes -> ebf8d3d "Dumb Down The cpp"
     |- origin/Priyanshu caeab0e: Hash/HashMap simplification
origin/develop 23b3e88 = local develop + merge of origin/Priyanshu  (LOCAL develop is BEHIND origin)
```
The main checkout was on `develop` at the time; nothing in the repo was modified.

## What differs
- **Only on stable1.0 (the dumb-down, commit ebf8d3d):** rewrote all 7 `include/data_structures/*.hpp`
  (DynamicArray, Hash, HashMap, LinkedList, MinHeap, Queue, Stack). Removed: move ctors/assignment, `swap`,
  `at()`, `std::move`, SFINAE `enable_if` in Hash.hpp (now plain overloads: `string`, `long long`, `T*`),
  and all `throw std::out_of_range` on empty/out-of-range (now silent no-op or UB; callers must check `empty()`).
  Added `using namespace std;` to every ds header (leaks into every TU; style concern, no conflicts today).
  HashMap::remove now uses `removeValue(Pair{key, V()})` with `Pair::operator==` comparing keys.
  `tests/self_test.cpp`: move-ctor test replaced by "copy is independent". `MapGenerator.cpp`: added `#include <stdexcept>`.
  Plus `sample codes/` folder (student C/C++ files, .exe, .docx, .pdf; not part of the engine).
- **Only on develop:** ApiController (+.hpp) changes, `web/app.js`, `web/index.html`, `tests/web_test.cpp` (+10 checks),
  `maps/Floor12.json`, `Classes.png`, `roles.png`, `.freebuff/` re-added. develop does NOT have the dumb-down.
- Grep confirmed no engine/web code catches the removed ds exceptions (the existing `catch` blocks are for `stoi`).

## Results so far (all builds with D:\C\ucrt64\bin g++, build.bat, 0 warnings)
| Build | Compiles | mapgen_tests | mapgen_web_tests |
|---|---|---|---|
| stable1.0 (dumbed down) ebf8d3d | yes | 102 checks pass | 71 checks pass |
| pre-dumb-down 322494c | yes | 102 pass | 71 pass |
| local develop 63dfedf | yes | 102 pass | 81 pass |
| origin/develop 23b3e88 | **NO** | - | - |

origin/develop build error: Priyanshu's HashMap removed the `const` overload of `find()`, so
`CampusMap::searchByName` (src/CampusMap.cpp:148) and `buildPath` (src/Navigation.cpp:20) fail with
"passing const HashMap as this discards qualifiers". stable1.0's HashMap keeps const find(), so it is fine.
(`[login] FAILED user='user'` in web test output is a log line from a negative test, not a failure.)
Note: test counts differ from CLAUDE.md (says 91/41); CLAUDE.md is stale there.

## Unfinished step
Randomized differential test of every ds:: container vs std:: (DynamicArray/vector, LinkedList/list,
HashMap/unordered_map incl. rehash, int/string/pointer keys, MinHeap incl. decreaseKey, Queue, Stack, copy/self-assign).
Source saved at `.handoff/ds_fuzz.cpp`. User interrupted before it ran. To run (PowerShell):
```powershell
$env:PATH="D:\C\ucrt64\bin;$env:PATH"
git worktree add --detach ..\wt_stable stable1.0; git worktree add --detach ..\wt_predumb 322494c
g++ -std=c++17 -O2 -Wall -Wextra -I..\wt_stable\include  .handoff\ds_fuzz.cpp -o fuzz_new.exe; .\fuzz_new.exe
g++ -std=c++17 -O2 -Wall -Wextra -I..\wt_predumb\include .handoff\ds_fuzz.cpp -o fuzz_old.exe; .\fuzz_old.exe
```
Expect "DIFF OK (0 failures)" from both. Optional next: scripted console smoke test diffing `mapgen.exe` output old vs new.

## Cleanup needed
Temporary worktrees were registered under the old session scratchpad
(`...\Temp\claude\E--Projects-PBL\30d70445-...\scratchpad\wt\{stable,predumb,develop,odevelop}`).
Remove with `git worktree remove --force <path>` for each, or `git worktree prune` if the folder is already gone.

## Open recommendations to discuss with the team
1. Merging stable1.0 into origin/develop will conflict in Hash.hpp/HashMap.hpp (two independent simplifications);
   keep stable1.0's HashMap (it compiles) or re-add `const V* find(const K&) const` to Priyanshu's.
2. Run `git pull` on local develop before comparing/merging.
3. Consider dropping `using namespace std;` from headers, and keeping .exe/.docx/.pdf out of git (`sample codes/`).
