# Lumo Language Reference — v2

Lumo is an LLM-native, Turing-complete programming language. Every operator and keyword is a single, plain English word — no symbols (`+`, `>`, `==`, `{`) appear in program logic.

This document is the normative human-readable reference. The machine-readable authority is `docs/grammar.ebnf`.

---

## Core Language Rules

1. **One Token, One Meaning.** Every operator and keyword is a single globally-reserved plain English word. No multi-word operators. No context-sensitive keywords.
2. **Visual Equals Semantic.** Blocks are terminated by the `end` keyword. Indentation is cosmetic only.
3. **One Canonical Form.** Every construct has exactly one valid syntactic form. Synonymy is forbidden.
4. **Total Control Flow.** All `match` blocks must end with `catch error yield`. No silent no-ops.
5. **Strict Segregation.** Variables must be declared with `let` before mutation with `set`. Statements and expressions have zero syntactic overlap. Bare expressions are not valid statements — use `skip` for a deliberate no-op.
6. **Native Abstraction.** Use `define` for reusable logic and `list` for sequences. Do not approximate them.
7. **Validator Domination.** Run `./lumo --validate <file>` to verify syntax before execution.

---

## Types

| Type    | Examples              | Truthiness                    |
|---------|-----------------------|-------------------------------|
| Number  | `0`, `3.14`, `42`     | non-zero is truthy, `0` is falsy |
| String  | `"hello"`, `""`       | non-empty is truthy, `""` is falsy |
| Boolean | `true`, `false`       | direct                        |
| List    | `list 1, 2, 3`        | non-empty is truthy, `[]` is falsy |
| Object  | `{ "key": value }`    | non-empty is truthy, `{}` is falsy |

**Type equality is strict**: `0 equals false` → `false`. Cross-type comparison always returns `false`.

---

## Reserved Words

The following words are globally reserved and may never be used as variable names, function names, or parameter names:

```
intent   let      be       set      print    skip     read
repeat   while    for      each     in
pipe     match    when     catch    error    yield    start
with     map      filter   transform end
if       then     elif     else
define   taking   return   call     passing
list     at       put      get
plus     minus    times    divby    modulo
above    below    atleast  atmost   equals   and      or       not
true     false    value
```

`value` is globally reserved as the pipe element register. It cannot be used as a variable, function, or parameter name anywhere in a Lumo program. Choose alternative names such as `item`, `element`, or `current` when a similar concept is needed outside a pipeline.

---

## Statements

### `intent`

Documents the purpose of a block of code. Printed to stderr at runtime.

```
intent "string"
```

```lumo
intent "Compute the factorial of 10"
```

### `let` — declare a variable

```
let <name> be <expression>
```

```lumo
let x be 42
let greeting be "hello"
let flags be list true, false, true
```

### `set` — mutate an existing variable

`set` requires the variable to have been declared with `let` first.

```
set <name> be <expression>
```

```lumo
set x be x plus 1
```

### `print`

```
print <expression>
```

```lumo
print "Hello, World!"
print x plus y
```

### `skip` — explicit no-op

Use `skip` wherever a statement is syntactically required but no action is desired.

```lumo
match x
  when 1 yield print "one"
  catch error yield skip
end
```

### `read` — read from standard input

Reads one line from standard input and stores it in `<name>`. The variable must be declared with `let` before calling `read`. Input is always stored as a String.

```
read <name>
```

```lumo
let name be ""
read name
print "Hello, " plus name
```

If stdin reaches end-of-file, `read` stores the empty string `""` and does not crash.

### `return` — return from a function

Only valid inside a `define` body.

```
return <expression>
```

### `call` — call a function (statement form)

When the return value is not needed, use `call` as a statement.

```
call <name> [passing <arg1>, <arg2>, ...]
```

```lumo
call greet passing "World"
```

### `put` — mutate a list element or JSON object field

```
put <name> at <index-or-key> be <value>
```

For lists, `<index-or-key>` must be a zero-based Number. For JSON objects, it must be a String.

```lumo
put nums at 0 be 99
put user at "age" be 31
```

---

## Block Constructs

All blocks are terminated with the `end` keyword. Indentation is cosmetic.

### `repeat while` — condition-controlled loop

```
repeat while <condition>
  <statements>
end
```

```lumo
let i be 1
repeat while i below 11
  print i
  set i be i plus 1
end
```

### `repeat times` — count-controlled loop

```
repeat <count> times
  <statements>
end
```

`<count>` must evaluate to a non-negative integer at runtime, otherwise a runtime error is thrown. No loop index variable is exposed. Use when iteration count matters but the index does not. Wrap complex count expressions in parentheses.

```lumo
repeat 3 times
  print "hello"
end
```

### `for each` — iterate over a list

```
for each <name> in <list-expression>
  <statements>
end
```

`<list-expression>` must evaluate to a List at runtime. `<name>` is bound to the current element on each iteration. The loop variable is not accessible outside the loop. `<name>` must not shadow an existing variable in the outer scope — the parser emits an error if it does.

```lumo
let words be list "a", "b", "c"
for each w in words
  print w
end
```

### `if` / `elif` / `else` / `end` — conditional

```
if <condition> then
  <statements>
elif <condition> then
  <statements>
else
  <statements>
end
```

`elif` and `else` branches are optional. There is no limit on the number of `elif` branches.

```lumo
let score be 85
if score above 90 then
  print "A"
elif score above 80 then
  print "B"
elif score above 70 then
  print "C"
else
  print "F"
end
```

### `define` — user-defined function

```
define <name> [taking <param1>, <param2>, ...]
  <statements>
  return <expression>
end
```

Lumo functions use isolated local scope. When called, a new environment is created containing only the declared parameters. Functions can read top-level (program-scope) variables but cannot mutate them. Local `set` statements affect only the local scope. The only way a function communicates a result back to the caller is via `return`.

```lumo
define add taking x, y
  return x plus y
end

let total be call add passing 3, 4
print total
```

Example demonstrating that mutations inside a function do not affect outer scope:

```lumo
let counter be 0

define increment
  set counter be counter plus 1
  return counter
end

let result be call increment
print result    # prints: 1
print counter   # prints: 0 — outer counter is unchanged
```

Zero-parameter function:

```lumo
define greet
  print "Hello!"
end

call greet
```

### `match` / `catch error` / `end` — pattern match

`match` compares a value against a list of `when` cases using strict equality. The last case **must** be `catch error` — this is enforced at parse time.

```
match <expression>
  when <value> yield <statement>
  when <value> yield <statement>
  catch error yield <statement>
end
```

```lumo
let status be "active"
match status
  when "active" yield print "User is active"
  when "banned" yield print "User is banned"
  catch error yield print "Unknown status"
end
```

### `pipe` — data transformation pipeline

`pipe` chains transformation steps. The pipeline result is stored by `yield <identifier>`. The pipe block name is for documentation only and has no semantic effect.

```
pipe <name>
  start with <expression>
  [transform <expression>]
  [map <expression>]
  [filter when <condition>]
  yield <identifier>
end
```

| Step | Description |
|------|-------------|
| `start with <expr>` | Initialize the pipeline with a value or list |
| `transform <expr>`  | Apply expression to the current value (scalar); `value` is the current value |
| `map <expr>`        | Apply expression to each element of a list; `value` is the current element |
| `filter when <expr>` | Keep only list elements for which `value` satisfies `expr` |
| `yield <identifier>` | Store pipeline output into the named variable |

The identifier given after `yield` is the variable name where pipeline output is stored. The pipe block name is for documentation only.

Inside `map`, `filter`, and `transform` steps, `value` refers to the current element being processed. It is read-only within these steps and invalid outside a `pipe` block — using `value` outside a pipe is a runtime error.

```lumo
let nums be list 1, 2, 3, 4, 5

pipe processed
  start with nums
  filter when value modulo 2 equals 0
  map value times 10
  yield evens
end

print evens
```

---

## Expressions

### Operators (all single tokens)

| Precedence | Operator(s)                                | Notes |
|------------|--------------------------------------------|-------|
| 1 (lowest) | `or`                                       | short-circuits |
| 2          | `and`                                      | short-circuits |
| 3          | `not` (unary prefix)                       | |
| 4          | `equals`, `above`, `below`, `atleast`, `atmost` | strict type equality for `equals`; numeric-only for comparisons |
| 5          | `plus`, `minus`                            | `plus` also concatenates strings |
| 6          | `times`, `divby`, `modulo`                 | |
| 7 (highest) | literals, identifiers, `(...)`, `call`, `get`, `list` | |

**`above`** — true when left operand is strictly greater than right  
**`below`** — true when left operand is strictly less than right  
**`atleast`** — true when left operand is greater than or equal to right (`>=`)  
**`atmost`** — true when left operand is less than or equal to right (`<=`)  
**`divby`** — division; throws on zero divisor  
**`modulo`** — remainder; throws on zero divisor

`atleast` and `atmost` are numeric-only. Cross-type comparison returns `false`.

### String concatenation

`plus` concatenates strings when either operand is a string:

```lumo
print "Hello, " plus "World"
print "Count: " plus 42
```

### Grouped expressions

```lumo
let result be (a plus b) times c
```

### Boolean literals

```lumo
let flag be true
let done be false
```

### `value` — pipe element register

`value` is globally reserved as the pipe element register. Inside `map`, `filter`, and `transform` steps, `value` refers to the current element or scalar being processed. Using `value` outside a pipe block is a runtime error.

---

## Lists

Lists are first-class values. Elements can be any Lumo type.

```lumo
let nums be list 1, 2, 3
let words be list "hello", "world"
let mixed be list 1, "two", true
```

### `get` — read a list element

```
get <list-expr> at <index>
```

Indices are zero-based. Out-of-bounds access throws a runtime error.

```lumo
let first be get nums at 0
print first
```

### `put` — write a list element

```
put <name> at <index> be <value>
```

```lumo
put nums at 1 be 99
print get nums at 1
```

---

## JSON Objects

JSON-style objects are first-class values with string keys.

```lumo
let user be { "name": "Alice", "age": 30 }
print user
```

### `get` — read a JSON field

```
get <object-expr> at <string-key>
```

Throws a runtime error if the key does not exist.

```lumo
print get user at "name"    # prints: Alice
print get user at "age"     # prints: 30
```

### `put` — write a JSON field

```
put <name> at <string-key> be <value>
```

```lumo
put user at "age" be 31
print get user at "age"     # prints: 31
```

---

## Functions

### `define` / `taking` / `return`

See [Block Constructs — `define`](#define--user-defined-function) for the full scoping specification.

```lumo
define factorial taking n
  if n below 2 then
    return 1
  end
  return n times call factorial passing n minus 1
end

print call factorial passing 5
```

### `call` — expression form

When the function return value is needed in an expression:

```
call <name> [passing <arg1>, <arg2>, ...]
```

```lumo
let result be call add passing 10, 20
```

### `call` — statement form

When the return value is not needed:

```lumo
call greet passing "Igor"
```

---

## Holes

`???` marks an unresolved expression that halts execution when reached.

```lumo
let answer be ??? "compute the correct value here"
```

---

## Comments

Comments begin with `#` and extend to the end of the line.

```lumo
let x be 5  # this is a comment
```

---

## Full Example — FizzBuzz

```lumo
intent "Classic FizzBuzz from 1 to 20"
let i be 1
repeat while i below 21
  let mod3 be i modulo 3
  let mod5 be i modulo 5
  if mod3 equals 0 and mod5 equals 0 then
    print "FizzBuzz"
  elif mod3 equals 0 then
    print "Fizz"
  elif mod5 equals 0 then
    print "Buzz"
  else
    print i
  end
  set i be i plus 1
end
```

## Full Example — Functions and Pipelines

```lumo
intent "Find squares of even numbers"

define square taking n
  return n times n
end

let nums be list 1, 2, 3, 4, 5, 6

pipe square_evens
  start with nums
  filter when value modulo 2 equals 0
  map call square passing value
  yield output
end

print output
```

---

## Command-Line Usage

| Flag | Effect |
|------|--------|
| `./lumo <file>` | Execute a Lumo program |
| `./lumo --ast <file>` | Print the AST without executing |
| `./lumo --validate <file>` | Parse and validate syntax; print `OK` or errors |
| `./lumo` | Read and execute a Lumo program from stdin |

---

## Grammar

The normative machine-readable grammar is in `docs/grammar.ebnf`. All valid Lumo programs must parse against that grammar without alteration.
