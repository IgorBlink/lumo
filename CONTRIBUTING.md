# Contributing to Lumo

Thank you for your interest in contributing. This document covers everything you need to get started.

---

## Building Locally

**Requirements:** C++17 compiler, CMake 3.10+

```bash
cmake -S . -B build
cmake --build build
./build/lumo examples/hello.lumo
```

Alternatively with clang++ directly:

```bash
clang++ -std=c++17 -Iinclude src/*.cpp -o lumo
./lumo examples/hello.lumo
```

---

## Project Structure

```
include/      C++ header files (lexer, parser, AST, interpreter)
src/          C++ implementation files
examples/     Runnable .lumo example programs
tests/        Test .lumo programs
docs/         Language reference and formal grammar
```

---

## Code Style

- C++17 throughout
- Headers in `include/`, implementations in `src/`
- Class and function names use `PascalCase` for types, `camelCase` for methods
- Keep the interpreter, parser, lexer, and AST strictly separated — no cross-layer dependencies in the wrong direction

---

## Making Changes

1. Fork the repository and create a branch from `main`
2. Make your changes
3. Verify the build compiles cleanly with no warnings
4. Run the examples to confirm nothing regressed:
   ```bash
   ./lumo examples/hello.lumo
   ./lumo examples/fizzbuzz.lumo
   ./lumo examples/arithmetic.lumo
   ./lumo examples/loop.lumo
   ./lumo examples/functions.lumo
   ```
5. Open a pull request against `main` with a clear description of what changed and why

---

## Reporting Issues

Use [GitHub Issues](../../issues). Include:

- A minimal `.lumo` program that reproduces the issue
- The command you ran
- The actual output
- The expected output

---

## Language Design Changes

Changes to Lumo's syntax or semantics require updating three places in sync:

1. `docs/grammar.ebnf` — the formal grammar
2. `docs/language-reference.md` — the human-readable spec
3. The interpreter in `src/` and `include/`

Proposals that update only one or two of these will not be merged until all three are consistent.

---

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
