---
name: lumo-proof
description: Generate and validate Lumo proof manifests (.lumo.proof.json) for strict-mode compilation. Triggers when user asks about proofs, verification, strict mode, or when working with .proof.json files.
user-invocable: true
allowed-tools: Read, Write, Edit, Bash, Grep, Glob
argument-hint: "[source-file.lumo]"
---

# Lumo Proof Manifest — AI Generation Guide

When Lumo runs in `--strict` mode, it requires a `.lumo.proof.json` file alongside the source. This proof is a structured JSON document where the AI proves the code is correct, safe, and aligned with patterns. The compiler **cross-validates** every claim — it does NOT blindly trust the proof.

## Workflow

```
1. Write or receive a .lumo file
2. Run: ./build/lumo --generate-proof-template program.lumo > program.lumo.proof.json
3. Fill in all safety_properties with verified: true and provide evidence
4. Fill in type_assertions for every expression
5. Fill in contracts for functions and loops
6. Run: ./build/lumo --strict --check program.lumo
7. If verification fails, read the errors and fix the proof
8. Repeat until VERIFIED
```

## What the Compiler Checks Mechanically (cannot lie about)

- **Source SHA-256 hash** — recomputed and compared
- **AST node count** — counted independently
- **AST structure hash** — DFS traversal hash compared
- **Variable names and existence** — must match exactly 1:1
- **Function names and parameter counts** — must match exactly
- **Mutation status** — compiler tracks every `set`/`put`/`read` independently
- **Variable types the compiler can infer** — number, string, boolean, list, object
- **Literal division by zero** — caught before proof is loaded
- **Literal negative indices** — caught before proof is loaded
- **Unresolved holes** — `???` count compared
- **Unused variables** — compared to compiler's own analysis
- **Unreachable code** — statements after `return` detected

## What the Compiler Accepts as Attestation (AI must reason about)

- Types the compiler cannot infer (`get` results, function return types)
- While-loop termination arguments
- Loop invariants
- Function pre/postconditions
- Non-literal division safety
- Non-literal bounds safety

## Proof JSON Structure

```json
{
  "lumo_proof_version": "1.0.0",

  "source_integrity": {
    "source_file": "program.lumo",
    "source_sha256": "<sha256 hex>",
    "source_line_count": 17,
    "ast_node_count": 42,
    "ast_structure_hash": "<sha256 hex>"
  },

  "declarations": {
    "variables": [
      {
        "name": "i",
        "declared_at_line": 2,
        "type": "number",
        "is_mutated": true,
        "mutation_lines": [15]
      }
    ],
    "functions": [
      {
        "name": "factorial",
        "declared_at_line": 3,
        "param_names": ["n"],
        "param_count": 1
      }
    ]
  },

  "type_assertions": [
    {
      "line": 4,
      "expression": "i modulo 3",
      "asserted_type": "number",
      "reasoning": "modulo operator always produces number"
    }
  ],

  "safety_properties": {
    "no_uninitialized_access": {
      "verified": true,
      "evidence": [
        {
          "variable": "i",
          "first_use_line": 3,
          "declaration_line": 2,
          "declaration_precedes_use": true
        }
      ]
    },
    "no_division_by_zero": {
      "verified": true,
      "evidence": [
        {
          "line": 4,
          "divisor_expression": "3",
          "divisor_is_literal_nonzero": true
        }
      ]
    },
    "no_out_of_bounds_access": {
      "verified": true,
      "evidence": []
    },
    "no_infinite_loops": {
      "verified": true,
      "evidence": [
        {
          "loop_line": 3,
          "loop_type": "repeat_while",
          "termination_argument": "i increments by 1 each iteration, exits when i >= 21",
          "variant_expression": "21 minus i",
          "terminates": true
        }
      ]
    },
    "no_unresolved_holes": { "verified": true, "hole_count": 0 },
    "no_unused_variables": { "verified": true },
    "no_unreachable_code": { "verified": true }
  },

  "contracts": {
    "functions": [
      {
        "name": "factorial",
        "preconditions": ["n atleast 0"],
        "postconditions": ["return_value atleast 1"],
        "return_type": "number",
        "terminates": true,
        "termination_argument": "n decreases by 1 each recursive call, base case n < 2"
      }
    ],
    "loop_invariants": [
      {
        "loop_line": 3,
        "invariant": "i atleast 1 and i atmost 21",
        "variant": "21 minus i",
        "terminates": true
      }
    ]
  },

  "pattern_compliance": [
    {
      "pattern_id": "lumo:intent-required",
      "satisfied": true,
      "evidence": "intent declaration at line 1"
    }
  ],

  "vulnerability_analysis": {
    "input_sources": [],
    "injection_vectors": [],
    "resource_management": {
      "unbounded_allocations": false,
      "reasoning": "all lists are fixed-size literals"
    }
  }
}
```

## Step-by-Step Proof Generation

### Step 1: Get the template
```bash
./build/lumo --generate-proof-template program.lumo
```
This gives you pre-computed hashes, variable declarations, and structure. NEVER change these values.

### Step 2: Fill in `type_assertions`
For every meaningful expression in the program, add a type assertion:
- `NumberExpr` → `"number"`
- `StringLiteralExpr` → `"string"`
- `BoolLiteralExpr` → `"boolean"`
- `ListExpr` → `"list"`
- `JsonObjectExpr` → `"object"`
- `plus` with any string → `"string"`
- `minus`, `times`, `divby`, `modulo` → `"number"`
- `equals`, `above`, `below`, `atleast`, `atmost` → `"boolean"`
- `and`, `or`, `not` → `"boolean"`
- `get ... at ...` → infer from context or use `"any"`
- `call ... passing ...` → infer from function body or use `"any"`

### Step 3: Set all `safety_properties` to `verified: true`
For each property, provide evidence:

**no_uninitialized_access:** List every variable with its declaration line and first use line. Confirm declaration precedes use.

**no_division_by_zero:** For each `divby`/`modulo`:
- If divisor is a literal: `"divisor_is_literal_nonzero": true`
- If divisor is a variable: explain why it's never zero in `"guarded_by"`

**no_out_of_bounds_access:** For each `get`/`put`:
- If index is within known bounds, explain
- If guarded by a bounds check, reference the guard

**no_infinite_loops:** For each loop:
- `for_each` and `repeat_times`: `"loop_type": "for_each"` or `"repeat_times"` (inherently finite)
- `repeat_while`: provide `termination_argument` and `variant_expression`

**no_unresolved_holes:** Set `hole_count` to match actual `???` count (must be 0 for execution).

### Step 4: Add contracts for functions and loops
- Preconditions: what must be true when function is called
- Postconditions: what is guaranteed after return
- Loop invariants: what holds at every iteration entry

### Step 5: Validate
```bash
./build/lumo --strict --check program.lumo
```

Read the output. Fix any `[ERROR]` entries. Iterate until `VERIFIED`.

## Type Universe

Valid types for `type` and `asserted_type` fields:
`"number"`, `"string"`, `"boolean"`, `"list"`, `"object"`, `"any"`, `"never"`

## Variable Classification Rules

Include in `declarations.variables`:
- Every `let` declaration at program level
- Every `let` declaration inside loops and conditionals
- Pipe `yield` target variables

Do NOT include:
- Function parameters (they are internal to the function)
- `for each` iteration variables (they are internal to the loop)

## Loop Types for Evidence

| Lumo construct | `loop_type` value | Termination |
|---------------|-------------------|-------------|
| `repeat while ...` | `"repeat_while"` | Must provide argument |
| `repeat N times` | `"repeat_times"` | Inherently finite |
| `for each ... in ...` | `"for_each"` | Inherently finite |
