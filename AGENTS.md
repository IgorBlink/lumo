# Lumo — AI Agent Instructions

## What is Lumo?

Lumo is a Turing-complete interpreted programming language where **every operator and keyword is a plain English word**. No symbols (`+`, `>`, `==`, `{`, `&&`) appear in program logic. It is implemented in C++17 and designed for LLM-native code generation.

## Repository Layout

```
include/          C++ headers (lexer, parser, AST, interpreter, analyzer, verifier)
src/              C++ implementation (~3500 LoC)
examples/         Example .lumo programs with proof manifests
docs/             Language reference, EBNF grammar, proof protocol, JSON schema
vendor/           Vendored dependencies (SHA-256)
.claude/          Claude Code skills
.cursor/          Cursor rules
```

## Build & Run

```bash
cmake -S . -B build && cmake --build build
./build/lumo program.lumo                           # execute
./build/lumo --validate program.lumo                # syntax check
./build/lumo --strict program.lumo                  # require proof manifest
./build/lumo --generate-proof-template program.lumo # output proof skeleton
./build/lumo --check program.lumo                   # strict verify without executing
```

## Lumo Syntax — Quick Reference

Lumo uses English words instead of symbols. Here is the complete mapping:

| Instead of | Write | Example |
|-----------|-------|---------|
| `+` | `plus` | `a plus b` |
| `-` | `minus` | `a minus b` |
| `*` | `times` | `a times b` |
| `/` | `divby` | `a divby b` |
| `%` | `modulo` | `a modulo b` |
| `==` | `equals` | `a equals b` |
| `>` | `above` | `a above b` |
| `<` | `below` | `a below b` |
| `>=` | `atleast` | `a atleast b` |
| `<=` | `atmost` | `a atmost b` |
| `&&` | `and` | `a and b` |
| `\|\|` | `or` | `a or b` |
| `!` | `not` | `not a` |
| `=` | `let x be` / `set x be` | `let x be 5` |
| `{ }` | `... end` | `if x then ... end` |
| `def f(x)` | `define f taking x ... end` | see below |
| `f(x)` | `call f passing x` | `let y be call f passing 3` |
| `arr[i]` | `get arr at i` | `get nums at 0` |

### Program Structure

Every program starts with `intent`. Variables are declared with `let`, mutated with `set`. All blocks end with `end`.

```lumo
intent "Description of what this program does"

let x be 10
set x be x plus 1

if x above 5 then
  print "big"
else
  print "small"
end
```

### Functions

```lumo
define add taking a, b
  return a plus b
end

let sum be call add passing 3, 4
print sum
```

Functions have isolated scope — they can read outer variables but cannot mutate them.

### Loops

```lumo
# While loop
let i be 0
repeat while i below 10
  print i
  set i be i plus 1
end

# Count loop
repeat 5 times
  print "hello"
end

# For-each
let items be list 1, 2, 3
for each item in items
  print item
end
```

### Lists and Objects

```lumo
let nums be list 1, 2, 3
let first be get nums at 0
put nums at 0 be 99

let user be { "name": "Alice", "age": 30 }
print get user at "name"
put user at "age" be 31
```

### Pipes

```lumo
let nums be list 1, 2, 3, 4, 5
pipe process
  start with nums
  filter when value modulo 2 equals 0
  map value times 10
  yield evens
end
print evens
```

### Match (must end with catch error)

```lumo
match status
  when "active" yield print "OK"
  when "banned" yield print "blocked"
  catch error yield print "unknown"
end
```

## Reserved Words

Never use these as variable, function, or parameter names:

```
intent let be set print skip read repeat while for each in pipe match
when catch error yield start with map filter transform end if then elif
else define taking return call passing list at put get plus minus times
divby modulo above below atleast atmost equals and or not true false value
```

## Strict Mode & Proof Manifests

When `--strict` is used, a `.lumo.proof.json` file must accompany the code. Generate a template:

```bash
./build/lumo --generate-proof-template program.lumo > program.lumo.proof.json
```

Then fill in:
- `safety_properties` — set all to `verified: true` with evidence
- `type_assertions` — type of every expression with reasoning
- `contracts` — function pre/postconditions, loop invariants
- `pattern_compliance` — confirm intent-required and other patterns

The compiler cross-validates everything mechanically and only accepts AI attestations for things it cannot independently verify (loop termination, complex invariants).

## Engineering Conventions

- C++17, CMake build system
- Headers in `include/`, sources in `src/`
- Visitor pattern for all AST operations
- PascalCase for classes, camelCase for methods/variables
- Tests: run all `examples/*.lumo` files
- Three-part sync for language changes: grammar.ebnf + language-reference.md + implementation

## Constraints

- Do NOT use symbol operators in .lumo files — only English words
- Do NOT use `value` as a variable name — it is the pipe register
- Do NOT skip `intent` — every program must start with one
- Do NOT generate `match` blocks without `catch error yield` at the end
- Do NOT use `set` on a variable that was never declared with `let`
