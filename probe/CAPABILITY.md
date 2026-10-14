# Host capability probe: tcc-json

Date: 2026-10-14. Host tree: this repository at `70d4ba1`, read only.
Driver: `build/langc` (built 2026-10-14 by `make check`). This file gives
the facts that the repository already records (the host README, STATUS
and the tests). No new probe ran (S7 ruling R4 (a)).

To run the probe again: `make check` runs the gate. The gate checks each
refusal of sections 3, 4, 7 and 10 (`test/check/expect.txt`,
`test/parse/expect.txt`, `test/emit/expect.txt`). `probe/guard.py` stops
a command above a resident memory limit or a time limit; S7 did not use
it. The largest process in the gate is `build/proof-regressions`, with a
peak of 394477568 bytes (376 MiB).

## Questions

Each host must answer these questions. Each answer is one numbered section
below. An answer sets the status of a former in `formers/tcc-json.md`.

| Question | Section | Formers |
|---|---|---|
| Which commands check a file, print a normal form, build and run? | 1 | all |
| Does the host take several files, or must the driver join them? | 2 | all |
| Which recursion does the host accept, and how does it refuse the rest? | 3 | F6, F7 |
| Which `Nat` primitives exist? Does `Nat` have an eliminator? | 4 | F6, F7 |
| How does a result leave the host? | 5 | target |
| What do check and build cost (time and resident memory) at N = 100 and N = 1000? | 6 | all |
| Can a family take a parameter? Can a constructor of such a family appear in a term? | 7 | F4, F12, F15 |
| Can a program project a Sigma? Does the kernel accept Sigma eta? | 8 | F11 |
| How does a program use equality: transport, cong, an index type? | 9 | F12, F13 |
| How does the host report an axiom? | 10 | R2 |

## 1. Commands

| Job | Command | Result |
| --- | --- | --- |
| Check | `build/langc check PROG` | exit 0 and `ok`; exit 1 and `langc: CODE: NAME: message` on stderr |
| Print normal form | `build/langc eval PROG NAME [ARGS...]` | the normal form of `NAME` applied to the literal arguments, on stdout |
| Build | `build/langc build PROG [-o OUT]` | one JSON document on stdout, or in the file `OUT` |
| Run | none | the JSON document is the result; there is no run step |

- Exit 0 is ok, exit 1 is a refusal, exit 2 is a usage or IO error. A
  refusal is one stderr line. A refused build writes nothing
  (`docs/host/README.md` `## Commands`).
- One program file for each run.

## 2. Several files

A run reads one program file. The domain joins at build time: the Makefile
runs `gen/embed.c` on `domain/domain.lang` and writes `build/domain.c`,
which `build/langc` links. Thus a program uses the domain names with no
import. A test domain in `test/emit/` gives a separate compiler
(`build/langc-truncation`, `build/langc-path-field`,
`build/langc-param-rec`).

## 3. Recursion and totality

A program cannot recurse by name. The checker refuses `def rec`, a use of
the definition itself and a use of a later definition with `REFUSE_REC`
(`test/check/rec.lang`, `test/check/self.lang`, `test/check/later.lang`).
Recursion comes only from `fold` (Nat, List and each family), `unfold` into
List with a Nat step limit, `elim` on an enumeration and `elimList`. The
evaluator stops at a depth of 2000 and after 20,000,000 steps for each
instance (`docs/host/README.md` `## Limits`).

## 4. Nat

`Nat` is an unsigned 64-bit integer. The primitives are in the table at
`src/front/check.c:126`. A Nat operation that overflows traps:
`langc eval` prints `trap`, and `langc build` refuses the instance with
`EVAL_OVERFLOW` (`test/emit/trap.lang`). A literal above 2^64 - 1 is
refused with `LEX_NAT_RANGE`. `Nat` has an eliminator: `fold step start n`
applies `step` n times.

## 5. How a result leaves the host

`langc check` prints `ok` on stdout. `langc eval` prints a normal form on
stdout. `langc build` writes one JSON document on stdout, or in `OUT` with
`-o`. The document is the only output of a program.

## 6. Cost

Cost at N = 100 and N = 1000 was not probed (S7 ruling R4 (a)).

| Step | N = 100 | N = 1000 |
| --- | --- | --- |
| Check | not probed | not probed |
| Build | not probed | not probed |
| Run | none | none |

The measured cost of the whole gate (`make -B check`: the build, 20
examples and all the tests) is 5.74 s wall, 1.36 s user and 1.33 s system (session 30, zsh `time`). The
largest process of the gate is `build/proof-regressions`, with a peak of
394477568 bytes (376 MiB).

## 7. Parametric families

A domain family can take a type parameter, and its constructors can appear
in a term: `Pth S`, `Mealy M S A` (S4a, S4b). S5 adds parametric recursive
domain families. The build refuses a wrong recursive domain family with
parameters with `DOMAIN_FAMILY` (`test/emit/param-rec.domain`). A program
family has no parameter (`REFUSE_DATA`, `test/check/enum-param.lang`).

## 8. Sigma projection and eta

`witness` and `payload` check (`examples/sigma.lang`). The kernel accepts
Product and Sigma eta (`docs/host/README.md` `## Origin`). A projection in
a family with 2 or more constructors is refused (O18).

## 9. Equality

A program writes `Eq A x y` with `refl`, `symm`, `trans`, `cong` and
`transport` (`examples/equality.lang`). One family `Eq` serves each type.
A domain HIT adds path constructors (`examples/paths.lang`). No proof
reduces by irrelevance: the JSON writes each proof as a tree.

## 10. Axiom detection

The grammar parses `axiom NAME : TYPE`, and the checker refuses it in a
program with `REFUSE_AXIOM` (`test/check/axiom.lang`). Thus a program that
checks has no axiom, and there is no separate axiom report.

## Consequences

1. Each run takes one file, and the domain is part of the executable
   (section 2).
2. A program recurses only through `fold`, `unfold`, `elim` and `elimList`,
   and each play takes a fuel (sections 3 and 4).
3. A Nat overflow is a build refusal, not a wrong value (section 4).
4. The JSON document is the only output; a proof leaves the host as a tree
   (sections 5 and 9).
5. Parametric families are in `domain/` only (section 7, rule R1).
6. A program cannot add an unproved fact (section 10).
