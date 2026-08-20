#!/usr/bin/env bash
# Idempotent Before Light install. Safe to run from the plugin service on enable.
# Always compiles savers from engine/ on the machine. Does not ship or install
# prebuilt ELFs. Build outputs go to ~/.cache/beforelight, never the plugin tree.
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

CACHE="${XDG_CACHE_HOME:-${HOME_DIR}/.cache}/beforelight"
mkdir -p "${CACHE}"
VERSION="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1])).get("version",""))' "${ROOT}/manifest.json" 2>/dev/null || true)"
STAMP="${HOME_DIR}/.config/omarchy/beforelight.setup"
SKIP_BUILD=0
if [[ -n "${VERSION}" && -f "${STAMP}" && "$(cat "${STAMP}" 2>/dev/null)" == "${VERSION}" \
      && -x "${SAVER_DST}/toastersaver" && -f "${BIN_DST}/libbeforelight-overlay.so" ]]; then
  SKIP_BUILD=1
  log "Setup already complete for ${VERSION}; skipping compile"
fi

can_build_savers() {
  command -v gcc >/dev/null && command -v make >/dev/null && command -v sdl2-config >/dev/null
}

can_build_overlay() {
  command -v gcc >/dev/null && command -v make >/dev/null && command -v wayland-scanner >/dev/null && command -v pkg-config >/dev/null && pkg-config --exists wayland-client sdl3
}

install_overlay() {
  local dest="${BIN_DST}/libbeforelight-overlay.so"
  if can_build_overlay && [[ -d "${ROOT}/engine/overlay" ]]; then
    log "Building overlay shim from source…"
    mkdir -p "${CACHE}/overlay"
    make -C "${ROOT}/engine/overlay" -j"$(nproc)" OUTDIR="${CACHE}/overlay"
    install -Dm0755 "${CACHE}/overlay/libbeforelight-overlay.so" "${dest}"
    return 0
  fi
  log "No overlay shim; savers will use compositor fullscreen instead of layer-shell"
}

install_savers() {
  local src="$1"
  local f
  for f in "${src}"/*; do
    [[ -f "$f" && -x "$f" ]] || continue
    local base
    base="$(basename "$f")"
    [[ "$base" == "screensaver_config" ]] && continue
    [[ "$base" == *.so ]] && continue
    if command -v ldd >/dev/null; then
      if ldd "$f" 2>/dev/null | grep -q "not found"; then
        log "skip ${base}: missing shared library"
        continue
      fi
    fi
    install -Dm0755 "$f" "${SAVER_DST}/${base}"
  done
}

if [[ "${SKIP_BUILD}" -eq 0 ]]; then
  if ! can_build_savers || [[ ! -d "${ROOT}/engine" ]]; then
    log "Need gcc, make, and sdl2-config to compile savers from source."
  else
    install_overlay
    log "Building savers from source…"
    make -C "${ROOT}/engine" -j"$(nproc)" BUILD="${CACHE}/engine" all
    install_savers "${CACHE}/engine"
    if [[ -n "${VERSION}" ]]; then
      printf '%s\n' "${VERSION}" > "${STAMP}"
    fi
  fi
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

END_MARKER="End Before Light"
if [[ -f "${HYPR}" ]]; then
  python3 - "${HYPR}" "${ROOT}/scripts/hypr-snippet.lua" "${MARKER}" "${END_MARKER}" <<'PY'
import pathlib, sys
hypr, snippet, marker, end = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]), sys.argv[3], sys.argv[4]
text = hypr.read_text()
block = snippet.read_text().rstrip() + "\n"
start = text.find(marker)
if start < 0:
    hypr.write_text(text.rstrip() + "\n\n" + block)
    sys.exit(0)
# Include the comment leader on the marker line.
line_start = text.rfind("\n", 0, start) + 1
stop = text.find(end, start)
if stop < 0:
    rest = ""
    new = text[:line_start] + block
else:
    stop = text.find("\n", stop)
    rest = text[stop + 1 :] if stop >= 0 else ""
    new = text[:line_start] + block + rest
if new != text:
    hypr.write_text(new)
PY
  hyprctl reload >/dev/null 2>&1 || true
fi

log "Before Light is ready."
