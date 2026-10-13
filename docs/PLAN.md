# strategy-lang plan

## 1. Rulings

The USER gave these rulings in the request:

- The core types and the core operations come only from `design/DESIGN.md`:
  the minimal span of a concurrent game structure (CGS) and one-goal
  Strategy Logic (SL[1G]).
- The type constructors are products, coproducts, monads (map and bind),
  algebras and coalgebras (fold and unfold), and quotients (paths, loops,
  higher inductive types (HIT) and quotient inductive types (QIT), as in
  HoTT).
- Proofs must not collapse by irrelevance (the no-collapse rule, section 3).
- Host: TinyCC. Output: the JSON serialization of the final states.

These are the builder's calls. The USER did not rule them. Section 6 lists
them as open items:

- Name: strategy-lang. The compiler binary is `build/langc`.
- Host kit: `hosts/tcc-json` of lang-template, without change. The kit
  output is the final value of each instance, so it agrees with the output
  ruling. The new kernel parts (sections 3 and 4) go into this repository,
  not into lang-template (O1).
- Tests: unit and integration tests are in the guest language. A host
  runner that loads and runs guest test files is permitted. Do not add a
  node, Python or shell harness.

## 2. The design on the kit basis

This table reads each row of `design/DESIGN.md`. The design text is a copy
of rendered math, so each name appears three times. The table gives the
name one time.

| Concept | Type | Encoding (proposal) | Slice |
|---|---|---|---|
| State | `St` | An enumeration (constructors with no fields) that the program declares | S3, S4 |
| Decision | `Dc = Ag -> Ac` | A function from an agent enumeration to an action enumeration. JSON: an object with one key for each agent. | S3, S4 |
| Transition | `tau : St * Dc -> St` | A program def `step : St -> Dc -> St`. The design gives a total `tau`, so each action is available in each state (O5). | S4 |
| Track | `Trk` | A state and a list of states (a track is not empty) | S4 |
| Path | `Pth` | A lasso (a stem track and a cycle that is not empty), as a quotient by unrolling (section 4) | S2, S4 |
| Strategy | `Str = Trk -/-> Ac` | `Trk -> Option Ac`. Strategies are made from Mealy machines (a coalgebra). The meaning on a track is a fold. | S4 |
| Assignment | `Asg = (Var + Ag) -/-> Str` | A list of bindings. Lookup gives `Option Str`. A complete assignment is a Sigma of the list and the evidence that each agent has a strategy. | S4 |
| Play | `play : Asg * St -> Pth` | An unfold of the product machine from the state. It stops at the first repeated machine state and gives the lasso. | S4 |
| Principal sentence | `quantifiers, bindings, goal` | A quantifier prefix (exists or forall over strategy variables), a binding list (agent to variable) and an LTL goal | S6 |
| Satisfaction | `G, chi, s |= phi` | An inductive family of evidence on plays. A verdict is a Sum of the evidence and the refutation evidence. | S5, S6 |

The design gives no labelling of states. The goal atoms need one. The
proposal is an atom enumeration and a def `label : St -> Atom -> Flag` in
the game (O6).

## 3. The no-collapse rule

A proof is data. The kernel and the JSON writer must keep it. These are
not permitted:

- a universe with definitional proof irrelevance (a `Prop` that identifies
  all of its proofs);
- UIP or axiom K, as an axiom or as a conversion rule;
- truncation of a quotient or of a HIT to a set;
- a conversion rule that makes two proofs of one `Eq` type equal only
  because they have the same type;
- groupoid laws as definitional equalities (for example
  `trans p (symm p)` to `refl`). They are propositional only (O4).
- a JSON writer that erases a proof.

Gaps in the kit at the start (found in the S0 survey):

- `src/json.c` writes each `Eq` value as `null`.
- `src/json.c` does not make an instance of a def whose type is an `Eq`
  type.
- `symm`, `trans`, `cong` and `transport` reduce on `refl` only. This is
  the J computation rule and it is permitted. S1 makes sure that no other
  rule identifies proofs.

## 4. Quotients and HITs

- A HIT declaration gives point constructors and path constructors. A path
  constructor has a type `Eq T a b`.
- The eliminator takes a case for each point constructor and a case for
  each path constructor (dependent: a path over the path). A point case
  computes by definition. `cong (elim ...) c` for a path constructor `c`
  computes by definition to the path case.
- Examples: the circle (`base`, `loop`), the interval, and the quotient
  `Quot A R` (`class`, `glue`). There is no truncation constructor, by the
  no-collapse rule (O3).
- A QIT is a HIT whose path constructors relate point constructors of an
  inductive type. It has no set truncation (O3).
- JSON: a HIT point is written as a family value. A path is written as its
  normal proof term: `{"refl": v}`, a path constructor, or a tree of
  `symm`, `trans`, `cong` and `transport` nodes.
- `Pth` is the quotient of lassos by unrolling:
  `(stem, c :: cs)` is equal to `(stem ++ [c], cs ++ [c])`. A function on
  `Pth` must give the path case, so a goal cannot tell two unrollings of
  one path apart.

## 5. Slices and gates

- S0 (this commit): `bin/new-lang.sh strategy-lang tcc-json`,
  `design/DESIGN.md` (the design without change) and this plan. The gate
  dash scan skips `design/DESIGN.md`, because the design text has dashes and
  it must stay without change (dil-lang precedent).
- S1: the no-collapse rule in the kernel. Audit the conversion and the
  reducer. The JSON writer writes proof terms, and `Eq` defs become
  instances. Refusal tests for a proof that checks only by irrelevance.
- S2: HITs and quotients (section 4). Refusal tests: a missing path case,
  a path case of the wrong type, and a truncation constructor.
- S3: enumerations in user programs (no fields), exhaustive elimination,
  decidable equality with evidence both ways, and functions on an
  enumeration as JSON tables.
- S4: the CGS core: decisions, transitions, tracks, strategies (Mealy
  machines and their fold), assignments and their completeness evidence,
  and `play` to a lasso in `Pth`.
- S5: LTL goals and satisfaction evidence on lassos (next, until, release,
  and the derived forms), with refutation evidence.
- S6: principal sentences. `exists` takes a witness strategy, or searches
  the positional strategies. `forall` gives evidence for each positional
  strategy (O2). Examples: matching pennies, a train and gate controller,
  and a sentence that shares one strategy between two agents (not
  expressible in ATL*).
- S7: `SPEC.md`, `probe/CAPABILITY.md`, `docs/STATUS.md`,
  `docs/VALIDATION.md` and `README.md`. Final gate.
- Gate: `make check` in this directory.

## 6. Open items

- O1: the route. The kernel parts of S1 and S2 are local to this repository.
  The other route is a new generic kit in lang-template (for example
  `hosts/tcc-json-hott`), as for tcc-js and tcc-webgpu. Default: local.
- O2: the strategy class of the quantifiers. SL[1G] uses strategies with
  perfect recall. M1 quantifies over positional strategies (a finite set),
  and an `exists` witness can be any Mealy machine. With positional
  strategies a verdict can differ from the perfect-recall verdict. Each
  JSON verdict names its class. Finite-memory quantification is later.
- O3: no truncation. Usual QITs are set-truncated. The no-collapse rule
  forbids this, so quotients are not truncated.
- O4: groupoid laws are propositional. The proof terms stay distinct in
  the JSON output.
- O5: each action is available in each state, because the design `tau` is
  total. A protocol (available actions for each state) is not in the
  design.
- O6: atoms and labelling are not in the design. The goal atoms need them.
- O7: fill `probe/CAPABILITY.md` (S7).
