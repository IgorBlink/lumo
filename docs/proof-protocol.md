# Lumo Proof Protocol v1.0.0

## Overview

The Lumo Proof Protocol defines a verification system where **any AI model** must produce a structured proof manifest alongside Lumo code before the program can execute in strict mode. The compiler mechanically cross-validates the proof against the code — it does **not** blindly trust the AI.

## Workflow

```
1. AI (or human) writes program.lumo
2. AI generates program.lumo.proof.json  (or use: lumo --generate-proof-template)
3. lumo --strict program.lumo
   → reads both files
   → cross-validates proof against code
   → VERIFIED? → executes
   → FAILED?   → blocked with error report
```

## Three Trust Tiers

| Tier | Meaning | Example |
|------|---------|---------|
| **Mechanically Verified** | Compiler independently confirms | Source hash, type checks, arity, mutation status |
| **Partially Verified** | Compiler confirms structure, not full semantics | Guard conditions exist for division |
| **AI Attested** | Compiler accepts with warning | Termination arguments, complex invariants |

## Generating a Proof Template

```bash
lumo --generate-proof-template program.lumo > program.lumo.proof.json
```

This produces a skeleton JSON with all structural fields pre-populated (hashes, declarations, counts). The AI model fills in:
- `safety_properties` (set `verified: true` and provide evidence)
- `type_assertions` (type of each expression with reasoning)
- `contracts` (pre/postconditions, loop invariants)
- `pattern_compliance`
- `vulnerability_analysis`

## Proof Manifest Structure

### `source_integrity`

| Field | Type | Verified By |
|-------|------|------------|
| `source_sha256` | string | Compiler recomputes and compares |
| `source_line_count` | int | Compiler counts lines |
| `ast_node_count` | int | Compiler walks AST |
| `ast_structure_hash` | string | Compiler computes DFS hash |

If **any** integrity check fails, the entire proof is rejected immediately.

### `declarations`

Every `let` and `define` in the code must have a corresponding entry. The compiler checks:
- Variable names match
- Function parameter counts match
- Mutation status (`is_mutated`) matches actual `set` usage

Extra or missing entries = rejection.

### `type_assertions`

For each expression, declare its type. Lumo's type universe:
`number`, `string`, `boolean`, `list`, `object`, `any`, `never`

The compiler independently infers types where possible and compares. Mismatches = rejection.

### `safety_properties`

All properties must have `"verified": true`. Required properties:

| Property | What Compiler Checks |
|----------|---------------------|
| `no_uninitialized_access` | Cross-checks with static analyzer |
| `no_division_by_zero` | Verifies literal divisors; accepts guard evidence |
| `no_out_of_bounds_access` | Verifies literal indices |
| `no_infinite_loops` | Finite loops auto-verified; while loops need evidence |
| `no_unresolved_holes` | Counts `???` nodes in AST |
| `no_unused_variables` | Cross-checks with static analyzer |
| `no_unreachable_code` | Cross-checks with static analyzer |

### `contracts`

Pre/postconditions for functions, and loop invariants. These are primarily attestations but the compiler:
- Confirms referenced functions exist
- Validates preconditions reference valid parameter names
- Parses invariant expressions for syntactic validity

### `pattern_compliance`

References pattern IDs (e.g., `lumo:intent-required`). The compiler runs its own pattern checks and cross-validates.

### `vulnerability_analysis`

Documents input sources (`read` statements), injection vectors, and resource management. The compiler traces `ReadDecl` nodes and cross-checks.

## CLI Reference

| Flag | Description |
|------|-------------|
| `--strict` | Enable strict mode (requires proof) |
| `--check` | Strict + no execution (verify only) |
| `--proof <file>` | Explicit proof path (default: `<source>.proof.json`) |
| `--patterns <file>` | Load additional pattern definitions |
| `--generate-proof-template` | Output skeleton proof JSON |
| `--validate` | Syntax-only check (no proof needed) |
| `--ast` | Print AST |

## For AI Model Authors

To produce valid proofs:

1. Parse the Lumo source mentally or with a tool
2. Compute SHA-256 of the exact source text
3. Count AST nodes (every expression, statement, and structural node)
4. For every variable: determine type, mutation status, and mutation lines
5. For every expression: determine its type with reasoning
6. For every safety property: provide evidence or an argument
7. For every loop: provide termination evidence
8. Validate your proof: `lumo --strict --check program.lumo`

The proof schema is defined in `docs/proof-schema.json`.
