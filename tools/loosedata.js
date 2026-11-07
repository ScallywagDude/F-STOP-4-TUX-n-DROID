// Files that replace what is in the packed archive: loose files win over the pak
const fs = require('fs');
const AP = (process.env.P2008_DIR + '/game/aperture/');
module.exports = G => {
  fs.mkdirSync(G + '/scripts', { recursive: true });
  // models whose files had to be matched up again, see modelfix.js
  const LM = (__dirname + '/../tools/loose_models');
  if (fs.existsSync(LM)) fs.cpSync(LM, G, { recursive: true });
  // materials and models that were missing: the real file where any build had it, otherwise the closest match
  const LT = (__dirname + '/../tools/loose_materials');
  if (fs.existsSync(LT)) fs.cpSync(LT, G, { recursive: true });
  fs.mkdirSync(G + '/resource', { recursive: true });

  // touch-sized main menu. The menu is proportional here: sizes are in 480-high units.
  fs.writeFileSync(G + '/resource/sourcescheme.res', '#base "SourceSchemeBase.res"\n\nScheme\n{\n\tBaseSettings\n\t{\n'
    + '\t\t"MainMenu.TextColor"\t\t\t"235 235 235 255"\n'
    + '\t\t"MainMenu.ArmedTextColor"\t\t"255 200 60 255"\n'
    + '\t\t"MainMenu.DepressedTextColor"\t"255 160 40 255"\n'
    + '\t\t"MainMenu.MenuItemHeight"\t\t"30"\n'
    + '\t\t"MainMenu.Inset"\t\t\t\t"16"\n'
    + '\t}\n\tFonts\n\t{\n'
    + '\t\t"MainMenuFont"\n\t\t{\n\t\t\t"1"\n\t\t\t{\n\t\t\t\t"name"\t\t"Verdana"\n\t\t\t\t"tall"\t\t"20"\n\t\t\t\t"weight"\t"700"\n\t\t\t\t"antialias"\t"1"\n\t\t\t}\n\t\t}\n'
    + '\t}\n}\n');

  // the title sits above the bigger menu, the menu starts high enough to fit every entry
  let cs = fs.readFileSync(AP + 'resource/clientscheme.res', 'latin1');
  cs = cs.replace(/"Main\.Title1\.Y"(\s+)"\d+"/, '"Main.Title1.Y"$1"16"')
         .replace(/"Main\.Menu\.Y"(\s+)"\d+"/, '"Main.Menu.Y"$1"120"')
         .replace(/"Main\.BottomBorder"(\s+)"\d+"/, '"Main.BottomBorder"$1"12"');
  fs.writeFileSync(G + '/resource/clientscheme.res', cs);

  // camera sounds: wave files shipped in the alpha, their script entries did not survive
  fs.writeFileSync(G + '/scripts/game_sounds_fstop_camera.txt', [
    ['Weapon_Camera.Capture', 'CHAN_WEAPON', '"wave"\t\t"camera/snapshot.wav"'],
    ['Weapon_Camera.Release', 'CHAN_WEAPON', '"wave"\t\t"camera/release.wav"'],
    ['PhotoInventory.Erased', 'CHAN_ITEM', '"wave"\t\t"camera/photo_erase1.wav"'],
    ['PhotoEraser.Fizzle', 'CHAN_ITEM', '"rndwave"\n\t{\n\t\t"wave"\t"camera/photo_erase1.wav"\n\t\t"wave"\t"camera/photo_erase2.wav"\n\t\t"wave"\t"camera/photo_erase3.wav"\n\t}'],
    ['HL2Player.PickupWeapon', 'CHAN_ITEM', '"wave"\t\t"items/ammo_pickup.wav"'],
  ].map(([n, c, w]) => '"' + n + '"\n{\n\t"channel"\t"' + c + '"\n\t"volume"\t"0.8"\n\t"soundlevel"\t"SNDLVL_75dB"\n\t' + w + '\n}\n').join('\n'));

  let man = fs.readFileSync(AP + 'scripts/game_sounds_manifest.txt', 'latin1');
  if (!man.includes('game_sounds_fstop_camera')) man = man.replace('{', '{\n\t"precache_file"\t\t"scripts/game_sounds_fstop_camera.txt"');
  fs.writeFileSync(G + '/scripts/game_sounds_manifest.txt', man);
};
