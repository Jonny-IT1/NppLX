#!/usr/bin/env bash
# NppLX uninstaller: removes NppLX completely from the machine.
#
# Usage:
#   ./uninstall.sh           removes the installed program (binary, menu
#                            entry, icon) from all common installation
#                            prefixes; also detects and removes a
#                            Debian package installation.
#   ./uninstall.sh --purge   additionally removes the entire project
#                            directory (source and build) completely.
#
# After ./uninstall.sh NppLX can be reinstalled at any time:
# ./install.sh in the source folder, or the .deb of the next release.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

note() { printf '==> %s\n' "$*"; }

run_root() {
  if [ "$(id -u)" -eq 0 ]; then "$@";
  elif command -v sudo > /dev/null 2>&1; then sudo "$@";
  else printf 'Error: root privileges required, but sudo is not available.\n' >&2; exit 1; fi
}

rm_file() {
  local f="$1"
  if [ -e "$f" ] || [ -L "$f" ]; then
    if [ -w "$(dirname "$f")" ]; then rm -f "$f"; else run_root rm -f "$f"; fi
    note "Removed: $f"
  fi
}

# 1) Debian package installation (.deb install path)
if command -v dpkg > /dev/null 2>&1 && dpkg -s npplx > /dev/null 2>&1; then
  note "Removing Debian package npplx ..."
  run_root env DEBIAN_FRONTEND=noninteractive apt-get remove -y npplx
fi

# 2) install.sh installations under all common prefixes
for prefix in "$HOME/.local" /usr/local /usr; do
  rm_file "$prefix/bin/npplx"
  rm_file "$prefix/share/applications/npplx.desktop"
  rm_file "$prefix/share/icons/hicolor/scalable/apps/npplx.svg"
done

if command -v update-desktop-database > /dev/null 2>&1; then
  for prefix in "$HOME/.local" /usr/local /usr; do
    [ -d "$prefix/share/applications" ] \
      && update-desktop-database "$prefix/share/applications" 2> /dev/null || true
  done
fi

# 3) --purge: remove the project directory completely
if [ "${1:-}" = "--purge" ]; then
  case "$SCRIPT_DIR" in
    /|$HOME|$HOME/) printf 'Error: --purge refused for unsafe path %s.\n' "$SCRIPT_DIR" >&2; exit 1 ;;
  esac
  note "Removing project directory: $SCRIPT_DIR (including this script)"
  cd /
  rm -rf "$SCRIPT_DIR"
elif [ -n "${1:-}" ]; then
  printf 'Error: unknown option: %s (allowed: --purge)\n' "$1" >&2
  exit 1
fi

note "Uninstall complete."
