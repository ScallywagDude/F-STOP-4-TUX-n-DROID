# Partner-engine UI overlay

`tools/stage_win.sh` copies this overlay after the 1.1 release data. Keep it in
the game data for other platform packages too.

- `resource/gameui_english.txt`: shared settings labels from the local
  Project 2008 Alpha 2 `game/aperture/resource` data. BaseModUI's token table
  does not contain all of the desktop GameUI labels.
- `resource/valve_english.txt`: keyboard action labels from the installed
  Portal `hl2/resource` data.
- `resource/fstop_ui_english.txt`: labels added by newer menu controls that
  are absent from the original GameUI table.
- `resource/sourceschemebase.res`: standard VGUI fonts, colors and borders from
  the release's `platform/resource` directory. The loose SourceScheme includes
  this file; keeping the dependency beside it makes that include self-contained.
- `resource/ui/basemodui`: F-STOP menu layouts, widened for complete labels.
  The touch-only dialog is named "Touch options".
- `materials/vgui/menu_tiles/alt` and `materials/vgui/btn_bg*_alt.*`: original
  artwork from the installed Portal 2 `portal2_dlc2/pak01_dir.vpk`, extracted
  with `tools/p2ui_extract.js`. These are the alternate tiles/buttons referenced
  by the ported BaseModUI code.

The artwork and shared UI resources retain their original Valve provenance.
- `scripts/game_sounds_manifest.txt`: the 1.1 manifest plus `scripts/game_sounds_player_hl2.txt`, the HL2 player
  sounds (sprint, use-deny, flashlight) the original game loaded from `scripts/sounds/`; this engine reads only the
  manifest. The script itself is copied from the original install into `.port-build/mount_content`.
