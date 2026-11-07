F-STOP for Steam Deck
=====================

INSTALL
1. In Desktop Mode, copy F-STOP-SteamDeck.tar.gz to the Deck. It needs about 3.5 GB free once unpacked.
2. Right-click the file, choose Extract, Extract archive here. Move the F-STOP folder where you want to keep it
   (the internal drive or an SD card formatted by SteamOS). Move it before step 3, not after.
3. Open the F-STOP folder and double-click "Install F-STOP.sh". Choose Execute when asked. Steam must be open.
4. Return to Game Mode. F-STOP is in your library under Non-Steam.

The first map load takes longer while the graphics driver prepares its shaders.

CONTROLS
  Left stick     move                     Right stick    look
  R2             open viewfinder, take a photo, place a photo
  L2             close viewfinder, cancel placing
  Right trackpad    drag up or down: zoom in the viewfinder, photo size while placing
  A  jump        B  crouch (hold)         X  use         Y  switch held item
  L3 run         R3 stay crouched
  D-pad left     quick save               D-pad right    quick load
  Start          menu (Developer Console is at the top of it)
  View           pause

In menus: D-pad or left stick to move, A to choose, B to go back. B or Start closes the pause menu.

OTHER CONTROLLERS, OR BUMPERS ON THE DECK
Without a Steam Deck trackpad, the bumpers do the zooming and resizing: hold R1 or D-pad up for more, L1 or D-pad down for less.
On a Deck, turn this on under Options > Mouse > "Zoom and resize with the bumpers", or enter  fstop_bumper_resize 1  in the Developer Console.
If the trackpad feels upside down, enter  fstop_trackpad_invert 1  in the Developer Console.

GYRO AIMING (optional)
The game reads a standard controller. To aim with the gyro, press the Steam button, open Controller Settings
and set "Gyro Behavior" to "As Joystick" on the Right Stick layout.

IF SOMETHING IS WRONG
No controller response:  Steam button > Controller Settings > choose the "Gamepad" template.
Will not start:          in Steam, F-STOP's Properties > Compatibility > force "Steam Linux Runtime 3.0 (sniper)".
                         The game was built against that runtime.
Docked on a TV:          in Steam, F-STOP's Properties > Launch Options, enter:  -w 1920 -h 1080
Saves and settings live inside the F-STOP folder. Keep fstop/save and fstop/cfg if you reinstall.
