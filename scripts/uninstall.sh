#!/usr/bin/env bash
# Remove Before Light leftovers. Safe after `omarchy plugin remove`.
# Also tries plugin remove first so one command is enough.
set -euo pipefail

HOME_DIR="${HOME}"
BIN_DST="${HOME_DIR}/.config/omarchy/bin"
SAVER_DST="${HOME_DIR}/.config/omarchy/branding/screensaver"
HYPR="${HOME_DIR}/.config/hypr/hyprland.lua"
CACHE="${XDG_CACHE_HOME:-${HOME_DIR}/.cache}/beforelight"

if command -v omarchy >/dev/null 2>&1; then
  omarchy plugin remove beforelight --yes >/dev/null 2>&1 || true
fi

for name in bouncingball fadeout fishsaver globe hardrain lifeforms lifeforms_new \
            logo matrix messages messages2 paperfire rainstorm randomizer spotlight \
            starrynight toastersaver warp worms; do
  pkill -9 -x "${name}" 2>/dev/null || true
done

rm -f \
  "${BIN_DST}/omarchy-beforelight" \
  "${BIN_DST}/omarchy-beforelight-settings" \
  "${BIN_DST}/omarchy-screensaver" \
  "${BIN_DST}/libbeforelight-overlay.so" \
  "${BIN_DST}/omarchy-beforelight-uninstall" \
  "${HOME_DIR}/.config/environment.d/90-beforelight.conf" \
  "${HOME_DIR}/.config/omarchy/beforelight.json" \
  "${HOME_DIR}/.config/omarchy/beforelight.setup"

rm -rf "${CACHE}" \
  "${XDG_RUNTIME_DIR:-/tmp}/beforelight.run.lock" \
  "${XDG_RUNTIME_DIR:-/tmp}/beforelight.preview.lock"

if [[ -d "${SAVER_DST}" ]]; then
  for name in bouncingball fadeout fishsaver globe hardrain lifeforms lifeforms_new \
              logo matrix messages messages2 paperfire rainstorm randomizer spotlight \
              starrynight toastersaver warp worms; do
    rm -f "${SAVER_DST}/${name}"
  done
fi

if [[ -f "${HYPR}" ]]; then
  python3 - "${HYPR}" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
text = p.read_text()
marker, end = "Before Light", "End Before Light"
start = text.find(marker)
if start < 0:
    sys.exit(0)
line_start = text.rfind("\n", 0, start) + 1
stop = text.find(end, start)
if stop < 0:
    new = text[:line_start]
else:
    nl = text.find("\n", stop)
    rest = text[nl + 1 :] if nl >= 0 else ""
    new = text[:line_start] + rest
p.write_text(new.rstrip() + "\n")
PY
  hyprctl reload >/dev/null 2>&1 || true
fi

echo "Before Light removed."
