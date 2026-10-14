# strategy-lang specification

Status: S0 to S7 DONE (2026-10-14). `strategy-lang` is a working name.

The detail of each section is in `docs/PLAN.md` (the rulings and the
encoding), `docs/STATUS.md` (each slice) and `docs/host/README.md` (the
commands, the JSON document and the limits).

## 1. Purpose

A program describes concurrent game structures (CGS) and one-goal strategy
logic (SL[1G]) sentences on them. `langc` checks the program, and it writes
one JSON document with the value of each instance. The host is tcc-json (the
TinyCC kit). The target is JSON.

The type formers are F1 to F15 of `formers/FORMERS.md`. The core data types
and the core operations are only those of `design/DESIGN.md` (the design).
Proofs are data: the kernel has no proof irrelevance, and the JSON document
writes each proof as a tree (`docs/PLAN.md` section 3, the no-collapse rule).

## 2. Programs

A program is a sequence of definitions and program enumerations (S3):

```
def NAME : TYPE := TERM
```

A program enumeration is a family whose constructors have no fields. The
checker refuses these forms in a program:

| Code | Form |
|---|---|
| `REFUSE_NAME` | A definition that uses a core name or a prelude name |
| `REFUSE_DATA` | A data type that is not an enumeration (a constructor with a field, a path constructor or a parameter) |
| `REFUSE_REC` | General recursion: `def rec`, a use of the definition itself or of a later definition |
| `REFUSE_AXIOM` | An unproved fact (an `axiom` declaration) |

Recursion comes only from `fold`, `unfold`, `elim` and `elimList`. The
compiler writes the target; a program cannot.

## 3. Type formers

The type formers F1 to F15 are DONE on tcc-json (`formers/tcc-json.md`).
The table gives the deviations only.

| ID | Status on tcc-json | Effect on this language | Open item |
|---|---|---|---|
| F7 | DONE into List with a Nat step limit | `unfold` gives a List, and each unfold takes a fuel (`visitsOf`) | O15 |
| F15 | DONE in `domain/` only (rule R1) | A program cannot declare an indexed family. The domain families are `Pth`, `Ltl`, `Mealy`, `Cls` and the kit sample `Lot` | none |

The language adds these forms to the formers:

- S2: higher inductive types (HITs). A domain family can have path
  constructors. `cong` reduces on a path, and a `fold` on a HIT gives a
  path case for each path constructor (`FOLD_PATH_CASE`).
- S3: program enumerations, `elim` on an enumeration (`ELIM_ENUM`,
  `FOLD_CASES`), `Empty` and `absurd`.
- S4b: the dependent `List` eliminator `elimList`.
- S5: recursive domain families with parameters. The build checks them
  (`DOMAIN_FAMILY`).

## 4. Structures

- **Monad**: `pure`, `map`, `bind` on Option, Sum E and List.
- **Algebra**: `fold` on Nat, List and each family. `unfold` into List with
  a Nat step limit.
- **Filterable**: `filter` on Option and List.

## 5. Core types

Source: `docs/PLAN.md` section 2. A name in the domain is in
`domain/domain.lang`. A program declares `St`, `Ag` and `Ac` as
enumerations (S3).

| Type | Meaning (design) | Definition |
|---|---|---|
| State | `St` | A program enumeration |
| Decision | `Ag -> Ac` | A function on the agent enumeration. JSON: a table |
| Transition table | `tau` | `stepTable : St -> Ac -> ... -> St`, one `Ac` argument for each agent, in agent order (O17) |
| Track | `Trk` | `Prod St (List St)`, a track is not empty |
| Path | `Pth` | The domain HIT `Pth S`: the point `lasso` (stem, cycle head, cycle tail) and the paths `unroll` and `unrollOne` (S4b) |
| Strategy | `Str` | A Mealy machine, the domain record `Mealy M S A`. Its meaning on a track is `mealyAct`, a right fold |
| Assignment | `Asg` | A binding list and the completeness evidence in a `Sigma`: `full` (agents, `examples/play.lang`) and `fullV` (variables, `examples/pennies.lang`) |
| Goal | LTL formula | The domain family `Ltl`. An atom is a `Nat` code (O6, O19) |
| Evidence | `G, chi, s |= phi` | `Sat`, a large fold on `Ltl`. A verdict is `Sum (Sat goal p) (Sat (dual goal) p)` |
| Class | the strategy class | The domain family `Cls`: `positional` or `memory` (O2) |

Rule R6 (a): the domain also holds the kit sample for F15 (`Color`, `Item`,
`Stack`, `Lot` and the defs `lineTotal`, `isRed`, `total`, `redTotal` and
`depth`, used by `examples/inventory.lang`). They are not core types.

## 6. Core operations

| Operation | Type | Meaning (design) |
|---|---|---|
| `step` | `St -> (Ag -> Ac) -> St` | The transition `tau` on a decision. A program def, not an instance |
| `visitsOf` | a step, a code, a fuel and a state to `List W` | The states of a play, until the first repeated state (an unfold) |
| `lassoOf` | a code, the visits and the next state to `Option (Pth W)` | The play as a lasso; `none` when the fuel is too short |
| `splitOf` | a play to an `Option` of the split | The stem and the cycle of a closed play; `none` without a closed cycle |
| `Sat`, `SatWord`, `SatPlay` | to a type | The evidence that a goal holds on a position, a word or a play. `SatPlay` is `Empty` without a closed cycle |
| `dual` | `Ltl -> Ltl` | The dual goal. Its evidence is the refutation |
| `holds`, `holdsWord`, `holdsPlay` | to `Flag` | The Flag model check. `holdsPlay` is `flagNo` without a closed cycle |
| `findFirst` | a test and a list to `Option` | The search for a witness index |
| `exists` sentence | `Sigma (i : Ix) (P i)` | A witness index and its evidence |
| `forall` sentence | `(t : TIx) -> P t` | A table of evidence, one row for each positional strategy |

A principal sentence is a quantifier prefix, a binding list from agents to
variables, and a goal (S6). A `flagNo` from an incomplete play is a failed
check, not refutation evidence.

## 7. Target and instance encoding

An instance is a top-level definition whose type is a data type (F14).
`langc build` writes each instance in one JSON document. Proofs do not
erase: each `Eq` value is a proof tree. The full table is
`docs/host/README.md` `## The JSON document`.

| Type | Encoding |
|---|---|
| `Eq A x y` | A proof tree: `{"refl": v}`, `{"symm": p}`, `{"trans": [p, q]}`, `{"cong": ...}`, `{"transport": ...}`, `{"path": ...}` |
| A function on an enumeration | A table: an object with one key for each constructor |
| `A -> Empty` | `{"refute": "f"}` |
| `Pth S` | `{"tag":"lasso",...}` with the stem and the cycle |
| A verdict | `{"inl": e}` (the evidence of the goal) or `{"inr": e}` (the evidence of the dual goal) |
| A sentence | A `Sigma` for `exists`, a table for `forall` |

The `type` text of an instance has a limit of 4096 bytes. A larger type is
refused with `JSON_TYPE` (O16, O22).

## 8. Host and target

The host is the tcc-json kit, a fork of the tcc-wasm kit front end
(`docs/host/README.md` `## Origin`). The build uses TinyCC 0.9.28rc
2026-09-04 mob@0fb54300 (AArch64 Darwin) and Apple clang 21.0.0 for the
`-Wswitch-enum` check. The repository has no `PIN` file: the kit is part of
this tree. The limits are in `docs/host/README.md` `## Limits`.

The facts from `probe/CAPABILITY.md` that set the design: one program file
for each run, recursion only from the eliminators, `Nat` is an unsigned
64-bit integer with an overflow trap, and a parametric recursive family is
possible in the domain only.

The output check is the gate (`make check`): it builds each example and
compares the JSON with the golden in `test/json/`.

## 9. Open items

The items are in `docs/PLAN.md` section 6.

| ID | Item | Status |
|---|---|---|
| O1 | The route (local or a new kit) | CLOSED (local route) |
| O2 | The strategy class of the quantifiers | Open |
| O3 | No truncation | CLOSED (S2) |
| O4 | Groupoid laws are propositional | CLOSED (S2) |
| O5 | Each action is available in each state | Open |
| O6 | Atoms and labelling | CLOSED (S5) |
| O7 | Fill `probe/CAPABILITY.md` | CLOSED (S7) |
| O8 | Each `Eq` def is an instance | RULED 2026-10-13 (USER): ruling (b) |
| O9 | Stuck proof trees in the JSON | Open |
| O10 | The universe of `Eq` | CLOSED (S2) |
| O11 | Dependent elimination of HITs | Open |
| O12 | A stuck `elim` prints as `fold` | Open |
| O13 | Enumerations with parameters | Open |
| O14 | Swapped positional cases | Open |
| O15 | The fuel of play | Open |
| O16 | The `JSON_TYPE` growth of evidence types | Open |
| O17 | Tau as a curried table | Open |
| O18 | No projection in a family with 2 or more constructors | Open |
| O19 | An atom is a `Nat` code | Open |
| O20 | The evidence is written by hand | Open |
| O21 | The evidence is not invariant on the `Pth` class | Open |
| O22 | The verdict `type` grows with the goal and the fuel | Open |
| O23 | The release evidence needs enough fuel | Open |
| O24 | Sentences with two quantifiers are check-only | Open |
| O25 | The search is not evidence | Open |
| O26 | The `memory` class has no example | Open |

## 10. Slices

Each slice is DONE. `docs/PLAN.md` section 5 gives the content and
`docs/STATUS.md` gives the tests.

| Slice | Commit | Content |
|---|---|---|
| S0 | `efdb21d` | The repository from lang-template, the design and the plan |
| S1 | `b40e45d` | The no-collapse rule in the kernel; proofs in the JSON |
| S2 | `37f8808`, `a9edaf3` | HITs and quotients, path cases, `cong` on paths |
| S3 | `acbf60d` | Program enumerations, `elim`, `Empty`, tables and refutations |
| S4a | `2b3d692` | Decisions, transitions, Mealy strategies, assignments and play |
| S4b | `8953207` | The `Pth` quotient and `elimList` |
| S5 | `f60236d` | LTL goals, satisfaction evidence and verdicts |
| S6 | `70d4ba1` | Principal sentences and the review fix of `splitOf` |
| S7 | this commit | This specification, the probe, the status, the validation and the README |
