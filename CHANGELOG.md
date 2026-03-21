# Changelog

All notable changes to Lumo are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Lumo uses [Semantic Versioning](https://semver.org/).

---

## [0.2.0] — 2026-03-21

### Added

**Standard library — built-in functions (called via `call name passing ...`)**

- **String**: `length`, `charat`, `substr`, `split`, `join`, `contains`, `replace`, `trim`, `uppercase`, `lowercase`, `charcode`, `fromcode`, `indexof`, `startswith`, `endswith`
- **List**: `push` (mutates in place), `pop`, `concat`, `slice`, `reverse`, `range`
- **Type**: `typeof` → `"number"|"string"|"boolean"|"list"|"object"`, `tostring`, `tonumber`, `tobool`
- **Math**: `floor`, `ceil`, `round`, `abs`, `sqrt`, `power`, `min`, `max`
- **Object**: `keys`, `values`, `haskey`, `entries`
- **I/O**: `readfile` — reads entire file as a string

**String indexing via `get`**: `get "hello" at 2` → `"l"` (in addition to list and object access)

These additions make Lumo self-hosting–capable: a full Lumo tokenizer, parser, and interpreter can now be written in Lumo itself.

---

## [0.1.1] — 2026-03-21

### Added

- VS Code extension (`vscode-extension/`) with syntax highlighting, real-time linting, and file icons for `.lumo` files
- Language logo: icon mark (`assets/logos/lumo-icon.svg`) and wordmark (`assets/logos/lumo-wordmark.svg`)
- Dijkstra's shortest-path algorithm as a full example program (`examples/dijkstra.lumo`)

### Changed

- README rewritten with full language reference table, extension install instructions, and example index

---

## [0.1.0] — 2026-03-21

### Added

- Turing-complete interpreter written in C++17
- Symbol-free arithmetic: `plus`, `minus`, `times`, `divided`, `modulo`
- Comparison operators: `equals`, `above`, `below`, `not`
- Boolean logic: `and`, `or`
- Variable declaration and mutation: `let` / `set`
- Conditionals: `if … then` / `elif` / `else` / `end`
- Pattern matching: `match` / `when` / `catch error yield`
- Loop constructs: `repeat while`, `repeat N times`, `for each … in`
- Function definitions and calls: `define` / `call … passing`
- List literals, indexing, and mutation: `list`, `get … at`, `put … at … be`
- Pipelines: `pipe` / `start with` / `filter` / `map` / `yield`
- Intent annotations: `intent "description"`
- First-class holes: `???`
- Explicit no-op: `skip`
- Input: `read`
- AST debug mode: `--ast`
- Syntax validator: `--validate`
- Formal EBNF grammar in `docs/grammar.ebnf`
- Language reference in `docs/language-reference.md`
- Example programs: hello, arithmetic, fizzbuzz, loop, functions
- CMake build support
