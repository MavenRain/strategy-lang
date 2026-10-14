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
| State | `St` | An enumeration (constructors with no fields) that the program declares. S3 part done: a program can declare an enumeration (S3 R1). | S3, S4 |
| Decision | `Dc = Ag -> Ac` | A function from an agent enumeration to an action enumeration. JSON: an object with one key for each agent. S3 part done: a function on an enumeration is a JSON table (S3 R6). S4a done: `dc` in `examples/game.lang` is a table instance. | S3, S4a |
| Transition | `tau : St * Dc -> St` | A program table `stepTable : St -> Ac -> ... -> St` (one `Ac` argument for each agent, in agent order) is the JSON instance. A program def `step : St -> Dc -> St` calls it, and `step` is not an instance (S4a R2, O17). The design gives a total `tau`, so each action is available in each state (O5). S4a done. | S4a |
| Track | `Trk` | A state and a list of states, `Prod St (List St)` (a track is not empty). S4a done. | S4a |
| Path | `Pth` | A lasso (a stem track and a cycle that is not empty), as a quotient by unrolling (section 4). S4a part done: the domain family `Pth S` with the one point constructor `lasso (lstem) (lhead) (ltail)`. The cycle is `lhead` and then `ltail`. The `unroll` paths and `elimList` are in S4b (S4a R1 (b)). | S2, S4a, S4b |
| Strategy | `Str = Trk -/-> Ac` | `Trk -> Option Ac`. Strategies are made from Mealy machines (a coalgebra). The meaning on a track is a fold. S4a done: the domain record `Mealy M S A` and the domain def `mealyAct` (a right fold). | S4a |
| Assignment | `Asg = (Var + Ag) -/-> Str` | A list of bindings. Lookup gives `Option Str`. A complete assignment is a Sigma of the list and the evidence that each agent has a strategy. S4a done for agents (`bindings`, `lookupAg`, `complete` and `full` in `examples/play.lang`). The `Var` part is S6. | S4a, S6 |
| Play | `play : Asg * St -> Pth` | An unfold of the product machine from the state. It stops at the first repeated machine state and gives the lasso. S4a done: the domain helpers `visitsOf` and `lassoOf`, and in the program `stepW`, `codeW`, `play : Option (Pth W)` and the closing evidence `closes` (O15). | S4a |
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

Gaps in the kit at the start (found in the S0 survey), and what S1 did
(the audit is in `docs/STATUS.md`):

- `src/json.c` wrote each `Eq` value as `null`. Closed in S1: an `Eq`
  value is `{"refl": v}`, with the point `v` (O8).
- `src/json.c` did not make an instance of a def whose type is an `Eq`
  type. Closed in S1: each `Eq` def is an instance (O8).
- `symm`, `trans`, `cong` and `transport` must reduce on `refl` only. This
  is the J computation rule and it is permitted. The S1 audit found that
  `reduce_proof` also reduced them on a proof that is not `refl` (F1).
  Closed in S1: such a proof stops the operation, so the operation stays a
  proof term. The audit found no other rule that identifies proofs.

## 4. Quotients and HITs

- A HIT is declared in `domain/domain.lang` only (R1). A program can
  declare an enumeration only: no parameters, and each constructor has no
  fields and no path type (S3 R1). A HIT declaration
  gives point constructors and path constructors. A path constructor has
  a type `Eq T a b`. Its fields are a telescope (R3).
- Each end `a` and `b` of a path constructor is built from point
  constructors only (the point-term rule, fixed).
- 1-paths only. No path field mentions the family. A truncation is
  refused at build time, `DOMAIN_TRUNCATION` (R4).
- The fold is not dependent (R2). It takes a case for each point
  constructor and a case for each path constructor. A path case for
  `p : Eq T a b` is a function of the fields of `p`. Its result type is
  `Eq C a' b'`, where `a'` and `b'` are the folds of `a` and `b`. By the
  point-term rule, these folds read only the point cases. A point case
  computes by definition. `cong (fun x => fold cases x) c` for a path
  constructor `c` computes by definition to the path case (R8).
  Dependent elimination is open (O11).
- Examples: the circle (`base`, `loop`), the interval (`zero`, `one`,
  `seg`), and the quotient `Quot A R` (`class`, `glue`). There is no
  truncation constructor, by the no-collapse rule (O3).
- A QIT is a HIT whose path constructors relate point constructors of an
  inductive type. It has no set truncation (O3).
- `Eq A x y` is at the level of `A` (R6, O10).
- JSON: a HIT point is written as a family value. A path is written as its
  normal proof term, a structured tree (R5): `{"refl": v}`, a path
  constructor, or a tree of `symm`, `trans`, `cong` and `transport`
  nodes. The writer refuses a stuck term that is not a proof (R7).
  Other stuck proof forms, such as a fold blocked on a transported
  point, are also refused with `JSON_VALUE` (O9).
- `Pth` (S4, R9) is the quotient of lassos by unrolling:
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
  enumeration as JSON tables. DONE (rulings R1 to R9, each the default
  (a), 2026-10-13): program enumerations, `FOLD_CASES`, `elim` for
  enumerations, `Empty` and `absurd`, refutations and tables in the JSON.
  Refusal tests: a program family that is not an enumeration, a wrong
  case count, an `elim` on a family that is not an enumeration, a wrong
  motive, an `absurd` with no hint, and a table on a HIT. See
  `docs/STATUS.md` `## S3 enumerations and decisions`.
- S4a: the CGS core with no kernel change. DONE (rulings R1 (b), R2 to R8
  (a), 2026-10-14): decisions and transitions as tables, tracks, Mealy
  strategies (`Mealy`, `mealyAct`), assignments with completeness
  evidence, and `play` to a lasso in `Pth` (a point, no path) with closing
  evidence. Refusal tests: a projection with parameters, a transition
  inside data, and a machine on `Unit` memory. See `docs/STATUS.md`
  `## S4a strategies, assignments and play`.
- S4b: the `Pth` quotient. Open: R3 (the dependent `List` eliminator
  `elimList`, the only kernel change of S4) and R4 (the paths `unroll` and
  `unrollOne`, the domain def `snoc`, and functions on `Pth` with their
  path cases), with the mutants M23 to M27. Then `Pth` has more than one
  constructor, and the goldens write a lasso as `{"tag":"lasso",...}`.
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
  forbids this, so quotients are not truncated. CLOSED (S2, R4):
  quotients stay untruncated. A truncation is refused at build time
  (`DOMAIN_TRUNCATION`, `test/emit/truncation.domain`).
- O4: groupoid laws are propositional; there is no proof irrelevance.
  CLOSED (S2): general groupoid identities are not
  definitional reductions, and proofs are not identified merely because
  their endpoints agree. Reflexivity computation still applies: if
  `r : Eq Circle base base := refl`, then `symm r` reduces to `refl`.
  The fold path computation in section 4 is also definitional.
  Tests: `proof-inverse`,
  `proof-symm-symm`, `proof-trans-refl`, `proof-cong-id` (S1), and
  `path-symm`, `path-not-refl`, `path-cong-stuck` on a path constructor
  (S2).
- O5: each action is available in each state, because the design `tau` is
  total. A protocol (available actions for each state) is not in the
  design.
- O6: atoms and labelling are not in the design. The goal atoms need them.
- O7: fill `probe/CAPABILITY.md` (S7).
- O8: each `Eq` def is an instance. A point with no JSON form (a function
  or a type) is written as its normal form in source syntax, a string.
  Ruling (b), 2026-10-13.
- O9: stuck proof trees in the JSON output wait for S2. In S1 no closed
  proof is stuck (canonicity). A `refl` leaf under `trans` or `cong` needs
  a point that the value does not hold. `ready()` in `src/json.c` still
  refuses a stuck term, so the writer erases no proof. S2 (R5) writes
  the listed proof forms as structured trees. The writer gets the
  point of each `refl` leaf from the ends of the type (`proof_ends` in
  `src/json.c`). A stuck `cong` keeps its carrier (`37f8808`). `ready()`
  still refuses a stuck term that is not a proof (R7). Other stuck proof
  forms remain unsupported: a fold blocked on a transported point can
  have an `Eq` type and still be refused with `JSON_VALUE`. See the
  example in `docs/host/README.md`.
- O10: `Eq A x y` is in `Type 0` for each level of `A` (F7,
  `src/front/check.c`). S2 decides the level rule. CLOSED (S2, R6):
  `Eq A x y` is at the level of `A` (`rule_eq`). Test: `eq-level`.
- O11: dependent elimination of HITs (R2). The S2 fold is not dependent.
  A dependent eliminator needs a path over the path: a path in the motive
  over each path constructor. Open. The S3 `elim` is for enumerations
  only.
- O12: a stuck `elim` prints as `fold` (S3 Risk 4). The core term of
  `elim` drops the motive `P`. `P` is a type, not a proof, so no proof is
  lost. But the printed text does not check again, because `fold` is not
  dependent. Printed text occurs in `refl` points (O8) and in `refute`
  strings (S3 R5). Open.
- O13: enumerations with parameters (S3 R1). A program enumeration has no
  parameters. A program family with parameters is refused with
  `REFUSE_DATA` (`enum-param`). Open.
- O14: swapped positional cases (S3 R2). `fold` and `elim` take the cases
  in declaration order and check no names. Thus two swapped cases of one
  type pass. A named-case form (R2 (b)) removes this risk. Open.
- O15: the fuel of play (S4a R7). `visitsOf` takes a fuel, and
  `examples/play.lang` gives `|W| + 1`. A short fuel gives a list with no
  repeat, so `lassoOf` gives `none` (`null` in the JSON), not a refusal.
  Open.
- O16: the `JSON_TYPE` growth of evidence types (S4a R6). The `type` of
  `complete` is the normal form with each machine inlined: 340 bytes for 2
  agents and 2 memory states (`full`: 216 bytes). It grows with the agents
  and the machines, against the 4096-byte `type` limit. Open.
- O17: tau as a curried table (S4a R2). `stepTable` takes one `Ac`
  argument for each agent, in agent order, so its form depends on the
  agent count, and a decision is not a key. `step` on a decision is not an
  instance. A Pi over a finite function space as rows (R2 (b)) removes
  this. Open.
