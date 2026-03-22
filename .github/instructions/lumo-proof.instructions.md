---
applyTo: "**/*.proof.json"
---

This is a Lumo proof manifest file for strict-mode compilation. The compiler cross-validates every claim.

Key rules:
- NEVER modify `source_integrity` values (hashes, node counts) — they are computed by `--generate-proof-template`
- ALL `safety_properties` must have `"verified": true` with evidence
- Variable declarations must match the code exactly (names, mutation status, line numbers)
- Function declarations must match exactly (names, param counts)
- Types must be from: "number", "string", "boolean", "list", "object", "any", "never"
- Loop termination evidence required for `repeat_while` loops
- `hole_count` must be 0 for execution to be permitted

Generate a template: `./build/lumo --generate-proof-template file.lumo`
Validate: `./build/lumo --strict --check file.lumo`

See docs/proof-protocol.md and docs/proof-schema.json for the full specification.
