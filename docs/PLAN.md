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
| Path | `Pth` | A lasso (a stem track and a cycle that is not empty), as a quotient by unrolling (section 4). DONE (S4a, S4b): the domain HIT `Pth S` with the point constructor `lasso (lstem) (lhead) (ltail)` and the path constructors `unroll` and `unrollOne`. The cycle is `lhead` and then `ltail`. A function on `Pth` gives its path cases with `elimList` proofs (S4b R3). | S2, S4a, S4b |
| Strategy | `Str = Trk -/-> Ac` | `Trk -> Option Ac`. Strategies are made from Mealy machines (a coalgebra). The meaning on a track is a fold. S4a done: the domain record `Mealy M S A` and the domain def `mealyAct` (a right fold). | S4a |
| Assignment | `Asg = (Var + Ag) -/-> Str` | A list of bindings. Lookup gives `Option Str`. A complete assignment is a Sigma of the list and the evidence that each agent has a strategy. S4a done for agents (`bindings`, `lookupAg`, `complete` and `full` in `examples/play.lang`). S6 done for variables (R9): the binding `bnd : Ag -> Var`, the map `asg` from a variable to a machine, the list `asgL` with the evidence `completeV` (each variable) and `completeAg` (each agent), and `fullV` in `examples/pennies.lang`. | S4a, S6 |
| Play | `play : Asg * St -> Pth` | An unfold of the product machine from the state. It stops at the first repeated machine state and gives the lasso. S4a done: the domain helpers `visitsOf` and `lassoOf`, and in the program `stepW`, `codeW`, `play : Option (Pth W)` and the closing evidence `closes` (O15). | S4a |
| Principal sentence | `quantifiers, bindings, goal` | A quantifier prefix (exists or forall over strategy variables), a binding list (agent to variable) and an LTL goal. S6 done: `exists` is a `Sigma` on a strategy index with the witness index (R7), `forall` is a function on the index into a table motive, and the family `Cls` names the strategy class (R6). A sentence with two quantifiers checks, but the build refuses it (O24). | S6 |
| Satisfaction | `G, chi, s |= phi` | An inductive family of evidence on plays. A verdict is a Sum of the evidence and the refutation evidence. S5 done: the domain family `Ltl` (an atom is a `Nat` code), the evidence type `Sat` (a large fold on `Ltl`), the verdict `Sum (Sat goal p) (Sat (dual goal) p)`, and the refutation as the evidence of the dual goal, in `examples/goals.lang`. | S5, S6 |

The design gives no labelling of states. The goal atoms need one. S5
(R1 (a)): an atom is a `Nat` code. The program declares an atom
enumeration `Ap`, a def `label : St -> Ap -> Flag`, and a def `labN` from
the code to the flag. An unknown code gives `flagNo` (O6 CLOSED, O19).

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
  one path apart. S4b gives two path constructors, because the head of
  the cycle is a field: `unroll` for a cycle of 2 or more states (`cs` not
  empty), and `unrollOne` for a cycle of 1 state (`cs` empty). A function
  from `Pth` to another type (`Nat`, `St`) gives each path case as an
  `elimList` proof, because the case needs a fact about `snoc` on a
  variable list (`cycLen` in `examples/game.lang`). A function
  `Pth W -> Pth W` that changes only the stem needs no `elimList`: the
  path constructors are its path cases (S4b P2b). `lassoOf` gives the
  canonical split (the first repeat). A value that depends on the split,
  such as the head of the cycle, reads the split (`lassoParts`), not the
  class (S4b Q10).

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
- S4b: the `Pth` quotient. DONE. R3 (a): the dependent `List` eliminator
  `elimList` (`OP_ELIM_LIST`), the only kernel change of S4. R4 (a): the
  paths `unroll` and `unrollOne` and the domain def `snoc`. R9 (a):
  `lassoOf` is a `map` over the split `lassoParts`. R10 (a): `headW` reads
  the split. R11 (a): a test pins the projection guard (O18). `cycLen` in
  `examples/game.lang` is a function on `Pth` with its path cases. `Pth`
  has more than one constructor, so the goldens write a lasso as
  `{"tag":"lasso",...}`. The mutants M23 to M27 and M31 are KILLED. See
  `docs/STATUS.md` `## S4b the Pth quotient`.
- S5: LTL goals and satisfaction evidence on lassos (next, until, release,
  and the derived forms), with refutation evidence. DONE (rulings R1 to R7
  (a), R8 (b), 2026-10-14), with no kernel change: the domain family `Ltl`
  (an atom is a `Nat` code), the evidence type `Sat` (a large fold on
  `Ltl`, with the fuel folds `untilN` and `releaseN`), the positions on the
  split of the lasso (`wordOf`, `lenOf`, `lassoNext`, `lassoAt`), the dual
  goal `dual`, and the verdict and the refutation in the new program
  `examples/goals.lang`. Refusal tests: false evidence, an `elim` on
  `Ltl`, a fold for the evidence, the evidence on the `Pth` class, the
  verdict `type` limit, and a recursive domain family with parameters. The
  mutants M32 to M39 are KILLED. See `docs/STATUS.md`
  `## S5 LTL goals and evidence`.
- S6: principal sentences. DONE (rulings R1 to R9 (a), 2026-10-14), with
  no kernel change: the Flag model check `holds` (with `holdsUntil` and
  `holdsRelease`), the search `findFirst`, the play helpers `lastIn`,
  `splitOf`, `SatPlay` and `holdsPlay`, and the class family `Cls` in the
  domain. A positional strategy is a machine on the memory `Solo` with an
  index decoder (O2). The new programs are `examples/pennies.lang`
  (matching pennies), `examples/traingate.lang` (a train and a gate
  controller) and `examples/shared.lang` (one strategy shared by two
  agents, which ATL* cannot express). `exists` takes a witness index, or
  the search gives the index. `forall` gives a table of evidence, with one
  row for each positional strategy. Refusal tests: a core name, a wrong
  witness, a wrong `forall` row, two agents with two variables, an
  incomplete assignment, and a witness that is not a constructor. The 5
  sentences with two quantifiers are check-only proof regressions (O24).
  The mutants M40 to M47 are KILLED. See `docs/STATUS.md`
  `## S6 principal sentences`.
  Review hardening: `splitOf` preserves failure as `none`. Without a
  closed cycle, `SatPlay` is `Empty` and `holdsPlay` is `flagNo`, so short
  fuel cannot substitute a loop at the initial state. The new
  `examples/playbounds.lang` and two check refusals cover zero and short
  fuel, exact closure, extra fuel and a singleton cycle. A `flagNo` from
  an incomplete play is a failed check, not refutation evidence.
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
  S6 (R5): a positional strategy is a machine on the memory `Solo` (one
  state) with an index decoder (`dec`, `pos`), so an index family names
  the positional strategies of an agent. The domain family `Cls`
  (`positional` or `memory`) names the class in a verdict (R6). In S6 only
  the verdict `vC` has the marker, and `vC` is check-only (O24).
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
  CLOSED (S5, R1 (a)): an atom is a `Nat` code, and the program gives the
  labelling (`label` and `labN` in `examples/goals.lang`).
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
  only. The S4b `elimList` is a dependent eliminator for `List` only. The
  HIT fold stays not dependent, so O11 stays open.
- O12: a stuck `elim` prints as `fold` (S3 Risk 4). The core term of
  `elim` drops the motive `P`. `P` is a type, not a proof, so no proof is
  lost. But the printed text does not check again, because `fold` is not
  dependent. Printed text occurs in `refl` points (O8) and in `refute`
  strings (S3 R5). Open. S4b: the core term of `elimList` also drops `P`,
  so a stuck `elimList` prints as `elimList` with no motive, for example
  `fun xs => elimList 0 (fun x => fun ys => fun n => natAdd n 1) xs`.
  This text does not check again, because `elimList` takes the motive as
  its first argument.
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
- O18: no projection in a family with 2 or more constructors (S4b R11).
  A field name of such a family, for example `lhead` of `Pth`, is not a
  name. Thus `lhead p` gives `NAME_UNKNOWN`, with no hint to use `fold`.
  Test: `pth-field`. Mutant: M31. A refusal code for a field name
  (R11 (b)) removes this. Open.
- O19: an atom is a `Nat` code (S5 R1). `Ltl` has no parameters, because
  the kernel refuses a recursive domain family with parameters
  (`DOMAIN_FAMILY`, test `param-rec.domain`). Thus the checker does not
  check an atom code against the atom enumeration. In `labN`, an unknown
  code gives `flagNo`. Open.
- O20: the evidence is written by hand (S5 R4). There is no dependent
  eliminator on `Ltl` (`ltl-elim`, `ELIM_ENUM`), and a fold does not give
  the evidence of each formula (`sat-fold`, `TYPE_INFER`). Open.
- O21: the evidence is not invariant on the `Pth` class (P6.1, P6.2). A
  function on the class into a type is refused (`sat-class`,
  `TYPE_REFL`). The lassos `p1` and `p2` of one class have different
  evidence trees (`ev1` and `ev2`). Open.
- O22: the verdict `type` grows with the goal and `n` (S5 R6). The `type`
  of the `G F` verdict is 3094 bytes at `n = 5` and 4336 bytes at
  `n = 9`. The second is more than the 4096-byte limit, so the build
  refuses it with `JSON_TYPE` (`verdict-type.lang`). Open.
- O23: the release evidence is sound only when `n` is at least the count
  of reachable positions (S5 R3). With a smaller `n`, the evidence does
  not reach each position, so the evidence of a false goal can check.
  Open.
- O24: the sentences with two quantifiers check, but their type text (the
  normal form) is more than `JSON_TYPE_MAX` (4096 bytes), so the build
  refuses them with `JSON_TYPE` (S6 R1 (a)). They are check-only proof
  regressions with no golden: 5 cases in `test/proof-regressions.c`
  (`fyEx2`, `exFyRef2`, `vA2`, `vC`, `tgSafe2`). A case uses a 1 GiB
  arena, as `langc` does. Open.
- O25: the search is not evidence (S6 R8). `found` comes from the Flag
  model check `holds`, and the evidence `byFound` is a separate proof
  term. No term proves that `holds` gives `flagYes` only when `Sat` has
  evidence. Open.
- O26: the `memory` class has no S6 example. Each S6 sentence quantifies
  over positional strategies. Open.
