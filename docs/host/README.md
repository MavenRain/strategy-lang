# TinyCC JSON host kit

This kit compiles a small typed language to JSON. A C front end (lexer,
parser, checker and evaluator) and a JSON writer build with TinyCC into
one executable, `build/langc`. The output is the final state of each
instance in the program. There is no IR and no lowering step.

From this kit directory, or from a generated language directory:

```sh
make check
build/langc check examples/inventory.lang
build/langc eval examples/inventory.lang stockTotal
build/langc build examples/inventory.lang -o inventory.json
```

The build uses TinyCC (`TCC`, default `tcc`). Validation uses TinyCC
0.9.28rc on AArch64 Darwin. `make check-clang` also compiles the sources
with `cc -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only` (`CC`, default
`cc`). The gate uses POSIX `sh`, `awk`, `cmp` and Node (for `JSON.parse`).
No network access or package installation is needed.

## Commands

| Command | Result |
|---|---|
| `langc check PROG` | Checks the program. Prints `ok`. |
| `langc eval PROG NAME [ARGS...]` | Applies the definition `NAME` to the literal arguments and prints the normal form. A Nat overflow prints `trap`. |
| `langc build PROG [-o OUT]` | Checks the program, evaluates each instance and writes one JSON document to stdout, or to `OUT` |

Exit 0 is success. Exit 1 is a refused program. Exit 2 is a usage or IO
error. A refusal writes one line to stderr: `langc: CODE: NAME: message`.
`langc build` makes the full document in memory before it writes. Thus a
refused build writes nothing to stdout and makes no `OUT` file.

## The JSON document

```json
{"strategy-lang":1,"instances":[{"name":"stockTotal","type":"Nat","value":70}]}
```

The key is the language name and the value is the format version, 1.
The generator reserves the name `instances` for this host to keep the two
document keys distinct.
`instances` holds one object for each instance, in source order. An
instance is a definition of the program (not of `domain/`) whose type is
data. A table and a refutation (see the type table below) are also
instances. A def is an instance when its type is a chain of zero or more
Pi types over enumerations that ends in data or in a refutation type.
These definitions are checked, but they are not instances:

- other functions (a Pi type, dependent or not). A function on a family
  with a path constructor (a HIT) is not a table, so it is not an
  instance;
- types and families (a universe type), also at the end of a chain of
  tables;
- the definitions in `domain/domain.lang`.

An equality proof (a definition of an `Eq` type) is an instance. The JSON
value keeps the proof (the no-collapse rule). The supported normal proof
forms are written as trees, as listed below. Reflexivity computation and
the fold path rule run before serialization.

`type` is the normal form of the definition type in source syntax, for
example `Sigma (n : Nat) (Eq Nat n 1200)`. Binder names are freshened when
needed to preserve scope. The complete text must fit a 4096-byte buffer;
the build refuses a type that exceeds this size or the printer depth of
200, rather than emitting an abbreviated type.
`value` is the evaluated value. Values carry no types, so the writer walks
the type and the value together (`src/json.c`). The fuel of the evaluator
starts again for each instance.

| Type | JSON value |
|---|---|
| `Nat` | A number: the full unsigned 64-bit value in decimal |
| `Flag` | `true` or `false` |
| `Unit` | `{}` |
| `Prod A B` | `{"first": a, "second": b}` |
| `Sum A B` | `{"inl": a}` or `{"inr": b}` |
| `Option A` | `null` or the value. When `A` is an `Option`, `some v` is `{"some": v}`. |
| `List A` | An array |
| `Sigma (x : A) B` | `{"witness": w, "payload": p}`. The payload type is `B` at the witness. |
| `Eq A x y` | A proof tree. A `refl` leaf is `{"refl": v}`, where `v` is the point `x` as a value of `A`. When `A` is a function type or a type, `v` is the normal form of `x` in source syntax, as a string. |
| `Eq A x y`, a path constructor | `{"path": "loop"}`. The fields of the constructor follow the name: `{"path": "glue", "from": 1, "to": 2, "rel": {"refl": 2}}` |
| `Eq A x y`, `symm p` | `{"symm": p}` |
| `Eq A x y`, `trans p q` | `{"trans": [p, q]}` |
| `Eq A x y`, `cong f p` | `{"cong": {"fn": "f", "path": p}}`, where `"f"` is the normal form of `f` in source syntax |
| `Eq A x y`, `transport P p u` | `{"transport": {"family": "P", "path": p, "value": u}}`, where `"P"` is the normal form of `P` in source syntax |
| A HIT (a family with path constructors) | A point. When no point constructor has fields, a point is the constructor name, as a string (`"base"`). Else a point is `{"tag": "name", ...}` with the fields of that constructor (`{"tag": "class", "elem": 3}`). |
| A family whose constructors have no fields (a domain family or a program enumeration) | The constructor name, as a string |
| `Empty` | None. `Empty` has no constructor, so it has no value. `absurd e` never reduces. |
| `A -> Empty` (a refutation) | `{"refute": "f"}`, where `"f"` is the normal form of the function in source syntax. The codomain must be `Empty` and not dependent. |
| A function on an enumeration (a table) | An object with one key for each constructor, in declaration order: `{"red": 1, "green": 2, "blue": 3}`. The value of a key is the function at that constructor, at the codomain for that constructor (a dependent codomain is permitted). When the codomain is a function on an enumeration, the value is a nested table. |
| A family with one constructor | An object of the fields |
| Any other family | `{"tag": "name", ...}` with the fields of that constructor |

A decision `Sum (Eq E x y) (Eq E x y -> Empty)` keeps the evidence on
each side: `{"inl": {"refl": "noon"}}` or
`{"inr": {"refute": "fun e => ..."}}`.

A path constructor field named `path` in `domain/domain.lang` is refused
at build time (`DOMAIN_FAMILY`), because `path` is the JSON key of the
path constructor name.

Other stuck proof forms are not supported, even when their type is
`Eq`. For example, this program passes `langc check`, but `langc build`
refuses `stuckProof` with `JSON_VALUE` because its fold is blocked on a
transported point:

```text
def r : Eq Nat 0 0 := refl
def stuckProof : Eq Nat 0 0 :=
  fold r refl (transport (fun (c : Circle) => Circle) loop base)
```

An indexed family uses the same rules. Its index is in `type` (for
example `Lot 3`), not in `value`.

`langc build` refuses a program with these codes:

| Code | Cause |
|---|---|
| `JSON_VALUE` | An instance holds a function that is not a table or a refutation (for example a function on a HIT, or a table entry that holds a function), a type, or a stuck term outside the supported proof-tree forms |
| `JSON_DEPTH` | An instance nests deeper than 2000 levels. A list spine does not count. |
| `JSON_TYPE` | The complete instance type exceeds the printer's size or depth limit |
| `EVAL_OVERFLOW` | A Nat operation in an instance overflows |

The check and evaluation codes are the same as in `langc check` and
`langc eval`. `test/parse`, `test/check` and `test/emit` hold a program
for each refusal.

`langc eval` also refuses an incomplete normal form with `EVAL_PRINT`
(a 65536-byte buffer and the same printer depth limit).

## The domain

`domain/domain.lang` is the sample domain. `gen/embed.c` embeds it in the
executable at build time. `domain/README.md` tells how to replace it.

A program can declare an enumeration only: a family with no parameters,
whose constructors have no fields and no path type (rule R1). The checker
refuses other program families with `REFUSE_DATA`. Only
`domain/domain.lang` can declare records, recursive families, indexed
families and HITs.

## Limits

| Limit | Value | Source |
|---|---|---|
| Source size | 1 MiB | `src/front/front.h:7` |
| Arena | 1 GiB | `src/main.c:10` |
| Parser nesting | 1000 | `src/front/parser.c:16` |
| Checker depth | 2000 | `src/front/check.c:10` |
| Evaluator depth | 2000 | `src/front/core.h:16` |
| Evaluator fuel, for each instance | 20,000,000 steps | `src/front/core.h:17` |
| JSON nesting | 2000 | `src/json.h:15` |
| `type` text | 4096 bytes | `src/json.c:7` |
| Type and normal-form printer depth | 200 | `src/front/eval.c` |

## Origin

This kit is a fork of the tcc-wasm kit front end. It does not have the
Wasm writer (`wasm.c`), the IR (`ir.h`) or the target interface
(`target.h`). It adds `src/json.c` and `src/json.h`, and `src/main.c` has
the verbs `check`, `eval` and `build`. The front end also fixes dependent
result inference, Product and Sigma eta, Sum bind error-type preservation,
domain name checking, the F14 universe rules, and complete,
scope-preserving value printing. The snapshot below records the original
fork, before these fixes. To find the changes after the fork,
compare the hashes with `shasum -a 256 src/front/*` in the tcc-wasm kit.

SHA-256 of the tcc-wasm kit sources at the fork (paths relative to that
kit):

| Path | SHA-256 |
|---|---|
| `src/front/ast.h` | `5dbe74e54c8207da17a57810624bf049d729f272c73a0765b9fd504fb3f18720` |
| `src/front/base.c` | `d994f71d11a352896212d97ed6edb5de062dc37ad6f31bf927487c4d8e675d5b` |
| `src/front/base.h` | `d36daae15e9a79ae298eadf5f1f9c3687e077c7305e63fad6cc5b477b58cebca` |
| `src/front/check.c` | `526a8207c66f3276c18767c62de05026866e7e8adc30e2c5383a2975c0a57114` |
| `src/front/check.h` | `5d781eb3f52a7a17db10d375c0fb7c5b578bf0527dd358cfe49ebb779d008d06` |
| `src/front/core.h` | `d04aa9412bb0f25d470bd3b91e3a073284b19e89aa90083e18d63a6d468fd9dc` |
| `src/front/eval.c` | `5a044b8935f77614a2321883afc8df7d2b1d745e198b8f861538dc1a61773c55` |
| `src/front/front.c` | `9b7d8890d0854fe989780431521189fabf0a79e624359733f5a90822bbf08c27` |
| `src/front/front.h` | `7eb0b8de605ad7336eac1bffc9b65587d7113b2e3849a95a9e6adf75fd12989a` |
| `src/front/lexer.c` | `6812190d933fa5dfb1ad60c9d40a46aeb460aab5342feed6c4697fe31a2425bc` |
| `src/front/lexer.h` | `85046ffc1f76878b8dc0102db08458a59fef914d7951462c9e7bf590cb5db314` |
| `src/front/parser.c` | `6220dcc67e0fc4782d60cf9516bae979d2d4eb3c5eb3e047d92b85806b0bb02d` |
| `src/front/parser.h` | `e9caa8e0dbcfc716b572fa754daa3023ee1a55fb16ff77fb0235c97a49adaa1c` |
| `src/ir.h` | `13021f895667552ec1d527267258e8a3437cadde74744a348e1219dff6d773b9` |
| `src/main.c` | `2294beb677fb4c208811937bbf48f1570911de6e52a6068874e90b3489b211ba` |
| `src/target.h` | `077dff69391f69dbd4623858553dee7b5547a8c1edc0c75660e0aede9b15e429` |
| `src/wasm.c` | `4f69fba5ea1f335e95c7f030d970c4e89da84ee13c57d95879137cf96d8235bd` |

`FORMERS.md`, copied to `formers/tcc-json.md` in a generated language,
gives the status of each former. `docs/CAPABILITY.md` gives the host
facts.
