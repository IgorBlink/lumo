# Lumo

**The LLM-Native Programming Language**

Lumo is a Turing-complete, interpreted programming language where every operator and keyword is a plain English word. There are no symbols like `+`, `>`, `==`, `{`, or `&&` in program logic. Lumo is designed to be written, read, and verified by both large language models and humans without ambiguity.

---

## Why Lumo

Traditional languages use dense symbol clusters (`&&`, `||`, `!=`, `->`, `::`) optimized for expert human brevity. This creates friction for LLMs: symbols tokenize inconsistently, visually similar operators get confused, and hallucinated syntax passes a surface read but breaks at runtime.

Lumo eliminates the symbol layer entirely. Every construct has one canonical form, one meaning, and one token — making generation reliable and verification trivial.

---

## Quick Example

```
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

---

## Build

Requires a C++17 compiler and CMake 3.10+.

```bash
cmake -S . -B build
cmake --build build
./build/lumo examples/hello.lumo
```

Or build directly with clang++:

```bash
clang++ -std=c++17 -Iinclude src/*.cpp -o lumo
./lumo examples/hello.lumo
```

---

## Usage

```bash
./lumo <file.lumo>           # Run a program
./lumo --validate <file.lumo> # Validate syntax only
./lumo --ast <file.lumo>     # Print the AST and exit
```

---

## Language Highlights

| Feature | Syntax |
|---|---|
| Variables | `let x be 10` / `set x be x plus 1` |
| Arithmetic | `plus` `minus` `times` `divided` `modulo` |
| Comparison | `equals` `above` `below` `not` |
| Logic | `and` `or` |
| Conditionals | `if … then` / `elif` / `else` / `end` |
| Pattern matching | `match` / `when` / `catch error yield` |
| Loops | `repeat while` / `repeat N times` / `for each` |
| Functions | `define f taking x` … `end` / `call f passing x` |
| Lists | `list 1, 2, 3` / `get nums at 0` / `put nums at 0 be 99` |
| Pipelines | `pipe` … `start with` / `filter` / `map` / `yield` |
| Intent annotation | `intent "description"` |
| Holes | `???` — marks an intentionally incomplete expression |
| No-op | `skip` |

---

## Examples

The [`examples/`](examples/) directory contains runnable programs:

- [`hello.lumo`](examples/hello.lumo) — Hello World
- [`arithmetic.lumo`](examples/arithmetic.lumo) — Basic arithmetic
- [`fizzbuzz.lumo`](examples/fizzbuzz.lumo) — FizzBuzz
- [`loop.lumo`](examples/loop.lumo) — Loop constructs
- [`functions.lumo`](examples/functions.lumo) — Functions and lists

---

## Documentation

- [Language Reference](docs/language-reference.md) — Complete syntax, types, operators, and semantics
- [Grammar](docs/grammar.ebnf) — Formal EBNF grammar

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build instructions, code style, and the pull request process.

---

## License

[MIT](LICENSE) © Igor Martynyuk
