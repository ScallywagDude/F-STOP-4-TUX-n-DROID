// Project 2008's sea material. The water shader expects its reflection the other way up from what func_reflective_glass renders, so the sea uses the shader made for that
// glass, over a flat sea colour so it stays opaque. usage: node tools/p2k_seamat.js <folder to write materials/water into>
const fs = require('fs'), path = require('path');
const root = process.argv[2];
fs.mkdirSync(path.join(root, 'materials/water'), { recursive: true });

// a 4x4 RGBA8888 VTF, version 7.2, no mips, clamped
const [r, g, b] = [24, 70, 84];
const W = 4, H = 4, HEADER = 80;
const v = Buffer.alloc(HEADER + W * H * 4);
v.write('VTF\0', 0, 'latin1');
v.writeUInt32LE(7, 4); v.writeUInt32LE(2, 8); v.writeUInt32LE(HEADER, 12);
v.writeUInt16LE(W, 16); v.writeUInt16LE(H, 18);
v.writeUInt32LE(0x4 | 0x8 | 0x100 | 0x2000 | 0x4000, 20);
v.writeUInt16LE(1, 24); v.writeUInt16LE(0, 26);
v.writeFloatLE(0.2, 32); v.writeFloatLE(0.2, 36); v.writeFloatLE(0.2, 40);
v.writeFloatLE(1, 48);
v.writeUInt32LE(0, 52);
v[56] = 1;
v.writeInt32LE(-1, 57);
v.writeUInt16LE(1, 63);
for (let i = HEADER; i < v.length; i += 4) { v[i] = r; v[i + 1] = g; v[i + 2] = b; v[i + 3] = 255; }
fs.writeFileSync(path.join(root, 'materials/water/fstop_seacolor.vtf'), v);

fs.writeFileSync(path.join(root, 'materials/water/water_2fort.vmt'), `"LightmappedReflective"
{
	"$refracttexture" "water/fstop_seacolor"
	"$refractamount" "0"
	"$refracttint" "[1 1 1]"
	"$reflecttexture" "_rt_WaterReflection"
	"$reflectamount" "0.01"
	"$reflecttint" "[.55 .72 .8]"
	"$fresnelreflection" "0.08"
	"$normalmap" "water/tfwater001_normal"
	"$surfaceprop" "water"
	"$bumpframe" "0"
	"%compilewater" 1

	"$temp" "[0 0]"
	"$curr" 0.0
	"$curr2" 0.0

	"Proxies"
	{
		"AnimatedTexture"
		{
			"animatedtexturevar" "$normalmap"
			"animatedtextureframenumvar" "$bumpframe"
			"animatedtextureframerate" 30.00
		}
		"Sine"
		{
			"sineperiod" "24"
			"sinemin" -0.5
			"sinemax" 0.5
			"resultVar" "$curr"
		}
		"Sine"
		{
			"sineperiod" "16"
			"sinemin" 0.5
			"sinemax" -0.5
			"resultVar" "$curr2"
		}
		"Equals"
		{
			"srcVar1" "$curr2"
			"resultVar" "$temp[0]"
		}
		"Equals"
		{
			"srcVar1" "$curr"
			"resultVar" "$temp[1]"
		}
		"TextureTransform"
		{
			"translateVar" "$temp"
			"resultVar" "$bumptransform"
		}
	}
}
`, 'latin1');
console.log('sea material written');
