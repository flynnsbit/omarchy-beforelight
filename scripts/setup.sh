#!/usr/bin/env bash
# Install Before Light engine + idle hook after `omarchy plugin add`.
# Omarchy does not run plugin install hooks, so this step is required.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="${HOME}"
BIN_DST="${HOME_DIR}/.config/omarchy/bin"
SAVER_DST="${HOME_DIR}/.config/omarchy/branding/screensaver"
HYPR="${HOME_DIR}/.config/hypr/hyprland.lua"
MARKER="Before Light: put ~/.config/omarchy/bin ahead"

echo "Before Light setup from ${ROOT}"

need() { command -v "$1" >/dev/null || { echo "missing dependency: $1" >&2; exit 1; }; }
need gcc
need python3
need sdl2-config
need pkg-config
pkg-config --exists SDL2_image SDL2_ttf || echo "warning: SDL2_image / SDL2_ttf may be missing"

mkdir -p "${BIN_DST}" "${SAVER_DST}" "$(dirname "${HYPR}")"

echo "Building savers…"
make -C "${ROOT}/engine" -j"$(nproc)" all

echo "Installing binaries…"
install -Dm0755 "${ROOT}/bin/omarchy-beforelight" "${BIN_DST}/omarchy-beforelight"
install -Dm0755 "${ROOT}/bin/omarchy-beforelight-settings" "${BIN_DST}/omarchy-beforelight-settings"
install -Dm0755 "${ROOT}/bin/omarchy-screensaver" "${BIN_DST}/omarchy-screensaver"
find "${ROOT}/engine/build" -maxdepth 1 -type f -executable -exec install -Dm0755 {} "${SAVER_DST}/" \;

if [[ ! -f "${HOME_DIR}/.config/omarchy/beforelight.json" ]]; then
  mkdir -p "${HOME_DIR}/.config/omarchy"
  cat > "${HOME_DIR}/.config/omarchy/beforelight.json" <<'JSON'
{
  "engine": "beforelight",
  "selected": "toastersaver",
  "args": "",
  "previewSeconds": 12,
  "settings": {}
}
JSON
  chmod 600 "${HOME_DIR}/.config/omarchy/beforelight.json"
fi

if [[ -f "${HYPR}" ]] && ! grep -q "${MARKER}" "${HYPR}"; then
  echo "Appending Hyprland PATH + window rules…"
  {
    echo
    echo "-- ${MARKER}"
    cat "${ROOT}/scripts/hypr-snippet.lua"
  } >> "${HYPR}"
fi

if command -v omarchy >/dev/null; then
  omarchy plugin validate "${ROOT}" || true
fi

echo
echo "Setup complete."
echo "1. Add the bar widget if it is not already there:"
echo "     omarchy plugin enable io.github.flynnsbit.beforelight"
echo "     omarchy bar add io.github.flynnsbit.beforelight --section right"
echo "2. Reload Hyprland (hyprctl reload) and restart the shell:"
echo "     omarchy restart shell"
echo "3. Click the Before Light icon on the bar."
