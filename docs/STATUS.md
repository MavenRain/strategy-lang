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
