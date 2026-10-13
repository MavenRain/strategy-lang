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
