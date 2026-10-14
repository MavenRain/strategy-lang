# strategy-lang

strategy-lang is a language for concurrent game structures and one-goal
strategy logic (SL[1G]) sentences. A program declares the states, the
agents and the actions, the transitions, the strategies (Mealy machines)
and the goals (LTL). `langc` checks the program and writes one JSON
document. Proofs are data: the document writes each proof and each
evidence tree. The language comes from lang-template with the tcc-json
host (TinyCC).

## Commands

| Command | Result |
|---|---|
| `langc check PROG` | Checks the program. Prints `ok`. |
| `langc eval PROG NAME [ARGS...]` | Applies `NAME` to the literal arguments and prints the normal form. |
| `langc build PROG [-o OUT]` | Checks the program and writes the JSON document of each instance. |

Exit 0 is ok, exit 1 is a refusal, exit 2 is a usage or IO error.

- `make`: builds `build/langc`.
- `make check`: the gate. It builds the compilers, checks the sources with
  clang and runs `test/gate.sh`.

## Slices

The slices S0 to S7 are DONE. `docs/STATUS.md` gives each slice and its
tests.

## Documents

- `SPEC.md`: the specification.
- `design/DESIGN.md`: the domain design. It is the only source of the core
  types and the core operations.
- `formers/FORMERS.md`: the type formers. `formers/tcc-json.md` gives their
  status on the tcc-json host.
- `probe/CAPABILITY.md`: the host probe.
- `docs/PLAN.md`: the rulings, the encoding of the design, the slices S0 to
  S7 and the open items.
- `docs/STATUS.md` and `docs/VALIDATION.md`: the status and the gate
  results.
- `docs/host/README.md`: how to build and test the host kit, the JSON
  document and the limits.

The open items are in `docs/PLAN.md` section 6.

## License

MIT OR Apache-2.0 (`LICENSE-MIT`, `LICENSE-APACHE`).
