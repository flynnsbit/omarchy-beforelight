#!/usr/bin/env bash
# Idempotent Before Light install. Safe to run from the plugin service on enable.
# Prefers shipped prebuilt savers so `omarchy plugin add` works without gcc.
set -euo pipefail

QUIET=0
[[ "${1:-}" == "--quiet" ]] && QUIET=1
log() { (( QUIET )) || echo "$*"; }

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="${HOME}"
BIN_DST="${HOME_DIR}/.config/omarchy/bin"
SAVER_DST="${HOME_DIR}/.config/omarchy/branding/screensaver"
HYPR="${HOME_DIR}/.config/hypr/hyprland.lua"
ENV_D="${HOME_DIR}/.config/environment.d/90-beforelight.conf"
MARKER="Before Light: put ~/.config/omarchy/bin ahead"

mkdir -p "${BIN_DST}" "${SAVER_DST}" "${HOME_DIR}/.config/environment.d" "$(dirname "${HYPR}")"

install -Dm0755 "${ROOT}/bin/omarchy-beforelight" "${BIN_DST}/omarchy-beforelight"
install -Dm0755 "${ROOT}/bin/omarchy-beforelight-settings" "${BIN_DST}/omarchy-beforelight-settings"
install -Dm0755 "${ROOT}/bin/omarchy-screensaver" "${BIN_DST}/omarchy-screensaver"

install_savers() {
  local src="$1"
  local f
  for f in "${src}"/*; do
    [[ -f "$f" && -x "$f" ]] || continue
    local base
    base="$(basename "$f")"
    [[ "$base" == "screensaver_config" ]] && continue
    if command -v ldd >/dev/null; then
      if ldd "$f" 2>/dev/null | grep -q "not found"; then
        log "skip ${base}: missing shared library"
        continue
      fi
    fi
    install -Dm0755 "$f" "${SAVER_DST}/${base}"
  done
}

if [[ -d "${ROOT}/prebuilt" ]] && compgen -G "${ROOT}/prebuilt/*" >/dev/null; then
  log "Installing prebuilt savers…"
  install_savers "${ROOT}/prebuilt"
elif [[ -d "${ROOT}/engine/build" ]] && compgen -G "${ROOT}/engine/build/*" >/dev/null; then
  log "Installing built savers…"
  install_savers "${ROOT}/engine/build"
elif command -v gcc >/dev/null && command -v sdl2-config >/dev/null; then
  log "Building savers…"
  make -C "${ROOT}/engine" -j"$(nproc)" all
  install_savers "${ROOT}/engine/build"
else
  log "No prebuilt savers and no compiler; picker will list whatever is already in ${SAVER_DST}"
fi

if [[ ! -f "${HOME_DIR}/.config/omarchy/beforelight.json" ]]; then
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

# Persist PATH so idle's `omarchy-screensaver` finds the wrapper, not packaged TTE.
if [[ ! -f "${ENV_D}" ]] || ! grep -q ".config/omarchy/bin" "${ENV_D}" 2>/dev/null; then
  printf 'PATH=%s/.config/omarchy/bin:${PATH}\n' "${HOME_DIR}" > "${ENV_D}"
fi

CURRENT_PATH="$(systemctl --user show-environment 2>/dev/null | sed -n 's/^PATH=//p' || true)"
if [[ -n "${CURRENT_PATH}" && "${CURRENT_PATH}" != "${HOME_DIR}/.config/omarchy/bin:"* ]]; then
  systemctl --user set-environment "PATH=${HOME_DIR}/.config/omarchy/bin:${CURRENT_PATH}" 2>/dev/null || true
fi
hyprctl eval "hl.env(\"PATH\", \"${HOME_DIR}/.config/omarchy/bin:\" .. (os.getenv(\"PATH\") or \"\"))" >/dev/null 2>&1 || true

if [[ -f "${HYPR}" ]] && ! grep -q "${MARKER}" "${HYPR}"; then
  log "Appending Hyprland PATH + window rules…"
  {
    echo
    echo "-- ${MARKER}"
    cat "${ROOT}/scripts/hypr-snippet.lua"
  } >> "${HYPR}"
  hyprctl reload >/dev/null 2>&1 || true
fi

log "Before Light is ready."
