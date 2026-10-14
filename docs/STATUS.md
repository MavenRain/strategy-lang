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

## S4a strategies, assignments and play

S4a adds the CGS core with no kernel change: decisions, transitions,
tracks, Mealy strategies, assignments with completeness evidence, and play
to a lasso in `Pth` (`docs/PLAN.md` section 5). In S4a a lasso is a point
only. The `Pth` quotient is in S4b. The base is `acbf60d`. Session 14
built the domain, the programs and the tests, and ran the mutants.
Session 15 wrote the docs.

Facts at `acbf60d` (the S4 kickoff, section 0) that S4a uses:

- Q1: a decision `dc : Ag -> Ac` on two program enumerations is a table
  instance, `{"alice":"heads","bob":"tails"}`, also inside data.
  `step : St -> (Ag -> Ac) -> St` checks and evaluates, but it is not an
  instance. After the `St` table, its codomain is a Pi over a function
  type, so `pi_instance` leaves the def out with no refusal. Inside data it
  is `JSON_VALUE`. The curried table `stepTable : St -> Ac -> Ac -> St`
  (one `Ac` argument for each agent, in agent order) is an instance.
- Q2: a track `Prod St (List St)` checks, evaluates and builds. `fold` on
  `List` is a right fold. A fold seed `none` has no hint (`TYPE_INFER`),
  so the seed is a typed def (`noSt : Option St := none`).
- Q4: `unfold g limit seed : List A` takes a fuel. There is no coalgebra
  type former, and `def rec` is refused. A Mealy machine is a generic
  domain record. Its constructor takes the parameters first
  (`machine Mem St Ac m0 ...`). A projection takes no parameters
  (`mUpd g m s`). A machine on `Unit` memory has no table form, because
  `Unit` is not an enumeration family.
- Q5: a generic family over `Ag` needs a type parameter, and only the
  domain can declare one. The PLAN form (a binding list, a lookup and the
  evidence `complete`) builds. The `type` text of `complete` is the normal
  form with each machine inlined, so it grows with the agents and the
  machines. Mixed memory types need a `Sigma` over a type (`JSON_VALUE`),
  so the machines of one binding list use one memory enumeration.
- Q6: `def rec` is refused, so play is an `unfold` with fuel. The stop
  test is `natEq` on a `Nat` code of the product state. A short fuel gives
  a list with no repeat, so the lasso split gives `Option`. A `Flag` stop
  test is a computation, not evidence.
- Q7: the no-collapse rule. The completeness evidence is a table of `Eq`
  proofs (`{"refl":true}` leaves). The closing evidence of a lasso,
  `Eq W (stepW last) repeat`, is `refl` on a closed play (canonicity). Its
  JSON is a `{"refl": v}` leaf, where `v` is the product state. The stop
  test and the fuel hold no proof, so the closing evidence is a separate
  instance.

Rulings (USER, 2026-10-14): R1 (b). R2 to R8 (a), each the default. R1
(b) moves R3 and the R4 paths to S4b.

- R1 (b): the slice split. S4a = decisions, transitions, tracks, Mealy
  strategies, assignments and play to a lasso with no path. No kernel
  change. S4b = the `Pth` quotient: the R3 `List` eliminator, the `unroll`
  paths, and functions on `Pth` with their path proofs.
- R2 (a): the JSON form of tau. No kernel change. A decision is a table
  `Ag -> Ac`. A game gives `stepTable` as the JSON instance, and
  `step := fun s d => stepTable s (d a1) ... (d an)` for computation.
  `step` is not an instance.
- R3 (a): a dependent `List` eliminator `elimList`. Moved to S4b.
- R4 (a): the shape of `Pth`. S4a builds the point part only: the domain
  family `Pth (S : Type 0)` with
  `lasso (lstem : List S) (lhead : S) (ltail : List S)`. The cycle is
  `lhead` and then `ltail`, so it is not empty by construction. The path
  constructors `unroll` and `unrollOne` and the domain def `snoc` are in
  S4b.
- R5 (a): strategies. The domain record `Mealy (M) (S) (A)` with
  `machine (mInit : M) (mOut : M -> S -> A) (mUpd : M -> S -> M)`, and the
  generic domain def `mealyAct`. A positional strategy is a machine on a
  memory enumeration with one constructor.
- R6 (a): assignments. The PLAN form: a binding list, a lookup, and a
  `Sigma` with the completeness evidence as a table of proofs.
- R7 (a): play. Program code on two generic domain helpers, `visitsOf`
  and `lassoOf`. The program writes the product state `W`, `stepW` and
  `codeW`. It states the closing evidence as its own instance (`refl`).
- R8 (a): names. The domain takes only generic names. Game names stay in
  programs. The one global namespace stays (S3 R8).

Changes:

- `domain/domain.lang` (34 lines added). No kernel file changed. The new
  domain names are global names, so a program cannot use them again
  (`REFUSE_NAME`):
  - `Pth (S : Type 0)` with the one constructor
    `lasso (lstem) (lhead) (ltail)`. In S4a it has no path constructor.
  - `Mealy (M : Type 0) (S : Type 0) (A : Type 0)` with
    `machine (mInit) (mOut) (mUpd)`.
  - `mealyRun`: a right fold that builds `S -> M -> A`. The memory reads
    each state before the last, then `mOut` reads the last state.
  - `mealyAct M S A g t := some (mealyRun M S A g (second t) (first t)
    (mInit g))`. The type is `Prod S (List S) -> Option A`.
  - `noCodes`, `seenCode` (a `Flag` test for a code in a list) and
    `stopVisit` (a typed `none`).
  - `visitsOf W f c fuel w0`: an `unfold` on the seed (the state, the
    codes seen). It stops before the first state with a code that it saw.
  - `lassoSeed` (a typed seed) and `lassoOf W c ws r`: a right fold on
    `Prod (Option (Pth W)) (List W)`. At the entry `x` with the code of
    `r`, it gives `some (lasso W nil x suffix)`. At an entry before it,
    `map` puts the entry at the front of the stem.
- `examples/game.lang` with `test/json/game.json`: the enumerations `St`
  (`s0`, `s1`), `Ag` (`alice`, `bob`) and `Ac` (`heads`, `tails`), the
  decision `dc`, the table `stepTable`, `step` (not an instance), `next`,
  `held` (a decision in data), `noSt`, the track `trk` = s0 s1 s1,
  `trkLenOf`, `lastOf`, `trkLen` = 3, `trkLast` = s1, and the lasso
  `cyc` = `lasso [s0] s1 []`.
- `examples/play.lang` with `test/json/play.json`: the game, plus the
  memory enumerations `Mem` (`m0`, `m1`) and `Solo` (`only`). The machines
  are `flip` (heads, tails, heads, ...), `wary` (heads once, then tails at
  s0 and heads at s1) and `mirror` (positional, on `Solo`). Then `trk` and
  `a1`. The assignment is `agCode`, `noneM`, `lookupAg`, `isSome`,
  `bindings` = [(alice, flip), (bob, wary)], `Has`, `complete` and `full`.
  The play is on `W = Prod St (Prod Mem Mem)`: `stCode`, `memCode`,
  `codeW` = 4 st + 2 ma + mb, `stepW` (it calls `step` on a decision),
  `w0`, `visits` (fuel 9 = |W| + 1), `noW`, `lastW`, `play`, `headW`, and
  `closes : Eq W (stepW lastW) headW := refl`.
- 1 new check refusal: `mealy-params` (`TYPE_MISMATCH`). A projection of
  a family with parameters takes no parameters, so
  `mUpd Mem St Ac g m0 s0` is refused.
- 2 new emit refusals (`JSON_VALUE`): `step-in-data` (a transition on a
  decision inside data, R2) and `unit-memory` (a machine on `Unit` memory
  has no table form, Q4).
- 4 evals: `game next => s1`, `game trkLast => s1`,
  `play a1 => some heads`, and `play visits` (the 5 entries of the trace
  below).

Each eval and each line of `game.json` and `play.json` was checked by
hand against the definitions. The `type` of `complete` is 340 bytes and
the `type` of `full` is 216 bytes, well under the 4096-byte limit (O16).

Hand trace of the play (a state is (st, ma, mb), where ma is the memory
of `flip` for `alice` and mb is the memory of `wary` for `bob`):

- w0 (s0, m0, m0): heads, heads. The next state is w1 (s0, m1, m1).
- w1 (s0, m1, m1): tails, tails. The next state is w2 (s0, m0, m1).
- w2 (s0, m0, m1): heads, tails. The next state is w3 (s1, m1, m1).
- w3 (s1, m1, m1): tails, heads. The next state is w4 (s1, m0, m1).
- w4 (s1, m0, m1): heads, heads. The next state is w1. Its code 3 was
  seen, so the unfold stops.

The lasso is stem [w0], head w1, tail [w2, w3, w4]. `closes` is
`{"refl": w1}`, because `stepW w4` and `headW` are both w1.

Three tests differ from the kickoff section 3 text. No kernel change and
no gate target change followed from them:

- `game trkLast => s1`, not `s0`. Reason: the track is s0 s1 s1, so a
  last state function that gives the first state fails.
- `play a1 => some heads` stays, but the value comes from the correct
  `mealyAct` on `wary`, not from the P4 shape. Reason: the memory after s0
  is m1, and `mOut m1 s1` is heads.
- `play visits` is the 5 entry list of the trace above, not the P6 list.
  Reason: this play has a stem.

Moved to S4b (R1 (b)):

- R3: the dependent `List` eliminator `elimList` (`OP_ELIM_LIST`), the
  only kernel change of S4.
- R4 paths: the path constructors `unroll` and `unrollOne`, the domain
  def `snoc`, and functions on `Pth` with their path cases.
- The mutants M23 to M27, the eval `game cycLen1`, the `elimList` and
  `Pth` path check tests, and the 2 proof regressions.
- JSON: with `unroll` and `unrollOne`, `Pth` is not a family with one
  constructor. Then `cyc` in `game.json` and `play` in `play.json` become
  `{"tag":"lasso",...}`. S4b updates both goldens by hand.

New open items: O15 (the fuel of play), O16 (the `JSON_TYPE` growth of
evidence types) and O17 (tau as a curried table), in `docs/PLAN.md`
section 6.

Gate (`make -B check`), S3 to S4a: examples 13 to 15 checked, 0 failed;
parse refusals 9 to 9; check refusals 49 to 50; evals 32 to 36; json 13
to 15 builds; build refusals 6 to 8; gate 0 failures; regressions 6
checked, 0 failed; printer regressions 2 checked; proof regressions 17 to
17 checked, 0 failed. The clang `-Wswitch-enum` build is clean. The S4a
column of the kickoff section 5 was for R1 (a). With R1 (b), `play.lang`,
`mealy-params` and `unit-memory` are in S4a, and the `elimList` tests are
in S4b.

Mutants (session 14, one `make -B check` each, source restored after
each, `shasum` OK after each restore). Each kill is a test. The final gate
after the mutants was GREEN at the same counts.

| Mutant | Form | Result | Killed by |
|---|---|---|---|
| M28 | lassoOf, the match branch `lasso W (cons x nil) x suffix` (the repeat entry also goes to the stem, so the split is one entry late) | KILLED | json `play` |
| M29 | mealyRun `k h (mUpd g m x)` (it reads the next state first, so a track of 2 states is read in reverse) | KILLED | eval `a1` (`some tails`), json `play` |
| M30 | table_type in `src/json.c`, `\|\| val_is(type->dom, OP_UNIT)` | KILLED | emit `unit-memory` (exit 0) |

## S4b the Pth quotient

S4b adds the `Pth` quotient: the dependent `List` eliminator `elimList`
(the only kernel change of S4), the path constructors `unroll` and
`unrollOne`, and a function on `Pth` with its path cases (`docs/PLAN.md`
section 5). The base is `2b3d692`. Session 16 wrote the kickoff. Sessions
17 and 18 built the code and the tests, and ran the mutants. Session 19
wrote the docs.

Facts at `2b3d692` (the S4b kickoff, section 0) that S4b uses:

- P1: the R4 (a) shape is a legal family. `embed.c` and tcc accept it,
  with no `DOMAIN_FAMILY` and no `DOMAIN_TRUNCATION`. But the S4a domain
  then fails, and each program fails with it:
  `NAME_UNKNOWN: lassoOf: unknown name lstem`. Cause: `find_known` makes a
  field a projection only in a family with 1 constructor, and `Pth` now
  has 3 constructors.
- P1b: `headW` in `examples/play.lang` read `lhead p` from the play.
  `lhead` is not a function on the quotient, because `unroll` moves the
  head from `uc` to `ud`. Its `fold` form with `refl` path cases fails with
  `TYPE_REFL`.
- P2: a `fold` on `Pth` with only the point case fails with
  `FOLD_PATH_CASE`, and with `refl` path cases it fails with `TYPE_REFL`.
  Thus the kernel does not let a fold drop the paths.
- P2b: a function `Pth W -> Pth W` that puts `x` at the front of the stem
  checks with the path constructors as its path cases. Reason:
  `snoc W (cons x upre) uc` reduces by one `fold` step to
  `cons x (snoc W upre uc)`. A function into another type (`Nat`, `St`)
  needs a fact about `snoc` on a variable list in its path case, so it
  needs `elimList`.
- P3: the path ends reduce on closed lists. `unroll` at a nil cycle fails
  with `TYPE_MISMATCH`. `langc eval` prints a path as a stuck path term,
  for example `unroll St nil s0 s1 nil`. The JSON forms are in
  `docs/host/README.md`.
- P4: with no `elimList`, a `cycLen` with `refl` path cases fails with
  `TYPE_REFL`. Thus R3 is necessary.
- P5: no test pinned the projection guard. With `ctor_count == 1u &&`
  removed, the 50 check refusals passed, and the S4a `play.lang` with
  `lhead` checked.
- P6: `src/json.c` has `case OP_FOLD_LIST:` in a switch on the op.
  `make check` runs `cc -Wswitch-enum -Werror`, so `OP_ELIM_LIST` needs an
  arm there too.
- P7: `game cycLen1` runs only the point case of `cycLen`. Thus the cycle
  count in the point case is an `elimList` (`stCount`), not a `fold`.
  With a `fold`, M25 and M26 live.
- Q9: a field of a family with a path constructor has no projection.
  Before S4b, no test pinned this (P5).
- Q10: `lassoOf` gives the first repeat split. A value that depends on the
  split (the head of the cycle, `closes`) must read the split, not the
  `Pth`.

Rulings (USER, 2026-10-14): R3 (a) and R4 (a) (from the S4 kickoff, moved
to S4b by S4a R1 (b)). R9 (a), R10 (a) and R11 (a), each the default
(session 17).

- R3 (a): the dependent `List` eliminator `elimList P z s xs : P xs`, with
  `P : List A -> Type l`, `z : P nil` and
  `s : (x : A) -> (ys : List A) -> P ys -> P (cons x ys)`. It is the new op
  `OP_ELIM_LIST`. Its core term drops `P`, as the S3 `elim` does.
- R4 (a): the domain HIT `Pth (S : Type 0)` with the point constructor
  `lasso (lstem : List S) (lhead : S) (ltail : List S)` and two path
  constructors. `unroll (upre) (uc) (ud) (uds)` is a path from
  `lasso S upre uc (cons ud uds)` to
  `lasso S (snoc S upre uc) ud (snoc S uds uc)`. `unrollOne (opre) (oc)`
  is a path from `lasso S opre oc nil` to
  `lasso S (snoc S opre oc) oc nil`. The domain def `snoc` is above the
  family.
- R9 (a): split `lassoOf`. `lassoParts W c ws r` is the S4a fold with the
  triple (stem, (head, tail)) in place of the `Pth`. `lassoOf` is a `map`
  of a `lasso` build over `lassoParts`. The fold never takes a `Pth`
  apart.
- R10 (a): `headW` reads the head of the cycle from the canonical split
  (`lassoParts`), not from the `Pth`. `closes` does not change (`refl`).
- R11 (a): the test `pth-field` and the mutant M31 pin the projection
  guard. No kernel change and no new refusal code (O18).

Changes:

- `src/front/core.h`: the new op `OP_ELIM_LIST`, after `OP_FOLD_LIST`.
- `src/front/check.c` (45 lines added): the builtin `elimList` with 4
  arguments, and `rule_elim_list`. The rule infers `xs` first, and its
  type must be `List A`. Then it infers `P`, which must be a Pi from
  `List A` to a universe. `z` checks against `P nil` and `s` against the
  step type. The result type is `P xs`, and the core term is
  `elimList z s xs`.
- `src/front/eval.c`: the print name `elimList`, the arity 3, and
  `reduce_elim_list`. At `nil` the result is `z`. At `cons x t` it is
  `s x t (elimList z s t)`. As in `fold`, the end of the list is reduced
  first. On a neutral tail the result is a stuck `elimList`, and a trap
  stays a trap.
- `src/json.c`: `case OP_ELIM_LIST:` next to `case OP_FOLD_LIST:` (P6).
  The build refuses a stuck `elimList` with `JSON_VALUE`.
- `domain/domain.lang`: `snoc S xs x` (a right fold that puts `x` at the
  end of `xs`), the two path constructors of `Pth` (R4), `lassoParts` and
  the new `lassoOf` (R9 (a)). The type of `lassoSeed` changes to the seed
  of the triple. The new comments tell about `snoc`, the lasso and the two
  paths.
- `examples/game.lang`: the lassos `p1` = `lasso [] s0 [s1]` and `p2` =
  `lasso [s0] s1 [s0]`, the path `u12 : Eq (Pth St) p1 p2` =
  `unroll St nil s0 s1 nil`, the count `stCount` (an `elimList`), the
  proof `stCountSnoc` (by `elimList`: the count of `snoc St xs c` is the
  count of `xs` plus 1), `cycLen : Pth St -> Nat`, and `cycLen1` =
  `cycLen p1` = 2. The `unroll` case of `cycLen` is
  `cong (fun (n : Nat) => natAdd 1 n) (symm (stCountSnoc uds uc))`. Its
  `unrollOne` case is `refl`.
- `examples/play.lang`: `headW` reads `first (second t)` from
  `lassoParts` (R10 (a)).
- `test/json/game.json` and `test/json/play.json`: `Pth` now has 3
  constructors, so `cyc` and `play` are `{"tag":"lasso",...}`. Each golden
  gets one `"tag":"lasso",`. `game.json` also gets `p1`, `p2`, `u12`
  (`{"path":"unroll","upre":[],"uc":"s0","ud":"s1","uds":[]}`) and
  `cycLen1` (2). The `game.json` lines were written by hand, and they
  matched the build byte for byte.

Tests:

- 5 new check refusals: `elim-list-motive` (`P` over `Nat`, not over
  `List St`, `TYPE_MISMATCH`), `elim-list-case` (`s` with 2 arguments, not
  3, `TYPE_MISMATCH`), `pth-path-case` (`cycLen` with a `fold` count and
  `refl` path cases, `TYPE_REFL`, P4), `pth-unroll-one` (`unroll` at a nil
  cycle, `TYPE_MISMATCH`, P3) and `pth-field` (`lhead` on a `Pth` point,
  `NAME_UNKNOWN`, R11 (a)).
- 1 new eval: `game cycLen1 => 2`.
- 2 new proof regressions in `test/proof-regressions.c`:
  - The `JSON_CASES` row `unroll path fields`: an `unroll` with 4
    different fields gives
    `{"path":"unroll","upre":["s1"],"uc":"s0","ud":"s1","uds":["s0"]}`.
  - `check_stuck_elim_list`: a C check, counted by `STUCK_CHECKS`. It
    applies `lift` (a stuck `elimList` under `trans r`) to a variable, and
    puts the result in place of the value of `pr`. The JSON build must
    refuse it with `JSON_VALUE`.

Three facts differ from the kickoff text (session 18). No kernel change
and no gate target change followed from them:

- The stuck `elimList` proof regression is a C check counted by
  `STUCK_CHECKS`, not a `JSON_CASES` row. Reason: a closed def cannot hold
  a stuck `elimList`, so the test builds the value in C.
- That test uses `trans r`, with `def r : Eq Nat 1 1 := refl`, not
  `trans refl (...)`. Reason: the first text failed with `TYPE_INFER`
  (`refl needs a declared type`).
- The M31 site is `check.c:1221`, not `check.c:1176` (the line at
  `2b3d692`). Reason: the S4b code above it.

Gate (`make -B check`), S4a to S4b: examples 15 to 15 checked, 0 failed;
parse refusals 9 to 9; check refusals 50 to 55; evals 36 to 37; json 15
to 15 builds; build refusals 8 to 8; gate 0 failures; regressions 6
checked, 0 failed; printer regressions 2 checked; proof regressions 17 to
19 checked, 0 failed. The clang `-Wswitch-enum` build is clean.

Mutants (session 18, one `make -B check` each, source restored after
each, `shasum` OK after each restore). Each mutant changes 1 line, and
each kill is a test. The final gate after the mutants was GREEN at the
same counts.

| Mutant | Form | Result | Killed by |
|---|---|---|---|
| M23 | rule_elim_list, `s` is inferred (`infer(c, k->args[2], &pt)`), not checked against the step type | KILLED | check `elim-list-case` (exit 0) |
| M24 | rule_elim_list, the motive test becomes `if (0 && (...))` | KILLED | check `elim-list-motive` (exit 0) |
| M25 | reduce_elim_list, the last argument of `s` is the tail `s.cells[i - 1u]->args[1]`, not the result on the tail | KILLED | examples `game` (`INTERNAL` in `stCountSnoc`), eval `cycLen1` (and `next`, `trkLast`), json `game` |
| M26 | reduce_elim_list, the `nil` arm `? a[0] :` becomes `? a[1] :` | KILLED | the same tests as M25 |
| M27 | domain `unroll`, the right end becomes `(lasso S (snoc S upre ud) uc (snoc S uds ud))` | KILLED | examples `game` (`u12` `TYPE_MISMATCH`), the evals of `game`, json `game` |
| M31 | `check.c:1221`, drop `ctor_count == 1u &&` | KILLED | check `pth-field` (exit 0) |

M25 and M26 also break the check of `stCountSnoc`, because conversion
evaluates `elimList`. Thus examples `game` kills them too.

Open items in `docs/PLAN.md` section 6: O11 narrows (`elimList` is for
`List` only), O12 adds the stuck `elimList` print, and O18 is new (no
projection in a family with 2 or more constructors).
