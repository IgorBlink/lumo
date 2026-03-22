# Lumo Language — GitHub Copilot Instructions

Lumo is a programming language where every operator is a plain English word. When generating or editing `.lumo` files:

## Operator Translation

Write `plus` not `+`. Write `equals` not `==`. Write `above` not `>`. Write `and` not `&&`. Write `let x be 5` not `x = 5`. Write `set x be x plus 1` not `x = x + 1`. Write `call f passing x` not `f(x)`. Write `get arr at 0` not `arr[0]`. All blocks end with `end`, never with `}`.

## Required Structure

1. Every program starts with `intent "description"`
2. Declare variables with `let name be expression`
3. Mutate with `set name be expression` (must `let` first)
4. Functions: `define name taking params ... return expr ... end`
5. Calls: `call name passing args` or `let x be call name passing args`
6. Loops: `repeat while cond ... end` / `repeat N times ... end` / `for each x in list ... end`
7. Conditionals: `if cond then ... elif cond then ... else ... end`
8. Match: `match expr ... when val yield stmt ... catch error yield stmt ... end`
9. Pipes: `pipe label ... start with expr ... filter when cond ... map expr ... yield varname ... end`

## Never Do

- Use symbol operators (`+`, `-`, `*`, `/`, `%`, `==`, `>`, `<`, `&&`, `||`, `!`)
- Use braces `{ }` for blocks (use `end`)
- Use `value` as a variable name (reserved pipe register)
- Omit `intent` as first statement
- Omit `catch error yield` from `match` blocks
- Use `set` before `let`

## Types

Number (`42`), String (`"hello"`), Boolean (`true`/`false`), List (`list 1, 2, 3`), Object (`{ "key": val }`)

## Reserved Words

```
intent let be set print skip read repeat while for each in pipe match when catch error
yield start with map filter transform end if then elif else define taking return call
passing list at put get plus minus times divby modulo above below atleast atmost equals
and or not true false value
```
