# Lumo v2 — Engineering Design Prompt

**Purpose:** This document is a complete, self-contained engineering prompt. Hand it to any capable LLM or developer to implement Lumo v2. It contains the exact syntax decisions, EBNF grammar, implementation tasks, and validation criteria required to bring all four underperforming dimensions to 10/10 while preserving and strengthening the core mission: **the most readable, most reliable LLM-native programming language.**

---

## 0. Context: Where Lumo v1 Stands

Three frontier LLMs (ChatGPT, Gemini, Claude) benchmarked Lumo v1 independently. Their consensus scores:

| Dimension | v1 Score | Target |
|---|---|---|
| Concept & Originality | 9/10 | 10/10 |
| Documentation | 9/10 | 10/10 |
| **Implementation Quality** | **7/10** | **10/10** |
| **Design Consistency** | **6/10** | **10/10** |
| **Language Completeness** | **4/10** | **10/10** |
| **Practical Usability** | **5/10** | **10/10** |

### Benchmark Consensus: Top Recurring Criticisms

From all three models, the following issues appeared in every review:

1. **Cosmetic indentation:** Visual structure diverges from parse structure. LLMs scope-hallucinate.
2. **Greedy nested-match:** `catch error` cascade workaround is non-obvious, fragile, documented as dangerous.
3. **Context-sensitive keywords:** `value` and `result` mean different things inside vs. outside `pipe`.
4. **Multi-word operators:** `divided by`, `greater than`, `less than` fragment across tokens.
5. **No user-defined functions:** Forces verbose repeated patterns; LLMs hallucinate abstractions.
6. **No data structures:** No lists, maps. LLMs hallucinate nonexistent collection syntax.
7. **`pipe map` and `pipe filter` are no-ops:** Documented but non-functional — silent surprise.
8. **Silent no-op match:** `match` without `catch error` silently does nothing — unpredictable.

**Features to preserve (unanimous praise):**

- `intent` keyword — keep as-is
- `???` holes — keep as-is, they are the most innovative feature
- `let` / `set` distinction — keep as-is
- One statement per line — keep as-is
- Plain English operators — keep and extend

---

## 1. The Eight Design Laws of Lumo v2

Every syntax decision in this document is derived from these laws. They are normative — any proposal that violates them is rejected even if it seems more convenient.

```
LAW 1 — ONE TOKEN, ONE MEANING
  Every keyword and operator is a single globally reserved word.
  No multi-word operators. No context-sensitive keywords.
  No word that means different things in different positions.

LAW 2 — VISUAL EQUALS SEMANTIC
  The structure a human or LLM sees on the page must exactly match
  the structure the parser builds. All blocks end with an explicit
  terminator keyword. Indentation is COSMETIC — it helps readability
  but carries zero parsing weight (consistent with v1).

LAW 3 — ONE CANONICAL FORM
  Every construct has exactly one valid way to write it.
  No synonyms. No optional syntax. No flexibility.
  Two programs that mean the same thing must look identical.

LAW 4 — TOTAL CONTROL FLOW
  Every branching construct must handle all possible outcomes.
  Silent no-ops in control flow are a parse error.
  Every `match` must end with `else`. Every `if` must have an `else`.

LAW 5 — ZERO CONTEXT-SENSITIVE KEYWORDS
  A word reserved in any context is reserved in all contexts.
  There is no "special inside a pipe" or "special inside a match."

LAW 6 — STRICT STATEMENT/EXPRESSION BOUNDARY
  Expressions produce values. Statements produce effects.
  A bare expression is not a valid statement.
  Only: let, set, print, if, match, repeat, define, return, 
        pipe, intent, ??? are valid statement forms.

LAW 7 — NATIVE ABSTRACTIONS OVER WORKAROUNDS
  Common programming needs (functions, lists, maps) must be
  first-class language features. LLMs must never need to invent
  patterns the language does not provide.

LAW 8 — MACHINE-VERIFIABLE GRAMMAR DOMINATES
  Prose descriptions are for humans. The EBNF grammar in section 5
  is normative. Any conflict between prose and grammar: grammar wins.
  All examples in all documentation must parse against this grammar.
```

---

## 2. Design Consistency Fixes (Fixes Laws 1–5)

### 2.1 Block Terminators — Replacing Dash-Prefix Nesting

**Problem:** In v1, block bodies use `-` prefixes with cosmetic indentation. LLMs misscope nested blocks because visual depth has no semantic weight.

**Solution:** All multi-statement blocks use an explicit `end` terminator. Single-statement inline forms are preserved for `if` and `match`.

**Rule:** Any block that spans more than one statement MUST use `end`. Inline forms (no `end`) are only valid for single-statement bodies.

```
# MULTI-STATEMENT BLOCK (uses end)
repeat while i below 10
    set i be i plus 1
    print i
end

# INLINE FORM (single statement, no end)
repeat while i below 10
    set i be i plus 1
```

**Important:** The `-` prefix is **removed entirely** from v2. Body statements are indented for readability but indentation carries no semantic weight. The `end` keyword terminates the block unambiguously.

---

### 2.2 Operator Consolidation — Single-Token Operators

**Problem:** `divided by`, `greater than`, `less than` each span two tokens. LLMs generate partial completions that break at token boundaries.

**Solution:** Replace all multi-word operators with single-word, globally reserved alternatives. The English readability principle is preserved; brevity is not the goal — unambiguity is.

**Complete operator table for v2:**

| v1 Form | v2 Form | Reserved? | Notes |
|---|---|---|---|
| `plus` | `plus` | Yes | unchanged |
| `minus` | `minus` | Yes | unchanged |
| `times` | `times` | Yes | unchanged |
| `divided by` | `dividedby` | Yes | single token |
| `modulo` | `modulo` | Yes | unchanged |
| `equals` | `equals` | Yes | unchanged |
| `greater than` | `above` | Yes | natural English |
| `less than` | `below` | Yes | natural English |
| *(new)* | `atleast` | Yes | >= (greater than or equal) |
| *(new)* | `atmost` | Yes | <= (less than or equal) |
| `and` | `and` | Yes | unchanged |
| `or` | `or` | Yes | unchanged |
| `not` | `not` | Yes | unchanged |

**Rationale for `above`/`below`:** These are unambiguous, single English words that carry the correct directional meaning without the word `than`, which itself adds a parsing obligation.

---

### 2.3 Conditionals — `if`/`else` and Cleaned `match`

**Problem:** Using `match` + `catch error` for simple boolean branching violates LLM priors. `catch error` as a label implies error handling, not a default branch.

**Solution:** Introduce `if`/`else`/`end` as a first-class construct. Rename `catch error` to `else` inside `match`. Rename `yield` to `then` inside `match` cases. Require the `else` clause — no silent no-ops (Law 4).

**`if` syntax:**

```
# Inline (single statement per branch)
if <condition> then <statement>
else <statement>

# Block form (multiple statements per branch)
if <condition> then
    <statement>
    <statement>
else
    <statement>
end

# Nested
if x above 0 then
    if x above 100 then
        print "very large"
    else
        print "medium"
    end
else
    print "not positive"
end
```

**`match` syntax (cleaned):**

```
# Inline case
match <expression>
    when <value> then <statement>
    when <value> then <statement>
    else <statement>
end

# The else clause is REQUIRED. Omitting it is a parse error.
```

**Combined example — FizzBuzz:**

```
intent "Classic FizzBuzz from 1 to 20"
let i be 1
repeat while i atmost 20
    let mod3 be i modulo 3
    let mod5 be i modulo 5
    if mod3 equals 0 and mod5 equals 0 then
        print "FizzBuzz"
    else if mod3 equals 0 then
        print "Fizz"
    else if mod5 equals 0 then
        print "Buzz"
    else
        print i
    end
    set i be i plus 1
end
```

---

### 2.4 Context-Free Keywords — Replacing `value` and `result`

**Problem:** Inside `pipe`, `value` refers to the pipe register and `result` is the default output name. These words are unusable as variable names but look like ordinary identifiers, causing LLM keyword collision errors.

**Solution:**
- Replace `value` (pipe register) with `it` — a globally reserved pronoun.
- Replace `result` (default yield target) — `yield` now requires an explicit variable name. No default.
- Both `it` and `result` are globally reserved and may never be used as identifiers.

**Pipe syntax in v2:**

```
pipe <name>
    start with <expression>
    transform it <operator> <expression>
    yield <identifier>
end
```

**Example:**

```
let n be 7
pipe square
    start with n
    transform it times it
    yield squared
end
print squared
```

**`it` is the active value register inside any pipe transform.** It is reserved globally — an LLM can never accidentally name a variable `it`.

---

### 2.5 Mandatory `else` Enforcement

In v1, `match` without `catch error` silently does nothing. In v2:

- Every `match` must have an `else` branch. **Missing `else` is a parse error.**
- Every `if` must have an `else` branch. **Missing `else` is a parse error.**
- If the programmer explicitly wants a no-op, they must write it: `else nothing`

**`nothing` keyword:** A new statement that explicitly does nothing. It satisfies the total-control-flow law while making intent transparent.

```
if x above 0 then
    print x
else
    nothing
end
```

---

## 3. Language Completeness Additions (Law 7)

### 3.1 User-Defined Functions

**Syntax:**

```
define <name> taking <param1>, <param2>, ...
    <statements>
    return <expression>
end
```

**Rules:**
- `define` is a statement at the top level only. Functions cannot be defined inside loops or other functions.
- `return` is required and must be the last statement. A function without `return` is a parse error.
- Parameters are declared with comma-separated names after `taking`.
- Functions with no parameters use `taking nothing`.
- Call syntax: `<name> with <arg1>, <arg2>, ...`
- Zero-argument call: `<name> with nothing`

**Examples:**

```
intent "Define and call a function"

define square taking n
    return n times n
end

define add taking a, b
    return a plus b
end

define greet taking name
    return "Hello, " join name
end

define pi taking nothing
    return 3.14159
end

let x be square with 5
print x

let sum be add with 3, 7
print sum

let message be greet with "Alice"
print message

let circle_area be pi with nothing times 4 times 4
print circle_area
```

**Recursion is supported:**

```
define factorial taking n
    if n atmost 1 then
        return 1
    else
        return n times (factorial with n minus 1)
    end
end

print factorial with 6
```

---

### 3.2 Lists

**Syntax:**

```
# Create
let <name> be list <expr>, <expr>, ...
let <name> be empty list

# Read
length of <list>
first of <list>
last of <list>
item <index> of <list>     # 1-based index

# Mutate
add <expr> to <list>
remove last from <list>
set item <index> of <list> be <expr>
```

**Examples:**

```
intent "List operations"
let nums be list 1, 2, 3, 4, 5
print length of nums
print first of nums
print last of nums
print item 3 of nums

add 6 to nums
print length of nums

remove last from nums
print length of nums

set item 1 of nums be 99
print first of nums
```

**Iterating over a list (repeat with index):**

```
let fruits be list "apple", "banana", "cherry"
let i be 1
repeat while i atmost length of fruits
    print item i of fruits
    set i be i plus 1
end
```

---

### 3.3 Maps

**Syntax:**

```
# Create
let <name> be map
    <key> is <expression>
    <key> is <expression>
end

let <name> be empty map

# Read
get <key> from <map>
has <key> in <map>          # returns boolean

# Mutate
set <key> of <map> be <expression>
remove <key> from <map>
```

**Examples:**

```
intent "Map operations"
let person be map
    name is "Alice"
    age is 30
    active is true
end

print get name from person
print get age from person
print has email in person

set age of person be 31
print get age from person

remove active from person
print has active in person
```

---

### 3.4 String Operations

**Syntax:**

```
<string> join <string>          # concatenation
length of <string>              # character count
uppercase of <string>           # uppercase transformation
lowercase of <string>           # lowercase transformation
contains <string> in <string>   # boolean membership
```

**Examples:**

```
let greeting be "Hello" join ", " join "World"
print greeting

let len be length of greeting
print len

let up be uppercase of greeting
print up

let has_hello be contains "Hello" in greeting
print has_hello
```

---

### 3.5 Working Pipe with `map` and `filter`

In v1, `map` and `filter` in pipes are no-ops. In v2, they operate on list values.

**Full pipe syntax (operates on a list stored in `it`):**

```
pipe <name>
    start with <list-expression>
    filter when <condition using it>
    map it to <expression using it>
    yield <identifier>
end
```

**`it` inside a `map` or `filter` step refers to the current element being processed.**

**Example:**

```
intent "Filter and transform a list"
let nums be list 1, 2, 3, 4, 5, 6

pipe evens_squared
    start with nums
    filter when it modulo 2 equals 0
    map it to it times it
    yield result_list
end

let i be 1
repeat while i atmost length of result_list
    print item i of result_list
    set i be i plus 1
end
```

Output: `4 16 36`

---

### 3.6 `nothing` Statement

A first-class no-op that satisfies the total-control-flow law without silence.

```
nothing
```

Use it anywhere a statement is required but no effect is intended.

---

## 4. Implementation Quality Requirements (Law 8)

### 4.1 Block-Scoped Variables

In v1, all variables share a single global environment. In v2:

- Variables declared with `let` inside `if`, `repeat`, or `define` blocks are **local to that block**.
- `set` can mutate a variable from an outer scope.
- Reading a variable that does not exist in the current scope or any enclosing scope throws a runtime error.

**Example:**

```
let total be 0
let i be 1
repeat while i below 6
    let step be i times 2      # `step` is local to this repeat block
    set total be total plus step
    set i be i plus 1
end
print total                    # OK: total is outer scope
# print step                   # ERROR: step is out of scope
```

---

### 4.2 Canonical Formatter

Implement a formatter (`./lumo --format <file>`) that:
- Normalises indentation to 4 spaces inside every block
- Enforces exactly one blank line between top-level `define` declarations
- Strips trailing whitespace
- Outputs the canonical form to stdout

A program is valid Lumo v2 if and only if running it through the formatter produces the same output. This is the normative conformance test.

---

### 4.3 Machine-Readable Grammar Schema

Publish `lumo_grammar.json` at the repo root. It must contain:

```json
{
  "version": "2.0",
  "reserved_words": [...],
  "operators": {
    "arithmetic": ["plus", "minus", "times", "dividedby", "modulo"],
    "comparison": ["equals", "above", "below", "atleast", "atmost"],
    "logical": ["and", "or", "not"],
    "string": ["join"]
  },
  "statement_forms": ["intent", "let", "set", "print", "if", "match",
                      "repeat", "define", "return", "pipe", "nothing"],
  "block_terminators": ["end"],
  "special_literals": ["true", "false", "it"],
  "hole_syntax": "??? \"<instruction>\""
}
```

This schema can be loaded by any LLM as a tool definition or system context, giving it a machine-readable contract for exact generation.

---

### 4.4 Test Suite (minimum 100 tests)

Organise tests in a `tests/` directory. Each test is a `.lumo` file paired with a `.expected` file containing expected stdout.

**Required test coverage categories:**

| Category | Min Tests |
|---|---|
| Arithmetic and precedence | 10 |
| Comparison operators (all 5) | 10 |
| Logic operators (and, or, not) | 8 |
| `if`/`else` (inline and block) | 10 |
| `match` (with `else`) | 10 |
| `repeat while` | 8 |
| `define` / `return` / recursion | 12 |
| Lists (all operations) | 10 |
| Maps (all operations) | 8 |
| Strings (all operations) | 6 |
| `pipe` (map, filter, transform) | 8 |
| `intent` and `???` holes | 4 |
| Error messages (runtime) | 6 |
| Block scoping | 6 |
| **Total** | **116** |

Run with: `./lumo --test tests/`

---

### 4.5 Error Messages

All error messages must include:
- File name
- Line number
- Column number
- The exact token that triggered the error
- A plain-English explanation
- A `hint:` line suggesting the fix

**Example parse error:**
```
Parse error at examples/hello.lumo:5:12 [IDENTIFIER(value)]
  Cannot use reserved word 'value' as a variable name.
  hint: rename this variable — 'value' is reserved for the pipe register.
```

**Example runtime error:**
```
Runtime error at examples/hello.lumo:12 [set]
  Cannot mutate 'counter' — it is out of scope here.
  hint: 'counter' was declared inside a repeat block and is not visible here.
```

---

## 5. Complete v2 Grammar (EBNF — Normative)

This grammar is the single source of truth. All documentation, all examples, and the formatter must agree with it.

```ebnf
Program      ::= TopLevelStmt*

TopLevelStmt ::= DefineDecl | Statement

DefineDecl   ::= "define" Identifier "taking" ParamList Newline
                   Statement+
                   ReturnStmt
                 "end"

ParamList    ::= "nothing" | Identifier ("," Identifier)*
ReturnStmt   ::= "return" Expression Newline

Statement    ::= IntentDecl
               | LetDecl
               | SetDecl
               | PrintDecl
               | IfDecl
               | RepeatDecl
               | PipeDecl
               | MatchDecl
               | NothingStmt
               | HoleStmt

IntentDecl   ::= "intent" StringLiteral Newline
LetDecl      ::= "let" Identifier "be" Expression Newline
SetDecl      ::= "set" LValue "be" Expression Newline
PrintDecl    ::= "print" Expression Newline
NothingStmt  ::= "nothing" Newline
HoleStmt     ::= "???" StringLiteral Newline

LValue       ::= Identifier
               | "item" Expression "of" Identifier
               | Identifier "of" Identifier

IfDecl       ::= IfInline | IfBlock
IfInline     ::= "if" Expression "then" Statement
                 "else" Statement
IfBlock      ::= "if" Expression "then" Newline
                   Statement+
                 ElseBlock
                 "end" Newline
ElseBlock    ::= "else" "if" Expression "then" Newline Statement+ ElseBlock
               | "else" Newline Statement+

RepeatDecl   ::= "repeat" "while" Expression Newline
                   Statement+
                 "end" Newline

MatchDecl    ::= "match" Expression Newline
                   MatchCase+
                   ElseCase
                 "end" Newline
MatchCase    ::= "when" Expression "then" Statement Newline
ElseCase     ::= "else" Statement Newline

PipeDecl     ::= "pipe" Identifier Newline
                   PipeStep+
                 "end" Newline
PipeStep     ::= StartStep | TransformStep | MapStep | FilterStep | YieldStep
StartStep    ::= "start" "with" Expression Newline
TransformStep::= "transform" Expression Newline
MapStep      ::= "map" "it" "to" Expression Newline
FilterStep   ::= "filter" "when" Expression Newline
YieldStep    ::= "yield" Identifier Newline

Expression   ::= OrExpr
OrExpr       ::= AndExpr ("or" AndExpr)*
AndExpr      ::= NotExpr ("and" NotExpr)*
NotExpr      ::= "not" NotExpr | CompExpr
CompExpr     ::= AddExpr (CompOp AddExpr)?
CompOp       ::= "equals" | "above" | "below" | "atleast" | "atmost"
AddExpr      ::= MulExpr (("plus" | "minus") MulExpr)*
MulExpr      ::= ConcatExpr (("times" | "dividedby" | "modulo") ConcatExpr)*
ConcatExpr   ::= CallExpr ("join" CallExpr)*
CallExpr     ::= Identifier "with" ArgList
               | AccessExpr
AccessExpr   ::= "length" "of" Primary
               | "first" "of" Primary
               | "last" "of" Primary
               | "item" Primary "of" Primary
               | "get" Identifier "from" Primary
               | "has" Identifier "in" Primary
               | "uppercase" "of" Primary
               | "lowercase" "of" Primary
               | "contains" Primary "in" Primary
               | Primary

ArgList      ::= "nothing" | Expression ("," Expression)*

Primary      ::= Number
               | StringLiteral
               | "true"
               | "false"
               | "it"
               | Identifier
               | ListLiteral
               | MapLiteral
               | "???" StringLiteral
               | "(" Expression ")"

ListLiteral  ::= "list" Expression ("," Expression)*
               | "empty" "list"
MapLiteral   ::= "map" Newline (Identifier "is" Expression Newline)+ "end"
               | "empty" "map"

Identifier   ::= [a-zA-Z_][a-zA-Z0-9_]*   (not a reserved word)
Number       ::= [0-9]+ ("." [0-9]+)?
StringLiteral::= '"' [^"\n]* '"'
Newline      ::= '\n'+
```

**Complete reserved word list (forbidden as identifiers):**

```
intent  let  set  be  print  if  then  else  end  match  when
repeat  while  define  taking  return  nothing  pipe  start  with
transform  map  filter  yield  to  it  and  or  not  true  false
plus  minus  times  dividedby  modulo  equals  above  below
atleast  atmost  join  list  empty  length  first  last  item
add  remove  get  has  from  uppercase  lowercase  contains  of  in
```

---

## 6. Annotated Example Programs

These programs demonstrate v2 idioms. Each one should parse and execute correctly against the grammar.

### 6.1 Hello World

```
intent "Print hello to the world"
print "Hello, World!"
```

### 6.2 Sum of 1 to 100

```
intent "Sum all integers from 1 to 100"
let i be 1
let total be 0
repeat while i atmost 100
    set total be total plus i
    set i be i plus 1
end
print total
```

### 6.3 FizzBuzz (1 to 100)

```
intent "Classic FizzBuzz from 1 to 100"
let i be 1
repeat while i atmost 100
    if i modulo 15 equals 0 then
        print "FizzBuzz"
    else if i modulo 3 equals 0 then
        print "Fizz"
    else if i modulo 5 equals 0 then
        print "Buzz"
    else
        print i
    end
    set i be i plus 1
end
```

### 6.4 Recursion — Fibonacci

```
intent "Print first 10 Fibonacci numbers"

define fib taking n
    if n atmost 1 then
        return n
    else
        return (fib with n minus 1) plus (fib with n minus 2)
    end
end

let i be 0
repeat while i below 10
    print fib with i
    set i be i plus 1
end
```

### 6.5 List — Sum of Even Numbers

```
intent "Sum even numbers in a list"

let nums be list 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
let total be 0
let i be 1
repeat while i atmost length of nums
    let val be item i of nums
    if val modulo 2 equals 0 then
        set total be total plus val
    else
        nothing
    end
    set i be i plus 1
end
print total
```

### 6.6 Map — Lookup Table

```
intent "HTTP status code lookup"

let codes be map
    ok is 200
    not_found is 404
    server_error is 500
end

let code be get ok from codes
match code
    when 200 then print "OK"
    when 404 then print "Not Found"
    else print "Unknown"
end
```

### 6.7 Pipe — Transform a List

```
intent "Square all numbers above 3"
let nums be list 1, 2, 3, 4, 5, 6

pipe large_squares
    start with nums
    filter when it above 3
    map it to it times it
    yield squares
end

let i be 1
repeat while i atmost length of squares
    print item i of squares
    set i be i plus 1
end
```

### 6.8 String Operations

```
intent "Demonstrate string operations"
let first_name be "Ada"
let last_name be "Lovelace"
let full_name be first_name join " " join last_name
print full_name
print length of full_name
print uppercase of full_name
print contains "Ada" in full_name
```

### 6.9 Holes — Incremental Development

```
intent "Send a welcome email to a new user"
let user_name be "Alice"
let user_email be "alice@example.com"
let subject be "Welcome to Lumo"
let body be ??? "Write a friendly welcome email body for a new user named user_name"
print body
```

### 6.10 Collatz Sequence

```
intent "Compute Collatz sequence steps starting at 27"
let n be 27
let steps be 0
repeat while not (n equals 1)
    if n modulo 2 equals 0 then
        set n be n dividedby 2
    else
        set n be n times 3 plus 1
    end
    set steps be steps plus 1
end
print steps
```

---

## 7. Implementation Checklist

Implement these in priority order. Each item has a clear done criterion.

### Phase 1 — Design Consistency (highest ROI)

- [ ] **1.1** Remove `-` dash-prefix syntax from lexer and parser entirely
- [ ] **1.2** Add `end` terminator token to lexer; update `repeat`, `match`, `pipe`, `if`, `define` parsers to require it
- [ ] **1.3** Rename `catch error` to `else` in `match`; rename `yield` to `then` in `match` cases
- [ ] **1.4** Add `if`/`else`/`end` parser (inline and block forms)
- [ ] **1.5** Add `above`, `below`, `atleast`, `atmost` tokens; replace `greater than`, `less than` in lexer/parser/interpreter
- [ ] **1.6** Add `dividedby` token; remove `divided` + `by` two-token form
- [ ] **1.7** Globally reserve `it`; remove `value` token; update pipe interpreter to use `it`
- [ ] **1.8** Globally reserve `result`; require explicit identifier in `yield`; update pipe interpreter
- [ ] **1.9** Add `nothing` statement to lexer, parser, interpreter (no-op)
- [ ] **1.10** Enforce mandatory `else` on `match` and `if` — missing `else` is a parse error

### Phase 2 — Language Completeness

- [ ] **2.1** Add `define`/`taking`/`return`/`end` to lexer and parser; implement function environment in interpreter with argument binding
- [ ] **2.2** Add `call with` expression syntax to parser; implement function call in interpreter
- [ ] **2.3** Add list runtime type to interpreter; implement `list ...`, `empty list` literals
- [ ] **2.4** Implement `length of`, `first of`, `last of`, `item N of`, `add to`, `remove last from`, `set item N of` for lists
- [ ] **2.5** Add map runtime type to interpreter; implement `map ... end`, `empty map` literals
- [ ] **2.6** Implement `get from`, `has in`, `set of be`, `remove from` for maps
- [ ] **2.7** Implement `join` for string concatenation; implement `uppercase of`, `lowercase of`, `contains in`, `length of` for strings
- [ ] **2.8** Make `pipe filter when` and `map it to` fully operational for lists; update pipe interpreter
- [ ] **2.9** Add `atleast`/`atmost` comparison operators to interpreter

### Phase 3 — Implementation Quality

- [ ] **3.1** Implement block-scoped environment (scope chain: each block creates a child scope; `set` walks up the chain)
- [ ] **3.2** Update all error messages to include file name, line, column, token, explanation, and `hint:` line
- [ ] **3.3** Implement `./lumo --format <file>` formatter that outputs canonical indentation
- [ ] **3.4** Write `lumo_grammar.json` schema file
- [ ] **3.5** Write 116+ tests in `tests/` with `.lumo` + `.expected` file pairs
- [ ] **3.6** Implement `./lumo --test tests/` test runner that compares stdout to `.expected` files
- [ ] **3.7** Update `LUMO.md` to reflect v2 syntax throughout (no v1 syntax may remain)
- [ ] **3.8** Add `--version` flag to `./lumo` that prints `Lumo v2.0`

---

## 8. What Must NOT Change

These v1 features are unanimously praised and must be carried forward unchanged:

| Feature | Rule |
|---|---|
| `intent "..."` statement | Keep syntax and stderr-output behavior exactly |
| `??? "..."` holes | Keep syntax and halt-with-message behavior exactly |
| `let` / `set` distinction | Keep semantics; only scope rules change |
| `print` statement | Unchanged |
| One statement per line | Unchanged |
| `#` comments | Unchanged |
| `.lumo` file extension | Unchanged |
| `--ast` debug flag | Keep and extend to show v2 AST nodes |
| C++17 build requirement | No new dependencies; same single-command build |
| Turing completeness | Must remain provably Turing-complete |
| Three runtime types: Number, String, Boolean | Extend with List and Map — do not remove or change existing three |

---

## 9. Validation: How to Verify 10/10

After implementing v2, run this checklist to confirm each dimension has reached 10/10:

### Design Consistency ✓
- [ ] No `-` dash characters appear in any v2 example program
- [ ] Every block in every example ends with `end`
- [ ] `above`, `below`, `atleast`, `atmost`, `dividedby` are the only comparison/division operators
- [ ] `value` and `result` cannot be used as variable names (parser rejects them)
- [ ] `it` is globally reserved and cannot be used as a variable name
- [ ] Every `match` has an `else` clause (parser rejects missing `else`)
- [ ] Every `if` has an `else` clause (parser rejects missing `else`)
- [ ] No construct has more than one valid syntactic form

### Language Completeness ✓
- [ ] The recursive Fibonacci example (section 6.4) executes correctly
- [ ] The list-sum example (section 6.5) executes correctly
- [ ] The map-lookup example (section 6.6) executes correctly
- [ ] The pipe-transform example (section 6.7) executes correctly
- [ ] `pipe filter` and `pipe map it to` produce correct output on lists
- [ ] String operations (join, length, uppercase, lowercase, contains) all work

### Implementation Quality ✓
- [ ] Block scoping: `print step` after the repeat loop in section 4.1 throws a runtime error
- [ ] All 116 tests in `tests/` pass with `./lumo --test tests/`
- [ ] `./lumo --format examples/fizzbuzz.lumo` produces canonical output
- [ ] `lumo_grammar.json` exists and is valid JSON
- [ ] Error messages include file, line, column, and `hint:`

### Practical Usability ✓
- [ ] The FizzBuzz program (section 6.3) requires no workarounds or cascade patterns
- [ ] A real-world sorting program (e.g., bubble sort of a list) can be written in under 30 lines
- [ ] Any LLM given the `lumo_grammar.json` schema and the eight design laws can generate valid v2 programs without additional context

---

*End of Lumo v2 Engineering Design Prompt.*
*This document is the single normative specification for Lumo v2.*
*Grammar in section 5 dominates all other prose in case of conflict.*
