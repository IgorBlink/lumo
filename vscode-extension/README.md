# Lumo Language — VS Code Extension

Adds **syntax highlighting** and **real-time parse error linting** for `.lumo` files to Visual Studio Code.

---

## Features

- Full syntax highlighting for all Lumo keywords, operators, strings, numbers, comments, and constants.
- Red error squiggles appear as you type (or on save), powered by the `lumo --validate` command.
- Auto-closing pairs for `()`, `{}`, `[]`, and `""`.
- Comment toggling with `#`.

---

## Requirements

You need the `lumo` binary on your system. Build it from the repository root:

```bash
cmake -B build && cmake --build build
cp build/lumo ./lumo   # or wherever you keep it
```

---

## Installation (Development / Local)

1. **Install dependencies and compile:**

```bash
cd vscode-extension
npm install
npm run compile
```

2. **Open the extension in VS Code's Extension Development Host:**

   - Open the `vscode-extension` folder in VS Code.
   - Press `F5` (or **Run → Start Debugging**) to launch the Extension Development Host.
   - Open any `.lumo` file — syntax highlighting and linting are active immediately.

3. **Or install locally using `vsce`:**

```bash
npm install -g @vscode/vsce
vsce package        # creates lumo-language-0.1.0.vsix
code --install-extension lumo-language-0.1.0.vsix
```

---

## Extension Settings

| Setting | Default | Description |
|---|---|---|
| `lumo.executablePath` | `""` | Path to the `lumo` binary. Leave blank to auto-detect a `lumo` binary at the workspace root, then fall back to `PATH`. |
| `lumo.validateOnType` | `true` | Validate while typing (debounced 500 ms). Set to `false` to validate on save only. |

### Pointing to a custom binary

Open VS Code **Settings** (`Cmd+,`) and search for `lumo.executablePath`, or add this to your `settings.json`:

```json
{
  "lumo.executablePath": "/path/to/lumo"
}
```

If the workspace root contains a file named `lumo`, the extension will use it automatically without any configuration.

---

## Syntax Token Colors

The extension uses standard TextMate scopes so any VS Code color theme applies its own colors automatically. Here is how Lumo constructs map to scopes:

| Lumo constructs | TextMate scope |
|---|---|
| `if then elif else end repeat for match when catch error yield return` | `keyword.control.lumo` |
| `plus minus times divby modulo above below atleast atmost equals and or not` | `keyword.operator.lumo` |
| `let set be define taking` | `keyword.other.declaration.lumo` |
| `print read intent skip` | `keyword.other.io.lumo` |
| `list at put get` | `keyword.other.list.lumo` |
| `pipe start with map filter transform` | `keyword.other.pipe.lumo` |
| `call passing` | `keyword.other.call.lumo` |
| `true false` | `constant.language.boolean.lumo` |
| `value` | `variable.language.value.lumo` |
| `42`, `3.14` | `constant.numeric.lumo` |
| `"hello"` | `string.quoted.double.lumo` |
| `# comment` | `comment.line.number-sign.lumo` |
| `??? "description"` | `invalid.illegal.hole.lumo` |
| Function names (after `call`/`define`) | `entity.name.function.lumo` |
| Other identifiers | `variable.other.lumo` |

---

## Linting

Linting is powered by `lumo --validate`, which reads the current file content from stdin and outputs one line per parse error in the form:

```
Parse error at line 2, col 16 [NEWLINE]: Expected expression
```

The extension maps each error to an inline red squiggle at the exact line and column. Only the first (fatal) error is shown, matching the interpreter's own behavior.
