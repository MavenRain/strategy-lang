# strategy-lang status

> Write here: one paragraph. Say which milestone the build implements, and
> which milestone it does not complete yet.

## Implemented

> Write here: one item for each feature that works. Name the former (F1 to
> F15), the core type or the core operation.

- <feature>.

## Remaining M0 work

> Write here: one numbered item for each missing feature. Say what each item
> needs first.

1. <work item>.

## Known limits

> Write here: one item for each limit of the current build, with its cause
> (a fuel budget, a host limit, a stack size).

- <limit>.

## Internal boundaries

> Write here: one paragraph for each module group. Say which module holds
> which job, which tests check which tables, and which types a program
> cannot use.

## S1 proofs (no collapse)

S1 keeps each proof as data (`docs/PLAN.md` section 3). The audit ran at
`efdb21d`. The line numbers refer to that commit.

Audit:

- F1 `src/front/eval.c:682-690` `reduce_proof`: `symm`, `trans`, `cong`
  and `transport` computed on a proof that is not `refl`. FIXED: a proof
  argument that is not `refl` stops the operation.
- F2 `eval.c:941-985` `conv_loop` is untyped: function eta (944-945),
  `Prod` and `Sigma` eta (946-949), `conv_pair` (931-937). It has no `Eq`
  eta, no UIP and no K. Held. Tests: `proof-uip`, `proof-k`, `proof-eta`.
- F3 `eval.c:957-958`: two traps are equal in conversion. This is not a
  proof rule. `src/json.c:77-78` refuses traps. Held.
- F4 `eval.c:974-975` and `check.c:547-556`: `refl` is the only proof
  constructor. Held.
- F5 `check.c:788-846`: `symm`, `trans`, `cong` and `transport` have
  typing rules only, with no other equations. Held. Tests:
  `proof-inverse`, `proof-symm-symm`, `proof-trans-refl`, `proof-cong-id`.
- F6 `check.c:1429-1430`: axioms are refused. There is no `Prop`. Held.
- F7 `check.c:488`: `Eq A x y` is in `Type 0` for each level of `A`. OPEN
  (`docs/PLAN.md` O10, S2).
- F8 `json.c:183-184`: an `Eq` value was written as `null`. FIXED: it is
  `{"refl": v}`.
- F9 `json.c:106`: `some` wrapped an `Eq` value. FIXED: `some` wraps an
  `Option` value only.
- F10 `json.c:285-305`: `Eq` defs were not instances. FIXED: each `Eq` def
  is an instance.

Changes:

- `src/front/eval.c` `reduce_proof`: the J rule only. A proof argument
  that is not `refl` gives a stuck operation, so the operation stays a
  proof term.
- `src/json.c` `refl_json`: an `Eq` value is `{"refl": v}`, with `v` the
  point as a value of the carrier. When the carrier is a function type or
  a type, `v` is the normal form of the point in source syntax, as a
  string (`docs/PLAN.md` O8).
- `src/json.c` `option_json`: `some v` is `{"some": v}` only when `v` is
  an `Option` value.
- `src/json.c` `is_instance`: each program def is an instance, but not a
  function or a type.
- Tests: 7 refusal tests `test/check/proof-*.lang` (each refused with
  `TYPE_REFL`), `examples/proofs.lang` with `test/json/proofs.json` (16
  instances), and the new `Eq` instances in `test/json/equality.json`,
  `laws.json` and `sigma.json`. `test/printer-regressions.js` expects
  `{"refl": 0}` as the innermost payload.

Deferred:

- Stuck proof trees in the JSON output (`docs/PLAN.md` O9, S2).
- F7, the level of `Eq` (`docs/PLAN.md` O10, S2).
- A test that kills mutant M6. S2 adds it, because its path constructors
  make a stuck proof operation reachable.

Gate (`make -B check`): examples 11 checked, 0 failed; parse refusals 9;
check refusals 31; evals 25; json 11 builds; build refusals 2; gate 0
failures; regressions 6 checked, 0 failed; printer regressions 2 checked.
The clang `-Wswitch-enum` build is clean.

Mutants (one `make -B check` each, source restored after each):

| Mutant | Result | Killed by |
|---|---|---|
| M1 an `Eq` value written as `null` | KILLED | `equality.json`, `laws.json` |
| M2 `is_instance` gives 0 for `Eq` | KILLED | `equality.json`, `laws.json` |
| M3 `Eq` back in the `some` wrap | KILLED | `proofs.json` |
| M4 conversion accepts each pair with a `refl` side | KILLED | `proof-k`, `proof-eta` |
| M5 `trans p refl` reduces to `p` | KILLED | `proof-trans-refl` |
| M6 F1 reverted | SURVIVED (expected) | no S1 test |
| M7 the printed point written as `""` | KILLED | `laws.json`, `proofs.json` |

## S2 HITs and quotients

S2 adds higher inductive types (HITs) to the domain file (`docs/PLAN.md`
section 4). The S2 code is commit `37f8808`. Session 6 built the S2 tests
and ran them on the session 5 kernel. No kernel fix was necessary for the
S2 tests in session 6. After session 6, `37f8808` added two kernel
changes: a stuck `cong` keeps its carrier, and the JSON key `path` is
reserved.

Rulings (each one the default (a), 2026-10-13):

- R1: a HIT is in `domain/domain.lang` only.
- R2: the fold is not dependent. It has one case for each point
  constructor and one case for each path constructor. Dependent
  elimination is open (`docs/PLAN.md` O11).
- R3: the fields of a path constructor are a telescope.
- R4: 1-paths only. No path field mentions the family. A truncation is a
  build refusal, `DOMAIN_TRUNCATION`.
- R5: the JSON writes a proof as a structured tree.
- R6: `Eq A x y` is at the level of `A`.
- R7: a stuck term that is not a proof is refused (`JSON_VALUE`).
- R8: `cong` computes on `fun x => fold cases x` applied to a path
  constructor. The result is the path case.
- R9: `Pth` is in S4.

The point-term rule (fixed, not a ruling): each end of a path constructor
is built from point constructors only. Thus the fold over an end reads
only the point cases.

Changes (the session 5 names first, then the `37f8808` additions):

- `src/front/ast.h`, `src/front/parser.c`: a constructor with `: T` after
  its fields is a path constructor (`Ctor.path`).
- `src/front/core.h`: `CtorInfo` has `is_path`, `lhs` and `rhs`. The
  family has `path_count`.
- `src/front/check.c`: `check_ctor` pushes each path field (the
  telescope), calls `check_path`, then pops the fields. `check_path`
  makes sure that the type is `Eq F a b`, that each end is a point term
  (`point_term`). `check_field` has a fifth argument, `path`, and refuses
  a path field that mentions the family (`DOMAIN_TRUNCATION`).
  `path_case_type` gives the type of a path case. `fold_family` checks the
  cases in two passes: the point cases first, then the path cases.
  `rule_fold` refuses a missing path case (`FOLD_PATH_CASE`). `ctor_app`
  makes the telescope environment. `rule_eq` puts `Eq A x y` at the level
  of `A` (R6, F7).
- `src/front/eval.c`: `reduce_cong` follows `reduce_proof`. It computes
  `cong (fun x => fold cases x) c`, with `c` a path constructor, to the
  path case (`cong_path`, `core_mentions`). For each other `cong`, it
  uses the J rule.
- `src/json.c`: `proof_json` writes a proof as a tree (`proof_tree`,
  `path_json`, `trans_json`), with the `JSON_DEPTH` guard. `proof_ends`,
  `path_ends`, `trans_ends` and `image_ends` (`cong`, `transport`) find
  the ends of each subproof, so each `refl` leaf gets its point
  (`refl_json(o, carrier, point)`, `printed_json`). `op_json` `OP_EQ`
  gets the ends from the type. `value_json` accepts the supported stuck
  proof forms (`stuck_proof`) and refuses other stuck terms (R7 and the
  implementation limit in (e) below). `is_enum` skips
  the path constructors. `path_ctor` finds the constructor of a path.
- `domain/domain.lang`: `Circle` (`base`, `loop`), `Interval` (`zero`,
  `one`, `seg`) and `Quot A R` (`class`, `glue`).
- `Makefile`: the pattern rule `build/langc-%` builds a compiler with the
  domain `test/emit/%.domain`. `test/gate.sh` has a `*.domain` arm for
  the build refusals.
- `37f8808`, carrier: `Core` and `Value` have `cong_carrier`.
  `reduce_op` gets the environment and the carrier, and `reduce_cong`
  keeps the result carrier on a stuck `cong`. `quote_value` keeps the
  carrier in the quoted core. Thus a nested proof keeps its types after
  normalization.
- `37f8808`, `check_ctor`: a path field named `path` is refused
  (`DOMAIN_FAMILY`), because `path` is the JSON key of the path
  constructor name.
- `37f8808`, `Makefile`: the target `build/proof-regressions`. The clang
  check also compiles `test/proof-regressions.c`.
- Tests: `examples/paths.lang` with `test/json/paths.json` (15
  instances); 3 evals (`congFold`, `loopBack`, `flipEnd`); 6 check
  refusals: `path-missing-case` (`FOLD_PATH_CASE`), `path-case-type` and
  `eq-level` (`TYPE_MISMATCH`), `path-symm`, `path-not-refl` and
  `path-cong-stuck` (`TYPE_REFL`); `test/emit/truncation.domain`
  (`DOMAIN_TRUNCATION`). The def `loop` in `test/check/self.lang` and
  `rec.lang` is now `spin`, because `loop` is a domain constructor.
- `37f8808`, tests: `test/emit/path-field.domain` (`DOMAIN_FAMILY`) and
  `test/proof-regressions.c`. It has 9 checks of the JSON of a `refl`
  leaf under `cong`: 7 carrier kinds (a family, `Nat`, a proof, `Option`,
  `List`, a function, a type), a carrier that is a bound type variable,
  and a fold path case.

S2 implements the listed stuck proof trees (O9), fixes F7 (O10, the level
of `Eq`) and adds the test that kills M6. Other stuck proof forms remain
unsupported, as noted in (e).

Deferred:

- (a) A `refl` leaf under `cong` had no known carrier. RESOLVED by
  `37f8808`: a stuck `cong` keeps its carrier.
  `test/proof-regressions.c` expects `{"refl": ...}` under `cong`.
- (b) A path constructor field named `path` clashed with the JSON key
  `path`. RESOLVED by `37f8808` as a build refusal:
  `test/emit/path-field.domain` gives `DOMAIN_FAMILY`.
- (c) Dependent elimination of HITs. OPEN (`docs/PLAN.md` O11, R2).
- (d) `trans` with a bare `refl` operand is refused with `TYPE_INFER`,
  because the S1 rule `rule_trans` infers the type of both proofs, and a
  bare `refl` has no inferred type. OPEN. `examples/paths.lang` names the
  proof `baseRefl` and writes `reflLoop` as `trans baseRefl loop`.
- (e) Proofs outside the supported tree forms remain unsupported (O9).
  In particular, a fold blocked on a transported point may have an `Eq`
  type but is still refused with `JSON_VALUE`. The reproducible example
  is in `docs/host/README.md`.

Gate (`make -B check`), S1 to S2: examples 11 to 12 checked, 0 failed;
parse refusals 9 to 9; check refusals 31 to 37; evals 25 to 28; json 11
to 12 builds; build refusals 2 to 4; gate 0 failures; regressions 6
checked, 0 failed; printer regressions 2 checked; proof regressions (new)
9 checked, 0 failed. The clang `-Wswitch-enum` build is clean. It also
compiles `test/proof-regressions.c`.

Mutants (session 6, one `make -B check` each, source restored after each,
`shasum -c` OK at the end). Each mutant is a semantic form, so tcc and
clang `-Werror` compile it, and each kill is a test. The 6 mutants ran on
the session 6 tree, before the `37f8808` kernel edits (the `cong` carrier
and the `path` key). They were not run again on `37f8808`. M6 survived
in S1 (no S1 test), and S2 kills it.

| Mutant | Form | Result | Killed by |
|---|---|---|---|
| M6 | `return result;` first in reduce_proof | KILLED | path-symm, path-cong-stuck, proof-inverse (exit 0, TYPE_REFL wanted); eval loopBack gives refl; paths.json |
| M8 | rule_eq `val_univ(c->m, 0)` | KILLED | eq-level (exit 0) |
| M9 | check_field `0 && out->recursive && path` | KILLED | truncation.domain build refusal (exit 1, another code) |
| M10 | reduce_cong `0 && cong_path(...)` | KILLED | eval congFold gives `cong (fun c => fold base loop c) loop`; paths.json |
| M11 | rule_fold `0 &&` on the FOLD_PATH_CASE guard | KILLED | path-missing-case (gets TYPE_MISMATCH) |
| M12 | pass 1 infers the case, accepts any Eq (`(void)path_case_type;`) | KILLED | path-case-type (exit 0); also quotFold TYPE_INFER, 3 evals, paths.json |

## S3 enumerations and decisions

S3 adds enumerations to programs, a case count check for `fold`, a
dependent eliminator for enumerations, the empty type, refutations and
JSON tables (`docs/PLAN.md` section 5). The base is `a9edaf3`. Session 10
built the kernel. Session 11 built the tests and ran the mutants. Session
12 wrote the docs.

Facts at `a9edaf3` (the S3 kickoff, section 0):

- Q1: the parser accepts `family` in each file. The checker refused it in
  a program (`REFUSE_DATA`, rule R1). The origin (`ORIGIN_DOMAIN` or
  `ORIGIN_PROGRAM`) was the only difference. `test/check/data.lang` was an
  enumeration, so S3 had to change it.
- Q2: `fold` was the only eliminator. Its cases are positional. A wrong
  case count on a family with no paths gave the general `TYPE_MISMATCH`.
  Two swapped cases of one type pass.
- Q3: no empty type existed. Large elimination and no-confusion with
  `Eq Nat 0 1` worked. `fold` is not dependent, so `decEq x y` did not
  check (`TYPE_REFL`).
- Q4: a function def was not an instance. A function inside data was
  refused (`JSON_VALUE`). Thus a decision with an `inr` side could not be
  an instance. `is_enum` in `src/json.c` gives 1 for `Circle`, so a table
  test must also require no path constructor.
- Q5: the no-collapse risks. Risk 1: a `Flag` decision has no evidence.
  S3 adds no `Flag` decision primitive. Risk 2: the writer must not erase
  a refutation. Risk 3: a table on a HIT drops the path images. Risk 4: a
  stuck `elim` prints as `fold`. Risk 5: S3 adds no conversion rule.
- Q6: one global namespace. A user constructor cannot use a domain name, a
  builtin name or a name in the program again (`REFUSE_NAME`). `name_owner`
  wrote "a domain constructor" for each constructor.

Rulings (each one the default (a), USER, 2026-10-13):

- R1: a program can declare an enumeration only: no parameters, and each
  constructor has no fields and no path type. Other program families keep
  `REFUSE_DATA`.
- R2: the positional `fold` stays. A wrong case count on a family gives
  `FOLD_CASES`. The message gives the expected count and the constructor
  names in order. `FOLD_PATH_CASE` stays for the S2 shape.
- R3: a dependent eliminator for enumerations only,
  `elim P c1 ... cn x : P x`, with `P : E -> Type l` and case `i : P ci`.
  The core term is `OP_FOLD_FAMILY` without `P`, so eval and JSON do not
  change. `decEq` is user code: an `elim` in an `elim`, with a large fold
  `code` for the refutation (`transport (code x) e unit`).
- R4: the builtins `Empty : Type 0` and `absurd : Empty -> A`. `absurd`
  takes `A` from the hint, else `TYPE_INFER`. It never reduces. `Not A` is
  written `A -> Empty`.
- R5: a function of type `A -> Empty` (codomain `Empty`, not dependent) is
  written `{"refute": "<normal form in source syntax>"}`. A def of such a
  type is an instance.
- R6: a function on an enumeration is a JSON table: one key for each
  constructor, in declaration order. A dependent codomain is permitted. A
  function on an enumeration in a function on an enumeration gives nested
  tables. A def is an instance when its type is a chain of Pi over
  enumerations that ends in a type that is not a Pi and not a universe.
- R7: a function on a family with a path constructor (`Circle`) is not a
  table. It is not an instance at the top level, and it is `JSON_VALUE`
  inside data.
- R8: the one global namespace stays (`REFUSE_NAME`). `name_owner` writes
  "a constructor" and "a family". No new JSON reservation.
- R9: the `langc eval` arguments do not change. `entry_arg` takes a `Nat`
  or a `Flag`.

Changes:

- `src/front/core.h`: `OP_EMPTY` (after `OP_EQ`) and `OP_ABSURD` (after
  `OP_PROJ`). The declaration of `is_enumeration`.
- `src/front/eval.c`: the name and arity rows for `Empty` and `absurd`.
  `OP_EMPTY` is a type former. `OP_ABSURD` is always stuck.
- `src/front/check.c`: the rules `RULE_ELIM` and `RULE_ABSURD`. The
  builtins `Empty`, `elim` and `absurd`. `fold_cases` gives `FOLD_CASES`.
  `is_enumeration` gives 0 for a family with parameters or path
  constructors, or with a constructor that has fields. `rule_fold` calls
  `fold_cases` on a family after the over-applied simple fold path, so
  that path does not change. `rule_elim` refuses a scrutinee that is not
  an enumeration (`ELIM_ENUM`). It checks the case count (`FOLD_CASES`)
  and the motive (a Pi to a universe, on the scrutinee type), and it
  checks case `j` at `P` applied to constructor `j`. `rule_absurd` needs a
  hint and checks the argument at `Empty`. `name_owner` writes "a family"
  and "a constructor" (R8). `enum_decl` and `check_decl` refuse a program
  family that is not an enumeration (`REFUSE_DATA`).
- `src/json.c`: `table_type`, `refute_type`, `table_json`, `refute_json`
  and `pi_json`. The `VAL_PI` arm of `typed_json` calls `pi_json`: a
  table, else a refutation, else `JSON_VALUE`. `value_json` does not call
  `ready` for a table or a refutation. `pi_instance` follows the tables to
  the end type. `is_instance` takes the machine and calls `pi_instance`
  for a Pi type. `op_json` refuses `OP_EMPTY`.
- `test/check/data.lang` has a field now (`tails (bias : Nat)`), so it
  still gives `REFUSE_DATA`.
- 12 new check refusals: `enum-path` and `enum-param` (`REFUSE_DATA`),
  `enum-name` and `enum-dup` (`REFUSE_NAME`), `fold-missing` and
  `fold-extra` (`FOLD_CASES`), `elim-case-type` and `elim-motive`
  (`TYPE_MISMATCH`), `elim-not-enum` (`ELIM_ENUM`), `absurd-hint`
  (`TYPE_INFER`), `absurd-arg` (`TYPE_MISMATCH`) and `dec-wrong`
  (`TYPE_REFL`).
- `examples/enums.lang` with `test/json/enums.json`: the enumeration
  `Phase`, the tables `next` and `gap` (nested), the large fold `code`
  (not an instance), `decEq` (an `elim` in an `elim`), the refutation
  `dawnNotDusk`, the `absurd` use `dawnDuskNat` (not an instance) and the
  decisions `decNoon` and `decDawnDusk`. Each line of `enums.json` was
  checked by hand against R5 and R6.
- `test/json/algebra.json`: `colorCode` is now a table instance,
  `{"red":1,"green":2,"blue":3}`. No other line changed.
- 4 evals: `afterDusk`, `duskCode`, `decNoon` and `dawnIsDusk`. A
  top-level `Flag` prints as 0 or 1.
- 2 emit refusals (`JSON_VALUE`): `circle-table.lang` (a function on
  `Circle`, R7) and `table-fn.lang` (a table entry that holds a function).
- `test/proof-regressions.c`: `check_enums` adds checks 10 and 11. Check
  10: the value of a refutation starts with `{"inr":{"refute":"` and the
  string is not empty. Check 11: an `elim` on a variable does not convert
  to a constant function.

Two tests differ from the kickoff section 3 text. No kernel change and no
gate target change followed from them:

- `fold-extra.lang` uses `Flag` cases,
  `fold flagYes flagNo flagYes flagNo c` at `(c : Color) -> Flag`.
  Reason: with `Nat` cases, `fold 1 2 3 4 c` goes to the over-applied
  `Nat` fold path (a fold over `Nat` at `3`, then applied). That path
  gives `TYPE_MISMATCH`, not `FOLD_CASES`.
- `absurd-hint.lang` uses
  `either (fun (n : Nat) => n) (fun (n : Nat) => n) (absurd e)`. Reason:
  `fold 1 2 3 (absurd e)` also goes to the over-applied `Nat` fold path.

Name note: the kickoff section 3 calls the decision instance `decPhase`.
R3 (a) and R9 (a) call it `decEq`. In `examples/enums.lang` the one def
`decEq` does both jobs.

Deferred:

- S2 (c), dependent elimination of HITs: OPEN, no change (`docs/PLAN.md`
  O11). `elim` is for enumerations only.
- S2 (d), `trans` with a bare `refl` operand: OPEN, no change.
- New open items: O12 (a stuck `elim` prints as `fold`), O13 (enumerations
  with parameters) and O14 (swapped positional cases), in `docs/PLAN.md`
  section 6.

Gate (`make -B check`), S2 to S3: examples 12 to 13 checked, 0 failed;
parse refusals 9 to 9; check refusals 37 to 49; evals 28 to 32; json 12
to 13 builds; build refusals 4 to 6; gate 0 failures; regressions 6
checked, 0 failed; printer regressions 2 checked; proof regressions 9 to
11 checked, 0 failed. The clang `-Wswitch-enum` build is clean.

Mutants (session 11, one `make -B check` each, source restored after
each, `shasum` OK after each restore). Each mutant is a semantic form, so
tcc and clang `-Werror` compile it, and each kill is a test. The final
gate after the mutants was GREEN at the same counts.

| Mutant | Form | Result | Killed by |
|---|---|---|---|
| M13 | check_decl `d->origin == ORIGIN_PROGRAM && 0 && !enum_decl(d)` | KILLED | data.lang, enum-path, enum-param |
| M14 | enum_decl `(0 && d->ctors[j].path != NULL)` | KILLED | enum-path |
| M15 | rule_fold `if (0 && s != NULL && val_is(st, OP_FAMILY))` | KILLED | fold-missing, fold-extra (TYPE_MISMATCH) |
| M16 | rule_elim `f->first_ctor + 0u * j` in the case check | KILLED | examples/enums.lang (decEq binder type); the 4 enums evals |
| M17 | rule_elim `0 &&` before the ELIM_ENUM test | KILLED | elim-not-enum (INTERNAL, wrong argument count) |
| M18 | table_json `f->first_ctor + 0u * j` in `cj` | KILLED | algebra.json, enums.json |
| M19 | is_enumeration `(f->path_count != 0u && 0)` | KILLED | circle-table |
| M20 | refute_json `(put_text(o, "false") \|\| printed_json(o, v))` | KILLED | enums.json. Make stops at gate.sh, so proof regression 10 did not run. |
| M21 | is_instance, the VAL_PI arm returns 0 | KILLED | algebra.json, enums.json |
| M22 | rule_absurd infers the argument, no check at Empty | KILLED | absurd-arg |
