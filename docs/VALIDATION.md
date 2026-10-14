# Validation

Date: 2026-10-14. TinyCC: 0.9.28rc 2026-09-04 mob@0fb54300 (AArch64
Darwin). Apple clang: 21.0.0. Host executable: `build/langc` from the
sources of `70d4ba1` (S6), the last commit that changes `src/` or
`domain/`. S7 changes the docs only.

`make check` passes. It builds `build/langc` with TinyCC
(`-std=c99 -Wall -Werror`), the three test compilers
`build/langc-truncation`, `build/langc-path-field` and
`build/langc-param-rec`, and `build/proof-regressions`. It checks the
sources with clang (`-Wswitch-enum`, `check-clang`). Then `test/gate.sh`
runs the tests with zero failures: examples 20 checked, 0 failed; parse refusals 9; check refusals 67; evals 57; json 20; build refusals 10; gate 0 failures; regressions 6 checked, 0 failed; printer regressions 2; proof regressions 25 checked, 0 failed. The final gate (`make -B check`, session 30) took
5.74 s wall, 1.36 s user and 1.33 s system (zsh `time`). The largest process is `build/proof-regressions`,
with a peak of 394477568 bytes (376 MiB).

The base commit `efdb21d` (S0) makes the language from lang-template with
the tcc-json host. Its tests check the formers F1 to F15 on the kit sample
domain.

S1 (`b40e45d`) adds 9 test files: 7 check refusals, 1 JSON golden and
`examples/proofs.lang`. They check the no-collapse rule: the conversion
and the reducer do not identify proofs, each `Eq` def is an instance, and
the JSON writes each proof as a tree. The refusals cover a proof that
checks only by irrelevance. Gate: examples 11, check refusals 31, evals
25, json 11, build refusals 2.

S2 (`37f8808`, docs `a9edaf3`) adds 11 test files: 6 check refusals, 2
test domains in `test/emit/`, 1 JSON golden and `examples/paths.lang`, and
`test/proof-regressions.c`. They check HITs, path constructors, `cong` on
a path and the path cases of a fold. The refusals cover a missing path
case, a path case of the wrong type and a truncation constructor
(`DOMAIN_TRUNCATION`). Gate: examples 12, check refusals 37, evals 28,
json 12, build refusals 4, proof regressions 9.

S3 (`acbf60d`) adds 16 test files: 12 check refusals, 2 test domains, 1
JSON golden and `examples/enums.lang`. They check program enumerations,
`elim`, `Empty`, `absurd`, refutations and tables. The refusals cover a
program family that is not an enumeration, a wrong case count, an `elim`
on a family that is not an enumeration, a wrong motive, an `absurd` with
no hint and a table on a HIT. Gate: examples 13, check refusals 49, evals
32, json 13, build refusals 6, proof regressions 11.

S4a (`2b3d692`) adds 7 test files: 1 check refusal, 2 test domains, 2 JSON
goldens, `examples/game.lang` and `examples/play.lang`. They check the
decision and transition tables, tracks, Mealy strategies, assignments with
completeness evidence and the play to a lasso. The refusals cover a
projection with parameters, a transition inside data and a machine on
`Unit` memory. Gate: examples 15, check refusals 50, evals 36, json 15,
build refusals 8, proof regressions 17 (STATUS records 11 at the S3 gate
and 17 at the start of S4a).

S4b (`8953207`) adds 5 check refusals. They check `elimList`, the paths
`unroll` and `unrollOne`, the path cases of `cycLen` and the projection
guard (O18). Gate: examples 15, check refusals 55, evals 37, json 15,
build refusals 8, proof regressions 19.

S5 (`f60236d`) adds 8 test files: 4 check refusals, 2 test domains, 1
JSON golden and `examples/goals.lang`. They check `Ltl`, the evidence
`Sat`, the dual goal, the verdict and the refutation. The refusals cover
false evidence, an `elim` on `Ltl`, a fold for the evidence, the evidence
on the `Pth` class, the verdict `type` limit and a recursive domain
family with parameters. Gate: examples 16, check refusals 59, evals 41,
json 16, build refusals 10, proof regressions 20.

S6 (`70d4ba1`) adds 12 test files: 6 check refusals, 3 JSON goldens,
`examples/pennies.lang`, `examples/traingate.lang` and
`examples/shared.lang`. They check the Flag model check `holds`, the
search `findFirst`, `exists` and `forall` sentences and the class `Cls`.
The refusals cover a core name, a wrong witness, a wrong `forall` row, two
agents with two variables, an incomplete assignment and a witness that is
not a constructor. The 5 sentences with two quantifiers are check-only
proof regressions (O24). Gate: examples 19, check refusals 65, evals 49,
json 19, build refusals 10, proof regressions 25.

The S6 review (in `70d4ba1`) adds 4 test files: `examples/playbounds.lang`
(8 evals), `test/json/playbounds.json`, `test/check/s6-short-play.lang`
and `test/check/s6-zero-play.lang` (both `TYPE_INFER`). They check zero
fuel, short fuel, exact closure, extra fuel and a singleton cycle. Both
refusals checked before the `splitOf` fix. Gate: examples 20, check
refusals 67, evals 57, json 20, build refusals 10, proof regressions 25.

## Mutants

Each mutant changes the source, runs `make -B check` once, and restores
the source. Each kill is a test. `docs/STATUS.md` gives each mutant row.

| Slice | Mutants | Result |
|---|---|---|
| S1 | M1 to M5, M7 | 6 KILLED |
| S1 | M6 (F1 reverted) | SURVIVED (expected, no S1 test) |
| S2 | M6, M8 to M12 | 6 KILLED |
| S3 | M13 to M22 | 10 KILLED |
| S4a | M28 to M30 | 3 KILLED |
| S4b | M23 to M27, M31 | 6 KILLED |
| S5 | M32 to M39 | 8 KILLED |
| S6 | M40 to M47 | 8 KILLED |

Total: 47 KILLED. M6 survived in S1, and S2 kills it. S7 runs no mutant
(ruling R8 (a), docs only).
