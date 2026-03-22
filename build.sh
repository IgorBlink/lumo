#!/usr/bin/env bash
set -euo pipefail

# ─────────────────────────────────────────────
# Lumo — build script
# Usage:
#   ./build.sh            build interpreter (default)
#   ./build.sh install    build + install to /usr/local/bin so `lumo` works globally
#   ./build.sh ext        build VS Code extension (.vsix)
#   ./build.sh all        build everything + install
#   ./build.sh clean      remove all build artifacts
# ─────────────────────────────────────────────

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT/build"
EXT_DIR="$ROOT/vscode-extension"

GREEN="\033[0;32m"
CYAN="\033[0;36m"
RESET="\033[0m"

step() { echo -e "\n${CYAN}▶ $1${RESET}"; }
ok()   { echo -e "${GREEN}✓ $1${RESET}"; }

build_interpreter() {
  if command -v cmake &>/dev/null; then
    step "Building Lumo interpreter (CMake)"
    cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release --log-level=WARNING
    cmake --build "$BUILD_DIR" --parallel
    cp "$BUILD_DIR/lumo" "$ROOT/lumo"
  elif command -v clang++ &>/dev/null; then
    step "Building Lumo interpreter (clang++ direct)"
    clang++ -std=c++17 -O2 -Iinclude src/*.cpp -o "$ROOT/lumo"
  elif command -v g++ &>/dev/null; then
    step "Building Lumo interpreter (g++ direct)"
    g++ -std=c++17 -O2 -Iinclude src/*.cpp -o "$ROOT/lumo"
  else
    echo "Error: no C++ compiler found. Install clang++, g++, or cmake."
    exit 1
  fi
  ok "Binary ready → ./lumo"
}

build_extension() {
  step "Installing VS Code extension dependencies"
  npm install --prefix "$EXT_DIR" --silent

  step "Compiling TypeScript"
  npm run compile --prefix "$EXT_DIR"

  step "Packaging VSIX"
  (cd "$EXT_DIR" && npx @vscode/vsce package \
    --no-dependencies \
    --allow-missing-repository \
    --out lumo-language.vsix)

  ok "Extension ready → vscode-extension/lumo-language.vsix"
  echo ""
  echo "  Install: Cmd+Shift+P → Install from VSIX → vscode-extension/lumo-language.vsix"
}

install_binary() {
  build_interpreter
  step "Installing lumo to /usr/local/bin"
  sudo cp "$ROOT/lumo" /usr/local/bin/lumo
  ok "Installed — run \`lumo\` from anywhere"
}

clean() {
  step "Cleaning build artifacts"
  rm -rf "$BUILD_DIR"
  rm -f  "$ROOT/lumo"
  rm -rf "$EXT_DIR/out"
  rm -f  "$EXT_DIR"/*.vsix
  ok "Clean"
}

case "${1:-interpreter}" in
  interpreter) build_interpreter ;;
  install)     install_binary ;;
  ext)         build_extension ;;
  all)         install_binary; build_extension ;;
  clean)       clean ;;
  *)
    echo "Usage: ./build.sh [interpreter|install|ext|all|clean]"
    exit 1
    ;;
esac
