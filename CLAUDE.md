# Lumo Project — Claude Code Instructions

## What is Lumo?

Lumo is a strict, LLM-native programming language implemented in C++17. Every operator is a plain English word — no symbols in program logic. It includes an AI-verified proof system where any AI model must produce a structured proof manifest before code can execute in strict mode.

## Build

```bash
cmake -S . -B build && cmake --build build
```

## Test

Run all examples to verify nothing is broken:
```bash
for f in examples/hello.lumo examples/arithmetic.lumo examples/fizzbuzz.lumo examples/loop.lumo examples/functions.lumo examples/dijkstra.lumo; do ./build/lumo "$f" > /dev/null 2>&1 && echo "PASS: $f" || echo "FAIL: $f"; done
```

Test strict mode with existing proofs:
```bash
./build/lumo --check examples/hello.lumo
./build/lumo --check examples/fizzbuzz.lumo
```

## Key Rules for This Project

1. When writing `.lumo` files: use English words, not symbols. See @docs/language-reference.md
2. When modifying the compiler (C++): all new AST passes must implement the `ASTVisitor` interface
3. Language changes require three-part sync: `docs/grammar.ebnf` + `docs/language-reference.md` + implementation
4. C++ style: PascalCase for classes, camelCase for methods, C++17 standard
5. Proof manifests: use `--generate-proof-template` to get hashes, then fill in safety proofs
6. Never use `value` as a variable name in Lumo — it is the pipe register

## Skills Available

- `/lumo-language` — Full Lumo syntax reference and code generation guide
- `/lumo-proof` — Proof manifest generation and verification guide

## Architecture

```
Source → Lexer → Parser → AST → [StaticAnalyzer] → [ProofVerifier] → [PatternChecker] → Interpreter
```

Static analysis, proof verification, and pattern checking only run in `--strict` mode.
