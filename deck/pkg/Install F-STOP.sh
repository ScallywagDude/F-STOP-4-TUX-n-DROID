#!/bin/bash
# Adds F-STOP to the Steam library. Run it in Desktop Mode, with Steam open.
HERE="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
chmod +x "$HERE/F-STOP" "$HERE/hl2_launcher"

say() {
  if command -v kdialog >/dev/null 2>&1; then kdialog --title "F-STOP" --msgbox "$1"
  elif command -v zenity >/dev/null 2>&1; then zenity --info --title "F-STOP" --text "$1"
  else echo "$1"; fi
}

BYHAND="In Steam: Games > Add a Non-Steam Game > Browse, then choose $HERE/F-STOP"
if ! command -v steamos-add-to-steam >/dev/null 2>&1; then
  say "This system has no steamos-add-to-steam. $BYHAND"
  exit 1
fi
if steamos-add-to-steam "$HERE/F-STOP"; then
  say "F-STOP was added to your Steam library. Return to Game Mode and look under Non-Steam."
else
  say "Steam did not accept the request. Open Steam and try again. $BYHAND"
  exit 1
fi
