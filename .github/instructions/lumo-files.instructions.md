---
applyTo: "**/*.lumo"
---

This is a Lumo source file. Lumo uses English words for all operators:

- `plus` (not +), `minus` (not -), `times` (not *), `divby` (not /), `modulo` (not %)
- `equals` (not ==), `above` (not >), `below` (not <), `atleast` (not >=), `atmost` (not <=)
- `and` (not &&), `or` (not ||), `not` (not !)
- `let x be 5` (not x = 5), `set x be 6` (not x = 6)
- `define f taking x ... end` (not def f(x)), `call f passing x` (not f(x))
- `get arr at 0` (not arr[0]), `put arr at 0 be 99` (not arr[0] = 99)
- All blocks end with `end`. Programs start with `intent "description"`.
- `match` blocks must end with `catch error yield <statement>`.
- `value` is reserved (pipe register). Never use as variable name.

See docs/language-reference.md and docs/grammar.ebnf for the full specification.
