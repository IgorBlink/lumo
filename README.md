# Lumo

**The LLM-Native Programming Language**

Lumo is a Turing-complete, interpreted programming language where **every operator and keyword is a plain English word**. There are no symbols like `+`, `>`, `==`, `{`, or `&&` in program logic. Lumo is built from the ground up to be effortlessly readable, writable, and verifiable by both large language models and humans.

```
intent "Sum integers from 1 to 10"
let i be 1
let total be 0
repeat while i less than 11
- set total be total plus i
- set i be i plus 1
print total
```

---

## Table of Contents

1. [The Problem Lumo Solves](#the-problem-lumo-solves)
2. [Design Philosophy](#design-philosophy)
3. [Getting Started](#getting-started)
4. [Feature Overview](#feature-overview)
   - [Symbol-Free Operators](#1-symbol-free-operators)
   - [Variables — `let` and `set`](#2-variables--let-and-set)
   - [Print](#3-print)
   - [Conditionals — `match`](#4-conditionals--match)
   - [Loops — `repeat while`](#5-loops--repeat-while)
   - [Pipelines — `pipe`](#6-pipelines--pipe)
   - [Intent Tracking](#7-intent-tracking)
   - [First-Class Holes — `???`](#8-first-class-holes--)
   - [JSON-Like Structured Data](#9-json-like-structured-data)
   - [Comments](#10-comments)
   - [AST Debug Mode](#11-ast-debug-mode)
5. [Types and Values](#types-and-values)
6. [Operator Precedence](#operator-precedence)
7. [Error Handling](#error-handling)
8. [Architecture](#architecture)
9. [Examples](#examples)
10. [Language Reference](#language-reference)
11. [LLM Code Generation](#llm-code-generation)
12. [Contributing](#contributing)

---

## The Problem Lumo Solves

Traditional programming languages rely heavily on dense symbol clusters: `&&`, `||`, `!=`, `++`, `->`, `::`, `{}`. While optimized for expert human brevity, this density introduces significant friction for Large Language Models:

- **Tokenization fragility.** Symbol sequences tokenize differently across models and contexts, causing subtle generation errors that pass syntax checks but produce wrong behavior.
- **Hallucinated syntax.** Models confuse `===` vs `==`, `>>` vs `>>=`, `->` vs `=>`, etc. because the visual difference is minimal.
- **Hard-to-audit AI output.** When an LLM generates a dense block of C++ or JavaScript, a human reviewer must mentally parse symbol clusters to verify intent — a slow and error-prone process.

Lumo solves this by enforcing a purely word-based grammar. Every construct reads like a sentence. LLMs excel at natural language, so they generate Lumo code reliably. Humans audit it instantly because the code is self-documenting by nature.

---

## Design Philosophy

**1. Every operator is a word.**
`plus`, `minus`, `times`, `divided by`, `modulo`, `equals`, `greater than`, `less than`, `and`, `or`, `not`. No exceptions.

**2. Structure through indented dashes, not braces.**
Loop bodies, match cases, and pipe steps are delimited by leading `-` characters. Indentation is purely cosmetic — the dash is the structural token.

**3. Mutation is explicit and intentional.**
`let` declares a new variable. `set` mutates an existing one. Calling `set` on an undeclared variable is a runtime error. This forces explicit initialization and eliminates a whole class of "used before assignment" bugs.

**4. AI-first affordances are first-class language features.**
`intent` and `???` (Holes) are not comment conventions — they are language keywords with runtime semantics. Every program formally states its purpose, and every unfinished piece of logic is a typed, detectable marker.

---

## Getting Started

### Prerequisites

A C++17-compatible compiler. No external libraries or package managers required.

- `clang++` (recommended, ships with Xcode Command Line Tools on macOS)
- `g++` (GCC 7+)

### Option A — Direct Compilation

```bash
# Clone the repo
git clone https://github.com/your-username/Umny.git
cd Umny

# Compile all sources in one command
clang++ -std=c++17 -Iinclude src/*.cpp -o lumo
```

### Option B — CMake Build

```bash
mkdir build && cd build
cmake ..
make
```

The CMakeLists.txt at the repo root handles include paths and C++17 standard flag automatically.

### Running Programs

```bash
# Execute a .lumo file
./lumo examples/hello.lumo

# Print the Abstract Syntax Tree instead of executing
./lumo --ast examples/fizzbuzz.lumo

# Suppress [INTENT] trace messages (they go to stderr)
./lumo examples/hello.lumo 2>/dev/null

# Run from stdin (pipe source directly)
echo 'print "hello"' | ./lumo /dev/stdin
```

---

## Feature Overview

### 1. Symbol-Free Operators

Every arithmetic, comparison, and logical operator is a plain English word. This is the core invariant of the language — it is enforced by the lexer, which does not recognize `+`, `>`, `==`, `&&`, etc. as expression tokens.

**Arithmetic operators:**

| Symbol (other langs) | Lumo keyword | Example | Result |
|---|---|---|---|
| `+` | `plus` | `3 plus 4` | `7` |
| `-` | `minus` | `10 minus 3` | `7` |
| `*` | `times` | `5 times 6` | `30` |
| `/` | `divided by` | `10 divided by 4` | `2.5` |
| `%` | `modulo` | `10 modulo 3` | `1` |

**Comparison operators** (always produce a boolean):

| Symbol | Lumo keyword | Example | Result |
|---|---|---|---|
| `==` | `equals` | `5 equals 5` | `true` |
| `>` | `greater than` | `7 greater than 3` | `true` |
| `<` | `less than` | `2 less than 10` | `true` |

**Logical operators:**

| Symbol | Lumo keyword | Notes |
|---|---|---|
| `&&` | `and` | Short-circuits: `false and <expr>` skips `<expr>` |
| `\|\|` | `or` | Short-circuits: `true or <expr>` skips `<expr>` |
| `!` | `not` | Prefix unary: `not true` → `false` |

Expressions compose freely with full precedence rules and parentheses for grouping:

```
let result be (3 plus 4) times 2       # 14
let check be x greater than 0 and y less than 100
```

---

### 2. Variables — `let` and `set`

Lumo enforces a strict two-keyword separation between **declaration** and **mutation**:

**`let <name> be <expression>`** — declares a new variable (or re-declares an existing one) and binds it to the result of the expression.

**`set <name> be <expression>`** — mutates a variable that was already declared. Calling `set` on an undeclared name is a runtime error with a helpful message.

```
let counter be 0           # declare
set counter be counter plus 1  # mutate — OK

set unknown be 5           # RUNTIME ERROR: Cannot set undefined variable: unknown.
                           #   Use 'let' to declare it first.
```

This discipline prevents a whole class of bugs common in dynamically-typed languages where assignment and declaration are indistinguishable.

Variable names follow the rule `[a-zA-Z_][a-zA-Z0-9_]*`. Examples: `x`, `total`, `user_name`, `step2`.

All variables share a **single flat environment** (no block scoping). Variables declared inside loops or match arms are visible everywhere after that point.

---

### 3. Print

```
print <expression>
```

Evaluates any expression and writes its value to **stdout** followed by a newline.

```
print "Hello, World!"          # Hello, World!
print 42                       # 42
print 3 plus 4                 # 7
print x greater than 0         # true  (or false)
print true and false           # false
```

Numbers that are mathematically whole print without a decimal point (`5.0` prints as `5`). Fractional numbers print with full precision (`2.5` prints as `2.5`).

---

### 4. Conditionals — `match`

`match` is Lumo's branching construct. It replaces `if/else if/else` chains, `switch` statements, and ternary expressions with a clean, flat structure.

**Syntax:**
```
match <expression>
- when <value> yield <statement>
- when <value> yield <statement>
- catch error yield <statement>
```

- Each case line starts with `-`
- `when <value>` checks if the match expression equals that value (strict type equality)
- `catch error` is the fallback case, equivalent to `else` or `default`. It is optional.
- `catch error` must always be the **last** case
- `yield <statement>` can be any valid Lumo statement: `print`, `let`, `set`, a nested `match`, etc.

**String match:**
```
let status be "active"
match status
- when "active" yield print "User is active"
- when "banned" yield print "User is banned"
- catch error yield print "Unknown status"
```

**Boolean branch (if/else pattern):**
```
match x greater than 5
- when true yield print "big"
- when false yield print "small"
```

**If-without-else pattern:**
```
match flag
- when true yield print "flag is set"
- catch error yield true        # no-op: true evaluates to itself and is discarded
```

**Nested match (cascade pattern):**

The recommended way to nest conditionals is to put the inner `match` inside the `yield` of a `catch error` case. This avoids parser ambiguity where inner `when` cases would otherwise be consumed by the outer `match`:

```
match mod15
- when 0 yield print "FizzBuzz"
- catch error yield match mod3
  - when 0 yield print "Fizz"
  - catch error yield match mod5
    - when 0 yield print "Buzz"
    - catch error yield print i
```

This is the standard FizzBuzz pattern in Lumo — three levels of nesting achieved without any symbol or brace syntax.

---

### 5. Loops — `repeat while`

```
repeat while <condition>
- <statement>
- <statement>
...
```

Executes the body statements repeatedly as long as the condition is truthy. Body lines are prefixed with `-`.

**Basic counter loop:**
```
let i be 0
repeat while i less than 5
- print i
- set i be i plus 1
```
Output: `0 1 2 3 4`

**Accumulator pattern:**
```
let total be 0
let i be 1
repeat while i less than 101
- set total be total plus i
- set i be i plus 1
print total
```
Output: `5050` (sum of 1 to 100)

**Countdown:**
```
let n be 5
repeat while n greater than 0
- print n
- set n be n minus 1
print "Liftoff!"
```

**`match` inside `repeat`:**

`match` and `repeat` compose freely. Any body line that is a `match` must have its cases indented one more dash level:

```
let i be 1
repeat while i less than 6
- match i
  - when 3 yield print "three"
  - catch error yield print i
- set i be i plus 1
```

---

### 6. Pipelines — `pipe`

A `pipe` is a named sequential data-transformation construct. It maintains an internal **`value` register** that passes through a chain of steps, producing a final result stored in the environment.

```
pipe <name>
- start with <expression>
- transform <expression using value>
- yield result
```

**Steps:**

| Step | Syntax | Effect |
|---|---|---|
| `start with` | `- start with <expr>` | Loads the expression into the `value` register |
| `transform` | `- transform <expr>` | Replaces `value` with the result; use `value` in the expression to refer to the current register |
| `map` | `- map using [var1, var2]` | Documents which variables are in scope (semantic annotation; no-op at runtime) |
| `filter` | `- filter when <condition>` | Documents a filter condition (semantic annotation; no-op at runtime) |
| `yield` | `- yield result` | Stores the current `value` into the global environment under the given identifier |

**Square a number:**
```
let n be 7
pipe square
- start with n
- transform value times value
- yield result
print result
```
Output: `49`

**Chained transforms:**
```
let n be 3
pipe compute
- start with n
- transform value times value    # 3 * 3 = 9
- transform value plus 1         # 9 + 1 = 10
- yield result
print result
```
Output: `10`

The `value` keyword is reserved inside pipe expressions as the name of the register. Multiple pipes can be defined in sequence; each one is independent and `yield` publishes the result to the shared environment.

---

### 7. Intent Tracking

```
intent "<description>"
```

`intent` formally documents the purpose of a program or a section of code. At runtime it emits a trace message to **stderr**:

```
[INTENT] <description>
```

This is not a comment. It is a first-class language construct that:

- Forces the program author (human or LLM) to state the goal explicitly before writing code
- Produces observable runtime evidence that the correct section of code is executing
- Bridges natural-language expectations with machine execution — making it easy to verify that code does what it says
- Keeps the signal stream separate from program output (stderr vs stdout), so it never corrupts output that is piped to other tools

```
intent "Compute compound interest"
let principal be 1000
let rate be 0.05
let years be 10
...
```

At runtime this prints `[INTENT] Compute compound interest` to stderr, then the program proceeds. To suppress intent messages:
```bash
./lumo myprogram.lumo 2>/dev/null
```

---

### 8. First-Class Holes — `???`

```
??? "<instruction>"
```

A **hole** is a first-class language feature for **incremental program construction**. It marks logic that has not yet been implemented. When the interpreter reaches a hole at runtime, it:

1. Prints the instruction to stderr:
   ```
   [HOLE] Unresolved hole at line N: "<instruction>"
   ```
2. Halts immediately with exit code 1

Holes allow you to write the **skeleton** of a program — all the surrounding logic, variable declarations, control flow — and leave placeholders for pieces that need to be filled in later by a developer or an AI agent.

```
intent "Process user order"
let user be "alice"
let items be ??? "Fetch the list of items in alice's cart from the database"
let total be ??? "Calculate the total price including tax and discounts"
print total
```

Running this immediately reports the first unresolved hole without crashing the whole program in an unpredictable way. The AI or developer now has a precise, structured task: fill in the hole at line 3.

This design enables a **scaffolded development workflow**:
1. Write the full program structure with `intent` and holes
2. Run it — each run surfaces the next hole
3. Fill in one hole at a time, guided by the instruction string
4. Repeat until all holes are resolved

---

### 9. JSON-Like Structured Data

Lumo supports JSON-style object literals as expression values:

```
let user_data be { "name": "Alice", "age": 30 }
```

Object literals use the `{ "key": expression }` syntax with comma-separated fields. This allows structured data to be passed into pipeline steps and described with `map using [fieldname]` annotations.

This feature is primarily used to express data shape intent — annotating what fields a pipeline operates on — and is the foundation for future first-class data operations.

---

### 10. Comments

```
# This is a full-line comment
let x be 5  # This is an inline comment
```

Comments start with `#` and extend to the end of the line. They are stripped by the lexer and have no effect on tokenization, parsing, or execution.

---

### 11. AST Debug Mode

Every Lumo program can be inspected at the AST level using the `--ast` flag:

```bash
./lumo --ast examples/fizzbuzz.lumo
```

This runs the lexer and parser normally, then prints the full Abstract Syntax Tree to stdout instead of executing the program. This is useful for:

- Debugging parser behavior
- Understanding how expressions are grouped and precedence applies
- Verifying that a generated program was parsed as intended
- Learning the language's structure

The AST printer is implemented as a concrete `ASTVisitor` that traverses the tree with proper indentation, so the output mirrors the tree structure directly.

---

## Types and Values

Lumo has three runtime types:

| Type | Examples | Notes |
|---|---|---|
| **Number** | `0`, `3.14`, `100`, `-5` | Stored as 64-bit IEEE 754 float. Integers print without decimal point. |
| **String** | `"hello"`, `"world"` | Double-quoted, UTF-8. No escape sequences in the current version. |
| **Boolean** | `true`, `false` | Case-sensitive keywords. |

**Truthiness rules** (used by `repeat while` conditions and `and`/`or`/`not`):

| Value | Truthy? |
|---|---|
| `true` | yes |
| `false` | no |
| any non-zero number | yes |
| `0` | no |
| any non-empty string | yes |
| `""` (empty string) | no |

**Strict type equality:** `0 equals false` → `false`. Two values are only equal if they are the same type and same value.

---

## Operator Precedence

From **lowest** (evaluated last) to **highest** (evaluated first):

| Level | Operators |
|---|---|
| 1 (lowest) | `or` |
| 2 | `and` |
| 3 | `not` |
| 4 | `equals`, `greater than`, `less than` |
| 5 | `plus`, `minus` |
| 6 | `times`, `divided by`, `modulo` |
| 7 (highest) | literals, identifiers, `(...)` |

This mirrors standard mathematical convention: multiplication before addition, comparisons before logic, `not` before `and`/`or`.

Use parentheses to override:
```
let x be 2 plus 3 times 4      # 2 + (3*4) = 14
let y be (2 plus 3) times 4    # (2+3)*4  = 20
```

---

## Error Handling

### Parse Errors

Reported to stderr during the parsing phase, before any code runs.

Format: `Parse error at line N, col M [TOKEN]: message`

| Cause | Message |
|---|---|
| Missing `be` after variable name in `let` | `Expected 'be' after variable name` |
| Missing `while` after `repeat` | `Expected 'while' after 'repeat'` |
| Missing `than` after `greater` or `less` | `Expected 'than' after 'greater'` |
| Missing `by` after `divided` | `Expected 'by' after 'divided'` |
| Empty `match` block | `Expected at least one case in match` |
| Empty `repeat` body | `Expected at least one statement in repeat body` |
| Unknown token | `Expected expression` |

### Runtime Errors

Reported to stderr; program exits with code 1.

| Cause | Message |
|---|---|
| Reading an undeclared variable | `Undefined variable: <name>` |
| `set` on an undeclared variable | `Cannot set undefined variable: <name>. Use 'let' to declare it first.` |
| Arithmetic on a non-number | `Arithmetic operator 'plus' requires a number on the left side, got: <value>` |
| Division by zero | `Division by zero` |
| Modulo by zero | `Modulo by zero` |
| Reaching a hole | `[HOLE] Unresolved hole at line N: "<instruction>"` |

---

## Architecture

The interpreter is implemented in C++17 with a clean, layered pipeline. All source lives in `src/` and headers in `include/`:

```
include/
  lexer.h          — Token types and Lexer interface
  ast.h            — All AST node classes and ASTVisitor interface
  ast_printer.h    — Debug AST printer (ASTVisitor impl)
  parser.h         — Recursive-descent parser interface
  interpreter.h    — Tree-walking interpreter and LumoValue type

src/
  lexer.cpp        — Tokenizer implementation
  ast.cpp          — AST node visitor dispatch
  ast_printer.cpp  — Pretty-printing AST visitor
  parser.cpp       — Recursive-descent parser
  interpreter.cpp  — Tree-walking interpreter
  main.cpp         — Entry point: file I/O, flag parsing, pipeline wiring
```

### Lexer (`lexer.h` / `lexer.cpp`)

The `Lexer` class consumes a raw source string character by character, tracking `line` and `column` for error reporting. It produces a flat `std::vector<Token>` where each `Token` carries a `TokenType` enum, a string `value`, and source location.

Key behaviors:
- **Whitespace and comments** are stripped before each token. Indentation has no semantic meaning; it is entirely ignored.
- **Keywords** are recognized via a static `unordered_map<string, TokenType>` lookup after an alphanumeric word is scanned. This means `plus`, `greater`, `than`, etc. are all distinct token types.
- **Multi-word operators** (`greater than`, `less than`, `divided by`) are handled at the parser level by consuming two tokens in sequence.
- **`???`** is handled character-by-character to avoid C trigraph substitution issues that would occur with a naive three-`?` string.
- **Numbers** are lexed as raw strings and converted to `double` at evaluation time, preserving the lexer/interpreter separation.

### Parser (`parser.h` / `parser.cpp`)

A hand-written **recursive-descent parser** that consumes the token stream and constructs a typed AST. Key design decisions:

- **One function per grammar rule.** `parseStatement()`, `parseExpr()`, `parseOrExpr()`, `parseAndExpr()`, `parseNotExpr()`, `parseCompExpr()`, `parseAddExpr()`, `parseMulExpr()`, `parsePrimary()` form a clean descent that directly encodes the precedence table.
- **NEWLINE as structure.** The parser uses `NEWLINE` tokens as statement terminators, not semicolons. Blank lines (consecutive NEWLINEs) are silently skipped.
- **DASH as block delimiter.** `repeat while` bodies, `pipe` steps, and `match` cases are all terminated by the absence of a leading `DASH` token, enabling unambiguous inline nesting.
- **Error recovery.** Parse errors throw with a message including line/column and the current token, giving useful context for debugging.

### AST (`ast.h` / `ast.cpp`)

A typed hierarchy of `ASTNode` subclasses using **smart pointers** (`std::unique_ptr`) for automatic memory management. All nodes implement the **Visitor pattern** via `accept(ASTVisitor&)`.

Expression nodes: `NumberExpr`, `StringLiteralExpr`, `BoolLiteralExpr`, `IdentifierExpr`, `HoleExpr`, `BinaryExpr`, `UnaryExpr`, `JsonObjectExpr`

Statement nodes: `IntentDecl`, `LetDecl`, `SetDecl`, `PrintDecl`, `ExprStatement`, `RepeatDecl`, `PipeDecl`, `MatchDecl`

Supporting nodes: `StepNode` (pipe step), `MatchCaseNode`, `ContextNode` (pipe map annotation), `ProgramNode` (root)

### Interpreter (`interpreter.h` / `interpreter.cpp`)

A **tree-walking interpreter** that implements `ASTVisitor`. It maintains:

- `env` — a flat `unordered_map<string, LumoValue>` for all variables
- `lastValue` — a `LumoValue` slot used to pass evaluated expression results up the call stack (avoiding a separate return-value mechanism)
- `pipeValue` — a dedicated register for the active pipe's `value` keyword, isolated from the regular environment

`LumoValue` is `std::variant<double, std::string, bool>` — the three runtime types in a tagged union.

Key behaviors:
- **`visit(BinaryExpr)`** dispatches on `op` string, performs type checking, and computes the result. Arithmetic operations require both operands to be numbers; comparison semantics are type-aware; logical operators short-circuit by checking the left operand before visiting the right.
- **`visit(RepeatDecl)`** loops by evaluating the condition, executing the body, and repeating. There is no special loop-interrupt mechanism; loops terminate only when the condition becomes falsy.
- **`visit(MatchDecl)`** iterates through cases, evaluating `when` conditions using `valuesEqual()` and executing the first match's `yield_stmt`. `catch error` runs if no `when` matched.
- **`visit(PipeDecl)`** iterates through steps, updating `pipeValue` on each `TRANSFORM`, and writing the final value into `env` on `YIELD`.
- **`visit(HoleExpr)`** throws a `HaltException`, which propagates to `main()` and exits with code 1.

### AST Printer (`ast_printer.h` / `ast_printer.cpp`)

A concrete `ASTVisitor` that prints the tree structure with indentation. Invoked when `--ast` is passed. Useful for debugging parser output and verifying expression grouping.

---

## Examples

All examples live in the `examples/` directory and can be run with `./lumo examples/<name>.lumo`.

### `hello.lumo` — Hello World

```
intent "Print hello world"
print "Hello, World!"
```

### `arithmetic.lumo` — Basic Arithmetic

```
intent "Basic arithmetic operations"
let a be 5
let b be 10
let sum be a plus b
print sum

let product be a times b
print product

let complex be a plus b times 2
print complex
```

Output: `15`, `50`, `25`

### `loop.lumo` — Counter Loop

```
intent "Count from 0 to 4"
let i be 0
repeat while i less than 5
- print i
- set i be i plus 1
```

Output: `0 1 2 3 4`

### `fizzbuzz.lumo` — FizzBuzz (nested `match` in `repeat`)

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

### Collatz Sequence

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

### Pipe — Chain of Transforms

```
intent "Square a number then add 1"
let n be 3
pipe compute
- start with n
- transform value times value
- transform value plus 1
- yield result
print result
```

Output: `10` (3² + 1)

### Using Holes for Incremental Development

```
intent "Send a welcome email to a new user"
let user be "alice@example.com"
let subject be ??? "Generate a subject line for the welcome email"
let body be ??? "Generate a personalized body for the welcome email"
print subject
print body
```

Running this immediately surfaces the first hole:
```
[HOLE] Unresolved hole at line 3: "Generate a subject line for the welcome email"
```

---

## Language Reference

The full formal language reference — including complete grammar (EBNF), all error messages, comprehensive examples, and detailed LLM code generation guidelines — is in **[LUMO.md](LUMO.md)**.

---

## LLM Code Generation

Lumo is designed to be generated by language models with zero hallucinated syntax. The [LLM Code Generation Guide](LUMO.md#17-llm-code-generation-guide) in LUMO.md covers:

- The 8 golden rules for generating correct Lumo
- Statement templates for every construct
- Common patterns (if/else equivalent, counting loops, accumulators, max/min, absolute value)
- Cascade nesting pattern for multi-level conditionals
- What NOT to do (with examples of common mistakes)
- Step-by-step procedure for translating a natural-language task into a Lumo program

**Key rules for LLMs:**
1. Never write `+`, `-`, `*`, `/`, `>`, `<`, `==`, `&&`, `||`, `!` in logic — use the English keywords
2. One statement per line
3. Always `let` before `set`
4. Prefix every body line with `-`
5. `catch error` is always last in a `match`
6. Nest matches only inside `catch error` yields
7. Use `value` only inside pipe transforms
8. Start every program with `intent "..."`

---

## Contributing

Lumo is an early-stage language and interpreter. Contributions, issues, and ideas are welcome.

**Areas actively seeking contribution:**
- Standard library functions (string manipulation, math utilities, type conversion)
- Richer error messages with source snippets
- More example programs
- Bidirectional transpiler (Lumo → JavaScript/Python/C)
- LSP / editor support with hole-detection

**To contribute:**
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Ensure the interpreter compiles cleanly: `clang++ -std=c++17 -Iinclude src/*.cpp -o lumo`
5. Test against the examples: `./lumo examples/*.lumo`
6. Open a pull request with a clear description

---

## Vision

The goal for Lumo is to serve as the bridge between natural-language reasoning models and deterministic execution environments.

**Near-term roadmap:**
- Rich standard library: string operations, math functions, file I/O — all symbol-free
- Bidirectional transpilation: compile Lumo to WebAssembly or C; transpile existing code back to Lumo for AI review
- Agentic IDE integration: editor support that detects `???` holes, surfaces them to an AI assistant, and automatically resolves them

**Long-term vision:**
An AI agent that can write a Lumo skeleton for any task, run it, observe the first unresolved hole, fill it in, and iterate — producing verified, auditable programs step by step without ever generating a syntax error.

---

## License

MIT License. See [LICENSE](LICENSE) for details.
