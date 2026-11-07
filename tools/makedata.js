// Assembles the final game folder: fsdata/fstop + fsdata/platform.
const fs = require('fs'), path = require('path'), cp = require('child_process');
const ROOT = (__dirname + '/../fsdata'), G = ROOT + '/fstop', OUT = (__dirname + '/../out/');
const C = 'C:/Program Files (x86)/Steam/steamapps/common/Portal/';
fs.rmSync(ROOT, { recursive: true, force: true });
fs.mkdirSync(G + '/maps', { recursive: true }); fs.mkdirSync(G + '/cfg', { recursive: true }); fs.mkdirSync(G + '/resource', { recursive: true }); fs.mkdirSync(ROOT + '/platform', { recursive: true });

// add the shaders that match this engine to the bundle
const list = JSON.parse(fs.readFileSync(OUT + 'bundle.json', 'latin1'));
const SH = (process.env.FSTOP_INPUT + '/source2018_assets/gmodshaders/shaders/fxc');
for (const f of fs.readdirSync(SH)) { const full = SH + '/' + f; list['shaders/fxc/' + f.toLowerCase()] = { file: full, size: fs.statSync(full).size }; }
// extras_merge: touch icons and option panels from the launcher's archive, without its shaders
{
  const X = (__dirname + '/../launcher/assets/extras_dir.vpk');
  const b = fs.readFileSync(X); let p = 0, tree = 0;
  if (b.readUInt32LE(0) === 0x55aa1234) { tree = b.readUInt32LE(4) === 1 ? 12 : 28; p = tree; }
  const dataStart = tree + (tree ? b.readUInt32LE(8) : 0);
  const str = () => { const s = p; while (b[p] !== 0) p++; const r = b.toString('latin1', s, p); p++; return r; };
  let n = 0;
  for (;;) { const e = str(); if (!e) break; for (;;) { const d = str(); if (!d) break; for (;;) { const nm = str(); if (!nm) break;
    p += 4; const pre = b.readUInt16LE(p); p += 2; const arc = b.readUInt16LE(p); p += 2; const off = b.readUInt32LE(p); p += 4; const len = b.readUInt32LE(p); p += 4; p += 2; p += pre;
    const full = ((d === ' ' ? '' : d + '/') + nm + '.' + e).toLowerCase();
    if (/^shaders\//.test(full) || arc !== 0x7fff) continue;
    const dst = OUT + 'extras/' + full; fs.mkdirSync(require('path').dirname(dst), { recursive: true }); fs.writeFileSync(dst, b.subarray(dataStart + off, dataStart + off + len));
    list[full] = { file: dst, size: len }; n++;
  } } }
  console.log('extras merged', n);
}
fs.writeFileSync(OUT + 'bundle.json', JSON.stringify(list));
cp.execFileSync('node', ['--max-old-space-size=8192', (__dirname + '/../tools/makevpk.js'), ROOT], { stdio: 'inherit' });

// maps
for (const f of fs.readdirSync((__dirname + '/../maps'))) fs.copyFileSync((__dirname + '/../maps/') + f, G + '/maps/' + f);

// platform UI files (English only)
const OTHER = /_(brazilian|bulgarian|czech|danish|dutch|finnish|french|german|greek|hungarian|italian|japanese|korean|koreana|latam|norwegian|polish|portuguese|romanian|russian|schinese|spanish|swedish|tchinese|thai|turkish|ukrainian|vietnamese|pirate)\.(txt|dat)$/i;
(function w(src, dst) { for (const e of fs.readdirSync(src, { withFileTypes: true })) { const a = src + '/' + e.name, b = dst + '/' + e.name;
  if (e.isDirectory()) { if (!/^(config|gamepadui)$/i.test(e.name)) w(a, b); } else if (!/\.vpk$/.test(e.name) && !OTHER.test(e.name)) { fs.mkdirSync(dst, { recursive: true }); fs.copyFileSync(a, b); } } })(C + 'platform', ROOT + '/platform');
// platform
for (const f of fs.readdirSync(C + 'platform')) if (/platform_misc.*\.vpk$/.test(f)) fs.copyFileSync(C + 'platform/' + f, ROOT + '/platform/' + f);

fs.writeFileSync(G + '/gameinfo.txt', `"GameInfo"
{
	game	"F-STOP"
	title	""
	type	singleplayer_only
	nodifficulty	1
	hasportals	1
	FileSystem
	{
		SteamAppId	400
		SearchPaths
		{
			mod+mod_write+default_write_path	|gameinfo_path|.
			game+game_write	|gameinfo_path|.
			game+mod	fstop/fstop_pak.vpk
			platform	|all_source_engine_paths|platform/platform_misc.vpk
			game	fstop
			platform	|all_source_engine_paths|platform
		}
	}
}
`);

// New Game chapters: the campaign in map order, then the standalone maps.
const chapters = [
  ['The Lab', 'lab_intro'], ['Fans', 'fans_01'], ['Mousetraps', 'mousetraps_01'], ['Training Doors', 'aperture_training_doors'],
  ['Chickens', 'puzzle_chickens'], ['The Barn', 'lab_barn'], ['Fan Lift', 'lab_fan_lift'], ['The Farm', 'farm'],
  ['Training Hub', 'training_hub'], ['Training', 'training'], ['Aperture Training', 'aperture_training'], ['Training Fans', 'aperture_training_fans'],
  ['Training Mousetraps', 'aperture_training_mousetraps'], ['Training Island', 'aperture_training_island'], ['Island', 'island'],
  ['Chicken', 'chicken'], ['Chicken Farm', 'chicken_farm'], ['Chicken Farm 2', 'chicken_farm_2'], ['Chicken Fling', 'puzzle_chicken_fling'],
  ['Swap 1', 'swap_01'], ['Swap 2', 'swap_02'], ['Zombies', 'zombies'], ['Zombie Training', 'zombie_training'],
  ['1970s', '1970s'], ['Fan Test', 'fan_test'], ['Lab Concept', 'lab_concept'], ['Lab 3 Concept', 'lab_03_concept'], ['Lab Concept Barn', 'lab_concept_barn'],
];
const missingMaps = chapters.filter(c => !fs.existsSync(G + '/maps/' + c[1] + '.bsp'));
if (missingMaps.length) throw new Error('chapter map missing: ' + missingMaps.map(c => c[1]));
let kv = '"lang"\n{\n"Language" "English"\n"Tokens"\n{\n"GameUI_GameMenu_NewGame" "New Game"\n';
chapters.forEach((c, i) => {
  fs.writeFileSync(`${G}/cfg/chapter${i + 1}.cfg`, `map ${c[1]}\n`);
  kv += `"fstop_chapter${i + 1}_title" "${c[0]}"\n"fstop_Chapter${i + 1}_Title" "${c[0]}"\n`;
});
kv += '}\n}\n';
// fstop_english.txt: F-Stop's own strings plus chapter names (engine reads UCS-2)
const ap = list['resource/aperture_english.txt'];
let base = '';
if (ap && ap.file) { const b = fs.readFileSync(ap.file); base = b[0] === 0xff ? b.toString('utf16le').replace(/^\ufeff/, '') : b.toString('latin1'); }
let merged = kv;
if (base) { const i = base.lastIndexOf('}', base.lastIndexOf('}') - 1); merged = base.slice(0, i) + kv.split('"Tokens"\n{\n')[1].split('}\n}\n')[0] + base.slice(i); }
fs.writeFileSync(G + '/resource/fstop_english.txt', Buffer.concat([Buffer.from([0xff, 0xfe]), Buffer.from(merged, 'utf16le')]));

// fstop-startup: no background map, so the menu shows straight away over a still image
fs.writeFileSync(G + '/cfg/valve.rc', 'exec joystick.cfg\nexec autoexec.cfg\nstuffcmds\n');
fs.writeFileSync(G + '/cfg/config_default.cfg', "mat_queue_mode 0\nmat_picmip 0\nr_rootlod 0\nmat_forceaniso 16\nmat_trilinear 1\nmat_hdr_level 2\nmat_bumpmap 1\nmat_specular 1\nmat_colorcorrection 1\nr_waterforceexpensive 1\nr_waterforcereflectentities 1\nr_shadowrendertotexture 1\nr_flashlightdepthtexture 1\n", 'latin1');
fs.writeFileSync(G + '/cfg/autoexec.cfg', [
  'sv_cheats 1',
  
  // the same quality settings as the PC build
  'r_lod 0', 
  'mat_phong 1', 'mat_bloomscale 1',
  'r_shadows 1', 
  'r_3dsky 1', 'r_drawdetailprops 1', 'cl_detaildist 1200', 'cl_detailfade 400', 'r_decals 2048', 'mp_decals 200', 'r_staticprop_lod -1',
  'r_portal_stencil_depth 2', 'portal_stencil_depth 2', 'fstop_apply_detail', '',
].join('\n'));
const HR = C + 'hl2/resource/';
fs.copyFileSync((__dirname + '/../tools/menu_scheme.res'), G + '/resource/sourcescheme.res');
// Menu: only what makes sense on a phone. "Console" is added by the engine itself.
fs.writeFileSync(G + '/resource/gamemenu.res', `"GameMenu"
{
	"1" { "label" "#GameUI_GameMenu_ResumeGame" "command" "ResumeGame" "OnlyInGame" "1" }
	"2" { "label" "#GameUI_GameMenu_NewGame" "command" "OpenNewGameDialog" }
	"3" { "label" "#GameUI_GameMenu_LoadGame" "command" "OpenLoadGameDialog" }
	"4" { "label" "#GameUI_GameMenu_SaveGame" "command" "OpenSaveGameDialog" "OnlyInGame" "1" }
	"5" { "label" "#GameUI_GameMenu_Options" "command" "OpenOptionsDialog" }
	"6" { "label" "#GameUI_GameMenu_Quit" "command" "Quit" }
}
`);

require('./loosedata.js')(G);

let size = 0, files = 0;
(function w(d) { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const f = d + '/' + e.name; if (e.isDirectory()) w(f); else { size += fs.statSync(f).size; files++; } } })(ROOT);
console.log('fsdata', files, 'files', Math.round(size / 1048576), 'MB, chapters', chapters.length);
