#!/usr/bin/env bash
# NppLX installer: checks and installs dependencies, builds and installs NppLX.
#
# Usage:
#   ./install.sh                          install to /usr/local (default)
#   PREFIX="$HOME/.local" ./install.sh    install into the user directory (no root)
#   ./install.sh --uninstall              remove the installed files
#
# The script operates in the source directory it lives in
# (main.cpp, CMakeLists.txt, assets/). The build happens in build/.
set -euo pipefail

PREFIX="${PREFIX:-/usr/local}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_FILE="$PREFIX/bin/npplx"
DESKTOP_FILE="$PREFIX/share/applications/npplx.desktop"
ICON_FILE="$PREFIX/share/icons/hicolor/scalable/apps/npplx.svg"

note() { printf '==> %s\n' "$*"; }
die()  { printf 'Error: %s\n' "$*" >&2; exit 1; }

have_root()  { [ "$(id -u)" -eq 0 ]; }

run_root() {
  if have_root; then "$@";
  elif command -v sudo > /dev/null 2>&1; then sudo "$@";
  else die "Root privileges required, but sudo is not available."; fi
}

pm_detect() {
  if command -v apt-get > /dev/null 2>&1; then echo apt;
  elif command -v pacman  > /dev/null 2>&1; then echo pacman;
  elif command -v dnf     > /dev/null 2>&1; then echo dnf;
  elif command -v zypper  > /dev/null 2>&1; then echo zypper;
  else echo none; fi
}

deps_missing() {
  command -v g++        > /dev/null 2>&1 || return 0
  command -v cmake      > /dev/null 2>&1 || return 0
  command -v pkg-config > /dev/null 2>&1 || return 0
  pkg-config --exists Qt6Widgets 2> /dev/null || return 0
  # Note: no 'grep -q' in the pipeline -- grep -q exits after the first
  # match and can trigger a non-deterministic SIGPIPE failure of ldconfig
  # under pipefail when ldconfig output is large.
  if ! pkg-config --exists Qt6Qscintilla 2> /dev/null \
     && ! pkg-config --exists qscintilla2_qt6 2> /dev/null \
     && [ -z "$(ldconfig -p 2> /dev/null | grep -i qscintilla)" ]; then
    return 0
  fi
  return 1
}

deps_install() {
  local pm; pm="$(pm_detect)"
  case "$pm" in
    apt)
      note "Installing dependencies (apt) ..."
      run_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
        build-essential cmake pkg-config qt6-base-dev libqscintilla2-qt6-dev
      ;;
    pacman)
      note "Installing dependencies (pacman) ..."
      run_root pacman -S --needed --noconfirm \
        base-devel cmake qt6-base qscintilla-qt6
      ;;
    dnf)
      note "Installing dependencies (dnf) ..."
      run_root dnf install -y \
        gcc-c++ make cmake qt6-qtbase-devel qscintilla-qt6-devel
      ;;
    zypper)
      note "Installing dependencies (zypper) ..."
      run_root zypper --non-interactive install \
        gcc-c++ make cmake qt6-base-devel qscintilla-qt6-devel
      ;;
    none)
      die "No known package manager found. Please install:
  a C++17 compiler, cmake, pkg-config, Qt6 (Widgets) and QScintilla for Qt6."
      ;;
  esac
}

try_install_file() {
  local src="$1" dst="$2"
  if install -Dm 644 "$src" "$dst" 2> /dev/null; then
    note "Installed: $dst"
  else
    run_root install -Dm 644 "$src" "$dst"
    note "Installed: $dst"
  fi
}

uninstall() {
  local f
  for f in "$BIN_FILE" "$DESKTOP_FILE" "$ICON_FILE"; do
    if [ -e "$f" ] || [ -L "$f" ]; then
      if [ -w "$(dirname "$f")" ]; then rm -f "$f"; else run_root rm -f "$f"; fi
      note "Removed: $f"
    fi
  done
  command -v update-desktop-database > /dev/null 2>&1 \
    && update-desktop-database "$PREFIX/share/applications" 2> /dev/null || true
  note "Uninstall complete."
}

main() {
  [ -f "$SCRIPT_DIR/main.cpp" ] || die "main.cpp not found (run the script from the project root)."
  [ -f "$SCRIPT_DIR/CMakeLists.txt" ] || die "CMakeLists.txt not found."

  if [ "${1:-}" = "--uninstall" ]; then uninstall; exit 0; fi
  [ "${1:-}" = "" ] || die "Unknown option: $1 (allowed: --uninstall)"

  if [ ! -w "$SCRIPT_DIR" ]; then die "Source directory is not writable: $SCRIPT_DIR"; fi

  if deps_missing; then
    deps_install
    deps_missing && die "Dependencies still incomplete after installation."
  else
    note "Dependencies are already present."
  fi

  note "Configuring and building NppLX (Release) ..."
  cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build" -DCMAKE_BUILD_TYPE=Release > /dev/null
  cmake --build "$SCRIPT_DIR/build" -j "$(nproc 2> /dev/null || echo 2)"

  try_install_file "$SCRIPT_DIR/build/npplx" "$BIN_FILE"
  chmod 755 "$BIN_FILE"
  if [ -f "$SCRIPT_DIR/assets/npplx.svg" ]; then
    try_install_file "$SCRIPT_DIR/assets/npplx.svg" "$ICON_FILE"
  fi
  if [ -f "$SCRIPT_DIR/assets/npplx.desktop" ]; then
    try_install_file "$SCRIPT_DIR/assets/npplx.desktop" "$DESKTOP_FILE"
    command -v update-desktop-database > /dev/null 2>&1 \
      && update-desktop-database "$PREFIX/share/applications" 2> /dev/null || true
  fi

  note "Done. Start with: npplx [file1 file2 ...]"
}

main "$@"
