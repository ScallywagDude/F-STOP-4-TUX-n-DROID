// Copies the BaseModUI windows from P2ASW into the client and fixes include case for the Linux build.
const fs = require('fs'), path = require('path');
const ROOT = (__dirname + '/../');
const SRC = ROOT + 'third_party/P2ASW/src/game/client/portal2/gameui/portal2/';
const OUT = ROOT + 'engine/game/client/portal2/basemodui/';
const names = ['basemodframe', 'vhybridbutton', 'vflyoutmenu', 'vfooterpanel', 'vmainmenu', 'vingamemainmenu',
  'vgenericconfirmation', 'vgenericpanellist', 'vgameoptions', 'vnewgamedialog', 'vsaveloadgamedialog', 'vvideo',
  'vadvancedvideo', 'vaudio', 'vkeyboardmouse', 'vkeybindings', 'vdropdownmenu', 'vslidercontrol', 'vspinnercontrol',
  'vdialoglistbutton', 'basemodpanel', 'uigamedata'];
fs.mkdirSync(OUT, { recursive: true });

// header index, so an include written in the wrong case resolves on a case-sensitive disk
const index = {};
function walk(dir, base) {
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    if (e.isDirectory()) { walk(dir + e.name + '/', base + e.name + '/'); continue; }
    if (!/\.(h|inl)$/i.test(e.name)) continue;
    index[(base + e.name).toLowerCase()] = base + e.name;
    if (!index[e.name.toLowerCase()]) index[e.name.toLowerCase()] = e.name;
  }
}
for (const d of ['public/', 'common/', 'game/shared/', 'game/client/', 'vgui2/include/', 'vgui2/vgui_controls/', 'public/tier0/', 'public/tier1/']) {
  if (fs.existsSync(ROOT + 'engine/' + d)) walk(ROOT + 'engine/' + d, '');
}
const own = {};
for (const n of names) own[n + '.h'] = n + '.h';

for (const n of names) {
  for (const ext of ['.cpp', '.h']) {
    const from = SRC + n + ext;
    if (!fs.existsSync(from)) continue;
    let t = fs.readFileSync(from, 'latin1');
    t = t.replace(/#include\s+"([^"]+)"/g, (m, inc) => {
      const low = inc.toLowerCase().replace(/\\/g, '/');
      const baseName = low.split('/').pop();
      if (own[baseName]) return '#include "' + own[baseName] + '"';
      if (index[low]) return '#include "' + index[low] + '"';
      return m;
    });
    fs.writeFileSync(OUT + n + ext, t, 'latin1');
  }
}
console.log('copied ' + names.length + ' units');
