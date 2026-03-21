# Lumo Language Reference

Lumo is an **LLM-Native programming language** — every operator and keyword is a plain English word. There are no symbols like `+`, `>`, `==`, or `{` in program logic. This makes Lumo programs easy for both humans and language models to read, write, and reason about without ambiguity.

Lumo is **Turing-complete**: it has variables, arithmetic, branching, and unbounded loops.

---

## Table of Contents

1. [Build & Run](#1-build--run)
2. [File Format](#2-file-format)
3. [Values and Types](#3-values-and-types)
4. [Variables](#4-variables)
5. [Operators and Expressions](#5-operators-and-expressions)
6. [Print](#6-print)
7. [Conditionals — match](#7-conditionals--match)
8. [Loops — repeat while](#8-loops--repeat-while)
9. [Pipelines — pipe](#9-pipelines--pipe)
10. [Intent](#10-intent)
11. [Holes — ???](#11-holes--)
12. [Comments](#12-comments)
13. [Operator Precedence](#13-operator-precedence)
14. [Complete Examples](#14-complete-examples)
15. [Error Reference](#15-error-reference)
16. [Grammar Reference](#16-grammar-reference)
17. [LLM Code Generation Guide](#17-llm-code-generation-guide)

---

## 1. Build & Run

**Requirements:** clang++ or g++ with C++17.

```bash
# Build
clang++ -std=c++17 -Iinclude src/*.cpp -o lumo

# Run a .lumo file
./lumo myprogram.lumo

# Print the AST instead of running (useful for debugging)
./lumo --ast myprogram.lumo
```

Lumo prints `[INTENT]` messages to **stderr** and all `print` output to **stdout**. To suppress intent messages:

```bash
./lumo myprogram.lumo 2>/dev/null
```

---

## 2. File Format

- File extension: `.lumo`
- Encoding: UTF-8
- One statement per line (blank lines are ignored)
- Indentation is cosmetic — it is stripped by the lexer and has no semantic meaning
- Comments start with `#` and run to end of line

```
intent "My first program"   # this is a comment
let x be 42
print x
```

---

## 3. Values and Types

Lumo has three runtime types:

| Type | Examples | Notes |
|---|---|---|
| **Number** | `0`, `3.14`, `100`, `-5` | All numbers are 64-bit floats internally. Whole numbers print without a decimal point. |
| **String** | `"hello"`, `"world"` | Double-quoted. No escape sequences currently. |
| **Boolean** | `true`, `false` | Lowercase only. |

**Truthiness rules** (used by `repeat while` and `and`/`or`/`not`):

| Value | Truthy? |
|---|---|
| `true` | yes |
| `false` | no |
| any non-zero number | yes |
| `0` | no |
| any non-empty string | yes |
| `""` | no |

**Type comparison:** Two values are only equal if they are the same type. `0 equals false` is `false`.

---

## 4. Variables

### `let` — declare a variable

```
let <name> be <expression>
```

Creates a new variable (or re-declares an existing one) and assigns the result of the expression to it.

```
let x be 5
let name be "Alice"
let flag be true
let sum be x plus 10
```

### `set` — mutate an existing variable

```
set <name> be <expression>
```

Updates a variable that was already declared with `let`. If the variable does not exist, the program halts with a runtime error.

```
let counter be 0
set counter be counter plus 1
```

**Rule:** Always use `let` first, then `set` to update.

**Variable names** must start with a letter or `_` and contain only letters, digits, and underscores. Examples: `x`, `counter`, `user_name`, `total2`.

---

## 5. Operators and Expressions

All operators are plain English words. Expressions compose freely and follow standard precedence.

### Arithmetic

| Lumo | Meaning | Example | Result |
|---|---|---|---|
| `plus` | addition | `3 plus 4` | `7` |
| `minus` | subtraction | `10 minus 3` | `7` |
| `times` | multiplication | `5 times 6` | `30` |
| `divided by` | division | `10 divided by 4` | `2.5` |
| `modulo` | remainder | `10 modulo 3` | `1` |

### Comparison

Comparison expressions produce a **boolean** value.

| Lumo | Meaning | Example | Result |
|---|---|---|---|
| `equals` | equality | `5 equals 5` | `true` |
| `greater than` | greater | `7 greater than 3` | `true` |
| `less than` | less | `2 less than 10` | `true` |

String comparison uses lexicographic order.

### Logic

| Lumo | Meaning | Example | Result |
|---|---|---|---|
| `and` | logical AND | `true and false` | `false` |
| `or` | logical OR | `false or true` | `true` |
| `not` | logical NOT | `not true` | `false` |

`and` and `or` **short-circuit**: `false and <expr>` never evaluates `<expr>`, and `true or <expr>` never evaluates `<expr>`.

### Grouping

Use parentheses to override precedence:

```
let result be (3 plus 4) times 2    # 14, not 10
```

### Storing expression results

Any expression can be stored in a variable and used later:

```
let a be 5
let b be 3
let bigger be a greater than b      # stores true
let combined be bigger and true     # stores true
```

---

## 6. Print

```
print <expression>
```

Evaluates the expression and writes its string representation to **stdout**, followed by a newline.

```
print "Hello, World!"               # Hello, World!
print 42                            # 42
print 3.14                          # 3.14
print true                          # true
print 10 plus 5                     # 15
print x                             # prints the value of x
print x greater than 0              # prints true or false
```

Numbers that are whole (e.g. `5.0`) print without the decimal: `5`.

---

## 7. Conditionals — `match`

`match` is Lumo's branching construct. It evaluates an expression and compares it against a list of `when` cases, executing the first one that matches.

```
match <expression>
- when <value> yield <statement>
- when <value> yield <statement>
- catch error yield <statement>
```

- Cases are listed with a leading `-`
- Each case ends with `yield` followed by a **statement** (any valid Lumo statement, including nested `match` or `print`)
- `catch error` is the fallback — it runs if no `when` case matched
- `catch error` is optional; if omitted and nothing matches, the `match` silently does nothing
- `catch error` must always be the **last** case

### Examples

**Simple string match:**

```
let status be "active"
match status
- when "active" yield print "User is active"
- when "banned" yield print "User is banned"
- catch error yield print "Unknown status"
```

**Numeric match:**

```
let code be 200
match code
- when 200 yield print "OK"
- when 404 yield print "Not found"
- when 500 yield print "Server error"
- catch error yield print "Unknown code"
```

**Boolean match:**

```
let ok be true
match ok
- when true yield print "success"
- when false yield print "failure"
```

**Match on a computed expression:**

```
let x be 10
match x greater than 5
- when true yield print "big"
- when false yield print "small"
```

### Nested match

The `yield` of a case can itself be another `match`. The key rule: **each `catch error` terminates its own match**, so nesting works as a cascade:

```
let mod3 be x modulo 3
let mod5 be x modulo 5
let mod15 be x modulo 15
match mod15
- when 0 yield print "FizzBuzz"
- catch error yield match mod3
  - when 0 yield print "Fizz"
  - catch error yield match mod5
    - when 0 yield print "Buzz"
    - catch error yield print x
```

**Important nesting rule:** When a `when` case (not `catch error`) yields a nested `match`, that inner match greedily consumes all following `-` lines that start with `when` or `catch`. To avoid ambiguity, use the cascade pattern above (nest only inside `catch error`) or ensure inner matches are fully enclosed before outer cases continue.

---

## 8. Loops — `repeat while`

```
repeat while <condition>
- <statement>
- <statement>
...
```

Executes the body statements repeatedly as long as the condition is truthy. Body lines are prefixed with `-`.

```
let i be 0
repeat while i less than 5
- print i
- set i be i plus 1
```

Output:
```
0
1
2
3
4
```

### Accumulator pattern

```
let sum be 0
let i be 1
repeat while i less than 6
- set sum be sum plus i
- set i be i plus 1
print sum
```

Output: `15`

### Countdown

```
let n be 5
repeat while n greater than 0
- print n
- set n be n minus 1
```

Output: `5 4 3 2 1`

### Using `let` inside a loop

Variables declared with `let` inside a loop body are reassigned each iteration (there is no block scoping — all variables share a single environment):

```
let i be 1
repeat while i less than 4
- let doubled be i times 2
- print doubled
- set i be i plus 1
```

Output: `2 4 6`

### Combining `repeat` and `match`

You can use `match` inside a `repeat` body. Make sure the `match` cases all start with `-` and the next repeat body statement (like `set i`) is at the same dash level but is **not** `when` or `catch error`, so the parser knows the match has ended:

```
let i be 1
repeat while i less than 6
- match i
  - when 3 yield print "three"
  - catch error yield print i
- set i be i plus 1
```

---

## 9. Pipelines — `pipe`

A `pipe` is a named sequential data transformation. It operates on an internal register called `value`, and stores the final result into a named variable.

```
pipe <name>
- start with <expression>
- transform <expression using value>
- yield result
```

### Steps

| Step | Syntax | What it does |
|---|---|---|
| `start with` | `- start with <expr>` | Loads an expression into the `value` register |
| `transform` | `- transform <expr>` | Replaces `value` with the result of the expression. Use the word `value` in the expression to refer to the current register. |
| `map` | `- map using [var1, var2]` | Annotates context variables (currently a no-op at runtime, used for documentation) |
| `filter` | `- filter when <condition>` | Annotates a filter condition (currently a no-op at runtime) |
| `yield` | `- yield result` | Stores `value` into the environment under the name `result` (or another identifier) |

### Example

```
let data be 100
pipe double_it
- start with data
- transform value times 2
- yield result
print result
```

Output: `200`

### Chained transforms

```
let n be 3
pipe compute
- start with n
- transform value times value
- transform value plus 1
- yield result
print result
```

Output: `10` (3² + 1)

### Important notes

- `value` is a **special keyword** inside pipe transform expressions. It refers to the current pipe register.
- `result` is stored in the **global environment** and can be accessed by `print result` after the pipe.
- Pipe names are just labels; you can have multiple pipes and they each run sequentially.

---

## 10. Intent

```
intent "<description string>"
```

Documents the purpose of the program or a section. At runtime it prints to **stderr** (not stdout):

```
[INTENT] <description string>
```

`intent` is primarily a signal for LLMs and human readers — it describes what the code block is supposed to do. It has no effect on computation.

```
intent "Count even numbers from 1 to 10"
let i be 1
let count be 0
repeat while i less than 11
- let rem be i modulo 2
- match rem
  - when 0 yield set count be count plus 1
  - catch error yield true
- set i be i plus 1
print count
```

---

## 11. Holes — `???`

```
??? "<instruction>"
```

A hole marks a part of the program that has not been implemented yet. When the interpreter reaches a hole at runtime, it **halts immediately** with a message on stderr and exits with code 1:

```
[HOLE] Unresolved hole at line <N>: "<instruction>"
```

Holes are a first-class feature for **incremental development** and LLM-assisted code generation. You can write the skeleton of a program and mark missing logic with holes, then let an LLM or developer fill them in.

```
intent "Send welcome email"
let user be "alice@example.com"
let message be ??? "Generate a personalized welcome message for the user"
print message
```

When run, the program immediately prints:
```
[HOLE] Unresolved hole at line 3: "Generate a personalized welcome message for the user"
```

---

## 12. Comments

```
# This is a comment
let x be 5  # inline comment
```

Comments start with `#` and extend to the end of the line. They are stripped by the lexer and have no effect on execution.

---

## 13. Operator Precedence

From **lowest** (evaluated last) to **highest** (evaluated first):

| Level | Operators |
|---|---|
| 1 (lowest) | `or` |
| 2 | `and` |
| 3 | `not` |
| 4 | `equals`, `greater than`, `less than` |
| 5 | `plus`, `minus` |
| 6 | `times`, `divided by`, `modulo` |
| 7 (highest) | literals, identifiers, `(...)` groups |

**Examples:**

```
let x be 2 plus 3 times 4      # 2 + (3*4) = 14
let y be (2 plus 3) times 4    # (2+3) * 4 = 20

let a be true or false and false   # true or (false and false) = true
let b be not true or false         # (not true) or false = false
```

---

## 14. Complete Examples

### Hello World

```
intent "Print hello world"
print "Hello, World!"
```

### Arithmetic

```
intent "Basic arithmetic"
let a be 5
let b be 10
let sum be a plus b
print sum

let product be a times b
print product

let complex be a plus b times 2
print complex
```

Output:
```
15
50
25
```

### Countdown loop

```
intent "Countdown from 5"
let n be 5
repeat while n greater than 0
- print n
- set n be n minus 1
print "Liftoff!"
```

Output:
```
5
4
3
2
1
Liftoff!
```

### Sum of 1 to 100

```
intent "Sum integers 1 to 100"
let i be 1
let total be 0
repeat while i less than 101
- set total be total plus i
- set i be i plus 1
print total
```

Output: `5050`

### FizzBuzz (1 to 20)

```
intent "Classic FizzBuzz from 1 to 20"
let i be 1
repeat while i less than 21
- let mod3 be i modulo 3
- let mod5 be i modulo 5
- let mod15 be i modulo 15
- match mod15
  - when 0 yield print "FizzBuzz"
  - catch error yield match mod3
    - when 0 yield print "Fizz"
    - catch error yield match mod5
      - when 0 yield print "Buzz"
      - catch error yield print i
- set i be i plus 1
```

### Pipe: square a number

```
intent "Square a number using a pipe"
let n be 7
pipe square
- start with n
- transform value times value
- yield result
print result
```

Output: `49`

### Boolean logic

```
intent "Boolean operations"
let a be true
let b be false
print a and b
print a or b
print not a
print not b
let c be 5 greater than 3
print c
print c and a
```

Output:
```
false
true
false
true
true
true
```

### Collatz sequence

```
intent "Collatz sequence starting at 27"
let n be 27
let steps be 0
repeat while not (n equals 1)
- let rem be n modulo 2
- match rem
  - when 0 yield set n be n divided by 2
  - catch error yield set n be n times 3 plus 1
- set steps be steps plus 1
print steps
```

---

## 15. Error Reference

### Parse errors

Printed to stderr during parsing, format: `Parse error at line N, col M [TOKEN]: message`

| Cause | Message |
|---|---|
| Missing `be` after `let x` | `Expected 'be' after variable name` |
| Missing `while` after `repeat` | `Expected 'while' after 'repeat'` |
| Missing `than` after `greater` | `Expected 'than' after 'greater'` |
| Missing `than` after `less` | `Expected 'than' after 'less'` |
| Missing `by` after `divided` | `Expected 'by' after 'divided'` |
| Empty `match` block | `Expected at least one case in match` |
| Empty `repeat` body | `Expected at least one statement in repeat body` |
| Unknown token | `Expected expression` |

### Runtime errors

Printed to stderr, program exits with code 1.

| Cause | Message |
|---|---|
| Reading an undeclared variable | `Undefined variable: <name>` |
| `set` on undeclared variable | `Cannot set undefined variable: <name>. Use 'let' to declare it first.` |
| Arithmetic on non-number | `Arithmetic operator 'plus' requires a number on the left side, got: <value>` |
| Division by zero | `Division by zero` |
| Modulo by zero | `Modulo by zero` |
| Reaching a hole | `[HOLE] Unresolved hole at line N: "<instruction>"` |

---

## 16. Grammar Reference

```ebnf
Program      ::= Statement*

Statement    ::= IntentDecl
               | LetDecl
               | SetDecl
               | PrintDecl
               | RepeatDecl
               | PipeDecl
               | MatchDecl
               | Expression              (expression used as statement)

IntentDecl   ::= "intent" StringLiteral
LetDecl      ::= "let" Identifier "be" Expression
SetDecl      ::= "set" Identifier "be" Expression
PrintDecl    ::= "print" Expression

RepeatDecl   ::= "repeat" "while" Expression Newline ("-" Statement)+

PipeDecl     ::= "pipe" Identifier Newline Step+
Step         ::= "-" StepOp
StepOp       ::= "start" "with" Expression
               | "transform" Expression
               | "map" ("using" "[" IdentifierList "]")?
               | "filter" "when" Expression
               | "yield" Identifier

MatchDecl    ::= "match" Expression Newline MatchCase+
MatchCase    ::= "-" "when" Expression "yield" Statement
               | "-" "catch" "error" "yield" Statement

Expression   ::= OrExpr
OrExpr       ::= AndExpr ("or" AndExpr)*
AndExpr      ::= NotExpr ("and" NotExpr)*
NotExpr      ::= "not" NotExpr | CompExpr
CompExpr     ::= AddExpr (("equals" | "greater" "than" | "less" "than") AddExpr)?
AddExpr      ::= MulExpr (("plus" | "minus") MulExpr)*
MulExpr      ::= Primary (("times" | "divided" "by" | "modulo") Primary)*
Primary      ::= Number
               | StringLiteral
               | "true"
               | "false"
               | Identifier
               | "value"
               | "result"
               | "???" StringLiteral
               | "{" JsonField ("," JsonField)* "}"
               | "(" Expression ")"

JsonField    ::= StringLiteral ":" Expression
Identifier   ::= [a-zA-Z_][a-zA-Z0-9_]*
Number       ::= [0-9]+ ("." [0-9]+)?
StringLiteral::= '"' [^"]* '"'
```

---

## 17. LLM Code Generation Guide

This section is written specifically for language models generating Lumo code.

### The Golden Rules

1. **No symbols in logic.** Never write `+`, `-`, `*`, `/`, `>`, `<`, `==`, `!=`, `&&`, `||`, `!`. Use the English keywords.
2. **One statement per line.** Never put two statements on the same line.
3. **`let` then `set`.** Always declare a variable with `let` before mutating it with `set`.
4. **Dash prefix for body lines.** Every line inside `repeat while`, `pipe`, or `match` must start with `-`.
5. **`catch error` is always last.** In a `match`, `catch error` must be the final case. You cannot put `when` after `catch error`.
6. **Nest matches via `catch error`.** To nest match expressions, always put the nested `match` in the `yield` of a `catch error` case, not a `when` case, to avoid ambiguous parsing.
7. **Use `value` inside pipe transforms.** The word `value` is the pipe register keyword. Do not use it as a regular variable name.
8. **`intent` first.** Start every program with `intent "..."` describing what it does.

### Statement Templates

**Declare a variable:**
```
let <name> be <expression>
```

**Update a variable:**
```
set <name> be <expression>
```

**Print output:**
```
print <expression>
```

**Conditional branch:**
```
match <expression>
- when <value> yield <statement>
- when <value> yield <statement>
- catch error yield <statement>
```

**Loop:**
```
repeat while <condition>
- <statement>
- <statement>
```

**Pipeline:**
```
pipe <name>
- start with <expression>
- transform value <operator> <expression>
- yield result
```

**Mark incomplete logic:**
```
??? "<what needs to go here>"
```

### Common Patterns

**If/else equivalent:**
```
match <condition>
- when true yield <then-statement>
- when false yield <else-statement>
```

**If without else:**
```
match <condition>
- when true yield <then-statement>
- catch error yield true
```

**Counting loop (0 to N-1):**
```
let i be 0
repeat while i less than <N>
- <body using i>
- set i be i plus 1
```

**Accumulator:**
```
let total be 0
let i be 1
repeat while i less than <N plus 1>
- set total be total plus i
- set i be i plus 1
print total
```

**Max of two numbers:**
```
let a be 8
let b be 5
match a greater than b
- when true yield print a
- when false yield print b
```

**Absolute value:**
```
let n be -7
match n less than 0
- when true yield set n be 0 minus n
- catch error yield true
print n
```

**Multi-condition dispatch (cascade pattern):**
```
match <primary-check>
- when <value1> yield <statement1>
- when <value2> yield <statement2>
- catch error yield match <secondary-check>
  - when <value3> yield <statement3>
  - catch error yield <default-statement>
```

### What NOT to do

```
# WRONG — symbol operators
let x be a + b
let y be a > b

# WRONG — two statements on one line
let a be 5  let b be 10

# WRONG — set before let
set x be 5            # crashes: x was never declared

# WRONG — when after catch error
match x
- catch error yield print "default"
- when 5 yield print "five"    # parser error

# WRONG — nesting when case yields match (ambiguous)
match x
- when 0 yield match y          # y's cases will steal outer cases
  - when 1 yield print "one"
- when 2 yield print "two"      # this might be consumed by inner match

# CORRECT — nest only inside catch error
match x
- when 0 yield print "zero"
- catch error yield match y
  - when 1 yield print "one"
  - catch error yield print "other"
```

### Generating programs step by step

When writing a Lumo program to solve a task:

1. Write `intent "..."` describing the task.
2. Declare all inputs with `let`.
3. Write the main logic using `repeat`, `match`, or `pipe`.
4. Use `print` to output results.
5. If any logic is unclear, insert `??? "explanation"` as a placeholder.

**Example prompt → code:**

Prompt: *"Sum all even numbers from 1 to 20"*

```
intent "Sum all even numbers from 1 to 20"
let i be 1
let total be 0
repeat while i less than 21
- let rem be i modulo 2
- match rem
  - when 0 yield set total be total plus i
  - catch error yield true
- set i be i plus 1
print total
```

Output: `110`
