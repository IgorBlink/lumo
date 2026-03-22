---
name: lumo-language
description: Write, read, and verify Lumo programs — the LLM-native language where every operator is a plain English word. Triggers on any .lumo file or when user asks about Lumo syntax, semantics, or code generation.
user-invocable: true
allowed-tools: Read, Write, Edit, Bash, Grep, Glob
argument-hint: "[task description]"
---

# Lumo Language — Complete AI Reference

You are writing code in **Lumo**, a Turing-complete interpreted language where every operator and keyword is a plain English word. There are NO symbols in program logic — no `+`, `>`, `==`, `{`, `&&`. Only English words.

## Golden Rules

1. **Every operator is a word.** Write `plus` not `+`. Write `equals` not `==`. Write `above` not `>`.
2. **Every block ends with `end`.** No braces. No indentation-sensitivity.
3. **One canonical form per construct.** There are no synonyms. `plus` is the only addition operator. `equals` is the only equality operator.
4. **Variables: `let` to declare, `set` to mutate.** You cannot `set` a variable that was not `let`-declared first.
5. **`value` is reserved.** It is the pipe element register. Never use it as a variable name.
6. **Programs start with `intent`.** Every program begins with `intent "description of purpose"`.

## Types

| Type | Literal syntax | Truthiness |
|------|---------------|------------|
| Number | `0`, `3.14`, `42` | `0` is falsy, all else truthy |
| String | `"hello"`, `""` | `""` is falsy, all else truthy |
| Boolean | `true`, `false` | direct |
| List | `list 1, 2, 3` | empty list is falsy |
| Object | `{ "key": value }` | empty object is falsy |

Type equality is **strict**: `0 equals false` evaluates to `false`.

## Complete Statement Reference

```
intent "description"                          # document purpose
let <name> be <expr>                          # declare variable
set <name> be <expr>                          # mutate variable (must be declared)
print <expr>                                  # output to stdout
skip                                          # explicit no-op
read <name>                                   # read line from stdin (var must exist)
return <expr>                                 # return from function
call <name> [passing <a>, <b>, ...]           # call function (statement form)
put <name> at <index> be <expr>               # mutate list element or object field
```

## Complete Block Reference

```
repeat while <condition>                      # while loop
  <statements>
end

repeat <count> times                          # count-controlled loop
  <statements>
end

for each <var> in <list-expr>                 # iterate list
  <statements>
end

if <condition> then                           # conditional
  <statements>
elif <condition> then                         # optional, any number
  <statements>
else                                          # optional
  <statements>
end

define <name> [taking <p1>, <p2>, ...]        # function definition
  <statements>
  return <expr>
end

pipe <label>                                  # data pipeline
  start with <expr>
  filter when <condition>                     # keep elements where condition is true
  map <expr>                                  # transform each element
  transform <expr>                            # transform scalar value
  yield <variable-name>                       # store result in this variable
end

match <expr>                                  # pattern matching (MUST end with catch error)
  when <value> yield <statement>
  catch error yield <statement>
end
```

## Operator Precedence (low to high)

| Precedence | Operators | Notes |
|-----------|-----------|-------|
| 1 (lowest) | `or` | short-circuits |
| 2 | `and` | short-circuits |
| 3 | `not` | unary prefix |
| 4 | `equals` `above` `below` `atleast` `atmost` | comparison |
| 5 | `plus` `minus` | `plus` also concatenates strings |
| 6 | `times` `divby` `modulo` | |
| 7 (highest) | literals, identifiers, `(...)`, `call`, `get`, `list` | |

**Operator meanings:**
- `plus` = addition or string concatenation
- `minus` = subtraction
- `times` = multiplication
- `divby` = division
- `modulo` = remainder
- `equals` = strict equality (type-sensitive)
- `above` = strictly greater than (`>`)
- `below` = strictly less than (`<`)
- `atleast` = greater than or equal (`>=`)
- `atmost` = less than or equal (`<=`)
- `and` = logical AND
- `or` = logical OR
- `not` = logical NOT

## Reserved Words — NEVER use as variable/function/parameter names

```
intent let be set print skip read repeat while for each in pipe match
when catch error yield start with map filter transform end if then elif
else define taking return call passing list at put get plus minus times
divby modulo above below atleast atmost equals and or not true false value
```

## Function Scoping

Functions have **isolated local scope**:
- Parameters are copied in
- Functions can READ outer variables but CANNOT MUTATE them
- Only `return` communicates results back
- Local `set` affects only the local scope

```lumo
let x be 10
define double taking n
  return n times 2
end
let result be call double passing x
print result    # 20
print x         # 10 (unchanged)
```

## Pipe Semantics

Inside `map`, `filter`, and `transform` steps, the keyword `value` refers to the current element. The `yield` step stores the pipeline result in the named variable.

```lumo
let nums be list 1, 2, 3, 4, 5, 6
pipe process
  start with nums
  filter when value modulo 2 equals 0
  map value times 10
  yield evens
end
print evens    # [20, 40, 60]
```

## Common Patterns

### FizzBuzz
```lumo
intent "FizzBuzz from 1 to 20"
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

### Recursive factorial
```lumo
intent "Compute factorial"
define factorial taking n
  if n below 2 then
    return 1
  end
  return n times call factorial passing n minus 1
end
print call factorial passing 10
```

### List processing with pipe
```lumo
intent "Sum of squares of odd numbers"
define square taking n
  return n times n
end

let nums be list 1, 2, 3, 4, 5
pipe squared_odds
  start with nums
  filter when value modulo 2 equals 1
  map call square passing value
  yield odds
end
print odds
```

## Critical Anti-Patterns — NEVER DO THESE

| Wrong | Correct | Why |
|-------|---------|-----|
| `x + y` | `x plus y` | No symbol operators |
| `x > y` | `x above y` | No symbol operators |
| `x == y` | `x equals y` | No symbol operators |
| `x && y` | `x and y` | No symbol operators |
| `if (x > 5) {` | `if x above 5 then` | No parens, no braces, use `then` |
| `}` | `end` | Blocks end with `end` |
| `x = 5` | `let x be 5` | Use `let`/`be` for declaration |
| `x = x + 1` | `set x be x plus 1` | Use `set`/`be` for mutation |
| `def f(x):` | `define f taking x` | Use `define`/`taking` |
| `f(x)` | `call f passing x` | Use `call`/`passing` |
| `arr[0]` | `get arr at 0` | Use `get`/`at` |
| `for i in range(10):` | `repeat 10 times` | Use `repeat`/`times` |
| `let value be 5` | `let item be 5` | `value` is reserved |

## Build and Run

```bash
cmake -S . -B build && cmake --build build
./build/lumo program.lumo              # run
./build/lumo --validate program.lumo   # syntax check
./build/lumo --ast program.lumo        # print AST
./build/lumo --strict program.lumo     # require proof manifest
```
