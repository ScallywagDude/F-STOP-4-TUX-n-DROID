#!/usr/bin/env python
# Writes Waf scripts from the partner VPC projects, so the Windows, Linux and Android source lists all come
# from the engine's own project files. Each wscript carries the sources shared by every platform plus the
# Windows-only and POSIX-only ones, the project's include folders and its own preprocessor definitions.
#
# usage: gen_wscripts.py [project folders...]      (no arguments: every project in the table)
#        gen_wscripts.py --check                    (report only: missing files and unmapped libraries)
#
# Hand-maintained wscripts (tier0, tier1, vstdlib, filesystem, materialsystem, ... ) are not in the table.
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vpcread import Project  # noqa: E402

SRC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src'))

# Definitions the top-level wscript already sets for every target, or that select the CS:GO product build.
GLOBAL_DEFINES = {
    'VPC', 'WIN32', '_WIN32', 'NDEBUG', '_WINDOWS', '_CRT_SECURE_NO_DEPRECATE', '_CRT_NONSTDC_NO_DEPRECATE',
    '_ALLOW_RUNTIME_LIBRARY_MISMATCH', '_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH', '_ALLOW_MSC_VER_MISMATCH',
    '%(PreprocessorDefinitions)', 'PLATFORM_64BITS', 'WIN64', '_WIN64', 'COMPILER_MSVC64', 'COMPILER_MSVC',
    'GNUC', 'POSIX', '_POSIX', 'COMPILER_GCC', '_LINUX', 'LINUX', '_DLL_EXT=.dll', '_DLL_EXT=.so',
    'CSTRIKE_REL_BUILD=1', 'RAD_TELEMETRY_DISABLED', 'INCLUDE_SCALEFORM', 'PANORAMA_ENABLE',
    'SURVIVAL_MODE_ENABLED=1', 'CSTRIKE15', '_USRDLL', '_HAS_ITERATOR_DEBUGGING=0', 'USE_SDL', 'DX_TO_GL_ABSTRACTION',
}

# VPC library name -> Waf name (a target in this build, or an uppercase system/configured library).
LIB_NAMES = {
    'libz': 'ZLIB', 'zlib': 'ZLIB', 'libpng': 'PNG', 'libjpeg': 'JPEG', 'jpeglib': 'JPEG', 'sdl2': 'SDL2',
    'steam_api': 'steam_api', 'steam_api64': 'steam_api', 'bzip2': 'bzip2', 'nvtc': 'NVTC', 'nvtc64': 'NVTC',
    'vgui_surfacelib': 'vgui_surfacelib', 'vgui_controls': 'vgui_controls',
    'dsound': 'DSOUND', 'dxguid': 'DXGUID', 'd3d9': 'D3D9', 'd3dx9': 'D3DX9',
    'cryptlib': 'CRYPTLIB', 'nvapi': 'NVAPI', 'ws2_32': 'WS2_32', 'winmm': 'WINMM', 'wsock32': 'WS2_32',
    'imm32': 'IMM32', 'shlwapi': 'SHLWAPI', 'rpcrt4': 'RPCRT4', 'wininet': 'WININET', 'psapi': 'PSAPI',
    'dbghelp': 'DBGHELP', 'iphlpapi': 'IPHLPAPI', 'dinput8': 'DINPUT8', 'vfw32': 'VFW32',
    # Crypto++: prebuilt on Windows (CRYPTLIB), built from src/external/crypto++-5.61 for the POSIX targets
    'libcryptopp_client': 'cryptopp', 'libcryptopp': 'cryptopp',
}
# linker inputs that every Windows module already gets (WIN_SYS) or that are not needed
IGNORED_LIBS = {'shell32', 'user32', 'advapi32', 'gdi32', 'comdlg32', 'ole32', 'odbc32', 'odbccp32', 'kernel32',
                'oleaut32', 'uuid', 'version', 'winspool', 'comctl32', 'legacy_stdio_definitions'}
WIN_SYS = ['USER32', 'SHELL32', 'GDI32', 'ADVAPI32', 'WS2_32', 'WINMM', 'OLE32']
# system libraries the POSIX projects reach through their headers (VPC names them as linker flags, which the reader
# does not keep); a name the platform did not configure is ignored by Waf
POSIX_SYS = ['SDL2', 'FT2', 'FC', 'OPENAL', 'ZLIB']
# VPC conditionals this port keeps off on every platform. Panorama is CS:GO's Scaleform successor; the partner scripts
# turn it on for 32-bit Windows and Linux only, and F-STOP's 64-bit Windows build (the reference) runs without it.
OFF_CONDITIONALS = ['PANORAMA_ENABLE']

# folder: dict(vpc=, target=, kind=stlib|shlib|program, use=[extra], drop=[sources], add=[sources],
#              defines=[extra], console=bool, windows_only=bool, nolibs=[vpc libs to ignore],
#              includes=[extra folders], noincludes=[folders to leave out], win_use=[extra Windows libs], win_defines=[extra Windows defines],
#              posix_use=[extra POSIX libs: system libraries the VPC links as files or linker flags, which the reader skips],
#              android_src=[sources added on Android only, on top of the POSIX ones],
#              android_drop=[POSIX sources left out on Android],
#              embed=[(text file, array name, header)] for texttoarray-style generated headers)
PROJECTS = {
    # engine libraries
    'videocfg': dict(vpc='videocfg.vpc', target='videocfg', kind='stlib'),
    'bonesetup': dict(vpc='bonesetup.vpc', target='bonesetup', kind='stlib'),
    'choreoobjects': dict(vpc='choreoobjects.vpc', target='choreoobjects', kind='stlib'),
    'datamodel': dict(vpc='datamodel.vpc', target='datamodel', kind='stlib'),
    'dmxloader': dict(vpc='dmxloader.vpc', target='dmxloader', kind='stlib'),
    'particles': dict(vpc='particles.vpc', target='particles', kind='stlib'),
    'responserules/runtime': dict(vpc='responserules_runtime.vpc', target='responserules_runtime', kind='stlib'),
    'thirdparty/quickhull': dict(vpc='quickhull.vpc', target='quickhull', kind='stlib'),
    'matchmaking': dict(vpc='matchmaking_base.vpc', target='matchmakingbase', kind='stlib', nolibs=['gcsdk']),
    'soundsystem/lowlevel': dict(vpc='soundsystem_lowlevel.vpc', target='soundsystem_lowlevel', kind='stlib'),
    'vgui2/vgui_controls': dict(vpc='vgui_controls.vpc', target='vgui_controls', kind='stlib'),
    'vgui2/vgui_surfacelib': dict(vpc='vgui_surfacelib.vpc', target='vgui_surfacelib', kind='stlib'),
    'vgui2/matsys_controls': dict(vpc='matsys_controls.vpc', target='matsys_controls', kind='stlib'),
    # engine modules
    'datacache': dict(vpc='datacache.vpc', target='datacache', kind='shlib'),
    'studiorender': dict(vpc='studiorender.vpc', target='studiorender', kind='shlib'),
    'inputsystem': dict(vpc='inputsystem.vpc', target='inputsystem', kind='shlib'),
    'vgui2/src': dict(vpc='vgui_dll.vpc', target='vgui2', kind='shlib', win_use=['IMM32', 'SHLWAPI']),
    'vguimatsurface': dict(vpc='vguimatsurface.vpc', target='vguimatsurface', kind='shlib', nolibs=['libcef']),
    'scenefilecache': dict(vpc='scenefilecache.vpc', target='scenefilecache', kind='shlib'),
    'soundemittersystem': dict(vpc='soundemittersystem.vpc', target='soundemittersystem', kind='shlib'),
    'localize': dict(vpc='localize.vpc', target='localize', kind='shlib'),
    'materialsystem/shaderapidx9': dict(vpc='shaderapidx9.vpc', target='shaderapidx9', kind='shlib',
                                        # Windows debug builds compile shaders on demand from src/materialsystem/stdshaders
                                        win_defines=['DYNAMIC_SHADER_COMPILE']),
    # Android: SDLActivity loads liblauncher.so and calls LauncherMainAndroid (launcher/android/main.cpp)
    'launcher': dict(vpc='launcher.vpc', target='launcher', kind='shlib',
                     android_src=['android/main.cpp', 'android/crashhandler.cpp']),
    # Direct3D-to-OpenGL translation for the desktop POSIX renderer; the VPC gates its sources on $GL
    'togl': dict(vpc='togl.vpc', target='togl', kind='shlib', conditionals=['GL']),
    'avi': dict(vpc='valve_avi.vpc', target='valve_avi', kind='shlib', nolibs=['libsdl2_mixer']),
    # the title module: CS:GO's for now, until F-STOP has its own mm_title sources
    'matchmaking/cstrike15': dict(vpc='../matchmaking_cstrike15.vpc', target='matchmaking', kind='shlib', nolibs=['gcsdk'],
                                  add=['../../common/steamid.cpp']),  # CSteamID::Render, which gcsdk.lib supplies in the partner build
    'launcher_main': dict(vpc='launcher_main.vpc', target='launcher_main', kind='program'),
    'engine': dict(vpc='engine.vpc', target='engine', kind='shlib', use=['protobuf'],
                   # Steam networking sockets and Steam Audio are external binaries this build does not ship:
                   # stub_steam answers for the first, audio/private/phonon_unavailable.cpp for the second
                   nolibs=['libprotobuf', 'steamnetworkingsockets', 'phonon'],
                   # Android has no OpenAL: voice capture goes through SDL there
                   android_drop=['audio/private/voice_record_openal.cpp'],
                   android_src=['audio/private/voice_record_sdl.cpp'],
                   # downloadthread.cpp: the VPC links thirdparty/libcurl/lib/linux64/libcurl.so as a $File
                   posix_use=['CURL'],
                   add=['audio/private/phonon_unavailable.cpp'],
                   # the old DirectX SDK copy of ksmedia.h/ks.h there breaks the Windows SDK audio headers
                   noincludes=['../dx9sdk/include']),
    'vscript': dict(vpc='vscript.vpc', target='vscript', kind='shlib',
                    # the VPC sets these per file; one list for the module is equivalent
                    includes=['languages/gm/src/gm', 'languages/gm/src/binds', 'languages/gm/src/platform/win32msvc',
                              'languages/lua/lua-5.1.4/src', 'languages/squirrel/include', 'languages/squirrel/sqplus'],
                    defines=['PROTECTED_THINGS_DISABLE'],
                    embed=[('languages/squirrel/vsquirrel/init.nut', 'g_Script_init',
                            'languages/squirrel/vsquirrel/init_nut.h')]),
    # F-STOP game code: CS:GO's client/server base with the F-STOP (Portal) game folders on top
    'mathlib/extended': dict(vpc='../mathlib_extended.vpc', target='mathlib_extended', kind='stlib'),
    'kv3lib': dict(vpc='kv3lib.vpc', target='kv3lib', kind='stlib', defines=['CSTRIKE15']),  # the library was only ever built for CS:GO
    'resourcefile': dict(vpc='resourcefile.vpc', target='resourcefile', kind='stlib'),
    'game/client': dict(vpc='client_fstop.vpc', target='client', kind='shlib', nolibs=['gcsdk', 'libprotobuf'], use=['protobuf'],
                        posix_use=['RT'],  # mumble.cpp: shm_open, in librt before glibc 2.34
                        add=['generated_proto/steammessages.pb.cc', 'portal/c_prop_gun_portal.cpp', 'portal/c_prop_door_portal.cpp', '../shared/portal/gun_portal_shared.cpp'], includes=['../shared/fstop_csstubs'],
                        embed=[('vscript_client.nut', 'g_Script_vscript_client', 'vscript_client_nut.h')]),  # gcsdk.lib supplies it in the partner build
    'game/server': dict(vpc='server_fstop.vpc', target='server', kind='shlib', nolibs=['gcsdk', 'libprotobuf'], use=['protobuf'],
                        add=['generated_proto/steammessages.pb.cc', 'portal/prop_gun_portal.cpp', 'portal/prop_door_portal.cpp', '../shared/portal/gun_portal_shared.cpp'], includes=['../shared/fstop_csstubs'],
                        embed=[('spawn_helper.nut', 'g_Script_spawn_helper', 'spawn_helper_nut.h'), ('vscript_server.nut', 'g_Script_vscript_server', 'vscript_server_nut.h')]),  # gcsdk.lib supplies it in the partner build
    # map compilers
    'utils/bsppack': dict(vpc='bsppack.vpc', target='bsppack', kind='shlib', windows_only=True, use=['lzma', 'mathlib_extended'], add=['../../public/collisionutils.cpp'], nolibs=['libcurl']),
    'raytrace': dict(vpc='raytrace.vpc', target='raytrace', kind='stlib'),
    'fgdlib': dict(vpc='fgdlib.vpc', target='fgdlib', kind='stlib'),
    'utils/lzma': dict(vpc='lzma.vpc', target='lzma', kind='stlib'),
    'utils/vmpi': dict(vpc='vmpi.vpc', target='vmpi', kind='stlib', windows_only=True),
    'utils/vbsp': dict(vpc='vbsp.vpc', target='vbsp', kind='program', console=True, windows_only=True,
                       use=['tier1', 'interfaces', 'tier0', 'vstdlib'], nolibs=['libmad']),
    'utils/vvis': dict(vpc='vvis_dll.vpc', target='vvis_dll', kind='shlib', windows_only=True,
                       use=['lzma'], nolibs=['libcurl']),
    'utils/vrad': dict(vpc='vrad_dll.vpc', target='vrad_dll', kind='shlib', windows_only=True,
                       use=['lzma'], nolibs=['libcurl']),
    'utils/vvis_launcher': dict(vpc='vvis_launcher.vpc', target='vvis', kind='program', console=True,
                                windows_only=True, use=['tier1', 'interfaces', 'tier0', 'vstdlib']),
    'utils/vrad_launcher': dict(vpc='vrad_launcher.vpc', target='vrad', kind='program', console=True,
                                windows_only=True, use=['tier1', 'interfaces', 'tier0', 'vstdlib']),
    # shader compiler: dx_proxy wraps D3DX9's HLSL compiler, the launcher loads shadercompile_dll
    'utils/dx_proxy': dict(vpc='dx_proxy_dx9_v00_pc.vpc', target='dx_proxy', kind='shlib', windows_only=True,
                           win_use=['D3DX9', 'D3D9']),
    'utils/shadercompile': dict(vpc='shadercompile_dll.vpc', target='shadercompile_dll', kind='shlib',
                                windows_only=True, use=['vmpi', 'lzma'], nolibs=['libcurl']),
    'utils/shadercompile_launcher': dict(vpc='shadercompile_launcher.vpc', target='shadercompile', kind='program',
                                         console=True, windows_only=True,
                                         use=['tier1', 'interfaces', 'tier0', 'vstdlib']),
}

TEMPLATE = '''#! /usr/bin/env python
# encoding: utf-8
# Generated by tools/gen_wscripts.py from {vpc}; edit the table there, not this file.

top = '.'
PROJECT_NAME = '{target}'

def options(opt):
	return

def configure(conf):
	return

def build(bld):
{guard}	source = {common}
	includes = {includes}
	defines = {defines}
	libs = {use}

	if bld.env.DEST_OS == 'win32':
		source += {win_src}
		defines += {win_def}
		libs += {win_use}
	else:
		source += {posix_src}
		includes += {posix_inc}
		defines += {posix_def}
		libs += {posix_use}
{android}
	bld{call}(
		source   = source,
		target   = PROJECT_NAME,
		name     = PROJECT_NAME,
		features = '{features}',
		includes = includes,
		defines  = defines,
		use      = libs,
		install_path = {install},
		subsystem = {subsystem},
		idx      = bld.get_taskgen_count()
	)
'''


def fmt(items, indent='\t'):
    if not items:
        return '[]'
    return '[\n' + ''.join("%s\t'%s',\n" % (indent, i) for i in items) + indent + ']'


def clean_defines(project, target):
    out = []
    for d in project.defines:
        d = d.replace('$PROJECTNAME', target).replace('$OUTBINNAME', target).replace('$OUTLIBNAME', target)
        if d in GLOBAL_DEFINES or '$' in d or d in out or not re.match(r'^\w', d):
            continue
        out.append(d)
    return out


def map_libs(project, cfg, known, problems, folder):
    out = []
    for lib in project.libs:
        if lib.startswith('-'):
            continue
        name = os.path.splitext(os.path.basename(lib))[0].lower()
        if name.startswith('lib') and name[3:] in known:
            name = name[3:]
        if name in cfg.get('nolibs', []) or name in IGNORED_LIBS:
            continue
        waf = name if name in known else LIB_NAMES.get(name)
        if not waf:
            problems.append('%s: unmapped library %s' % (folder, lib))
            continue
        if waf not in out:
            out.append(waf)
    return out


def generate(folder, cfg, known, check, problems):
    base = os.path.join(SRC, folder)
    target = cfg['target']
    plats = {}
    for plat in (['win64'] if cfg.get('windows_only') else ['win64', 'linux64']):
        plats[plat] = Project(os.path.join(base, cfg['vpc']), plat, cfg.get('conditionals', []), OFF_CONDITIONALS)
    win = plats['win64']
    posix = plats.get('linux64')

    def here(p, path):
        # project paths are relative to the .vpc; the wscript may sit in another folder (vpc='../x.vpc')
        if os.path.normcase(p.base) == os.path.normcase(os.path.normpath(base)):
            return path
        return os.path.relpath(os.path.join(p.base, path), base).replace(os.sep, '/')

    def srcs(p):
        if p is None:
            return []
        out = [s for s in [here(p, x) for x in p.sources()] if s not in cfg.get('drop', [])] + cfg.get('add', [])
        for proto in [s for s in out if s.endswith('.proto')]:
            # the project lists generated_proto/<name>.pb.cc itself; the .proto only drives protoc
            out.remove(proto)
            protoc(base, proto, p.macros.get('GENERATED_PROTO_DIR', 'generated_proto'), check, problems, folder)
        for s in out:
            if not os.path.exists(os.path.join(base, s)):
                problems.append('%s: missing source %s' % (folder, s))
        return out

    ws, ps = srcs(win), srcs(posix)
    common = [s for s in ws if s in ps] if posix else []
    wd, pd = clean_defines(win, target), clean_defines(posix, target) if posix else []
    cdef = [d for d in wd if d in pd] + cfg.get('defines', [])
    wl = map_libs(win, cfg, known, problems, folder)
    pl = map_libs(posix, cfg, known, problems, folder) if posix else []
    cuse = [l for l in wl if l in pl] + [u for u in cfg.get('use', []) if u not in wl or u not in pl]
    for text, array, header in cfg.get('embed', []):
        embed(os.path.join(base, text), array, os.path.join(base, header), check)
    # Windows folders (and the table's extras) are shared; folders only the POSIX configuration names go to POSIX alone
    incs, pincs = [], []
    for p, out in [(win, incs)] + ([(posix, pincs)] if posix else []):
        for inc in ['.'] + [here(p, x) for x in p.include_dirs()] + cfg.get('includes', []):
            if inc not in incs and inc not in out and inc not in cfg.get('noincludes', []) and os.path.isdir(os.path.join(base, inc)):
                out.append(inc)
    kind = cfg['kind']
    text = TEMPLATE.format(
        vpc=folder + '/' + cfg['vpc'], target=target,
        guard="\tif bld.env.DEST_OS != 'win32':\n\t\treturn\n\n" if cfg.get('windows_only') else '',
        common=fmt(common), includes=fmt(incs), defines=fmt(cdef), use=fmt(cuse), posix_inc=fmt(pincs, '		'),
        win_src=fmt([s for s in ws if s not in common], '\t\t'),
        win_def=fmt([d for d in wd if d not in cdef] + cfg.get('win_defines', []), '\t\t'),
        win_use=fmt([l for l in wl if l not in cuse] +
                    ([] if kind == 'stlib' else WIN_SYS + cfg.get('win_use', [])), '\t\t'),
        posix_src=fmt([s for s in ps if s not in common], '\t\t'),
        posix_def=fmt([d for d in pd if d not in cdef], '\t\t'),
        posix_use=fmt([l for l in pl if l not in cuse] + [l for l in POSIX_SYS + cfg.get('posix_use', []) if l not in pl and l not in cuse], '\t\t'),
        call={'stlib': '.stlib', 'shlib': '.shlib', 'program': ''}[kind],
        features='c cxx cxxprogram' if kind == 'program' else 'c cxx',
        install='None' if kind == 'stlib' else ('bld.env.BINDIR' if kind == 'program' and not cfg.get('console') else 'bld.env.LIBDIR'),
        subsystem="'CONSOLE'" if cfg.get('console') else 'bld.env.MSVC_SUBSYSTEM',
        android=("\t\tif bld.env.DEST_OS == 'android':\n" +
                 ("\t\t\tsource = [s for s in source if s not in %s]\n" % fmt(cfg['android_drop'], '\t\t\t')
                  if cfg.get('android_drop') else '') +
                 ("\t\t\tsource += %s\n" % fmt(cfg['android_src'], '\t\t\t') if cfg.get('android_src') else ''))
                if cfg.get('android_src') or cfg.get('android_drop') else '')
    if not check:
        with open(os.path.join(base, 'wscript'), 'w', newline='\n') as f:
            f.write(text)
    print('%-34s %-18s win %3d  posix %3d  common %3d' % (folder, target, len(ws), len(ps), len(common)))


PROTOC = os.path.normpath(os.path.join(SRC, '..', '.port-build', 'win-x64', 'thirdparty', 'protobuf-2.5.0', 'protoc.exe'))
PROTO_DONE = set()


def protoc(base, proto, outdir, check, problems, folder):
    # Same search path as vpc_scripts/protobuf_builder.vpc. The .pb.cc/.pb.h pair is the same on every
    # platform, so it is generated once here with the protoc built from thirdparty/protobuf-2.5.0.
    src = os.path.normpath(os.path.join(base, proto))
    out = os.path.normpath(os.path.join(base, outdir))
    name = os.path.splitext(os.path.basename(src))[0]
    target = os.path.join(out, name + '.pb.cc')
    if (src, out) in PROTO_DONE or check:
        return
    PROTO_DONE.add((src, out))
    if os.path.exists(target) and os.path.getmtime(target) >= os.path.getmtime(src):
        return
    if not os.path.exists(PROTOC):
        problems.append('%s: protoc is not built, cannot generate %s.pb.cc (build target protoc first)' % (folder, name))
        return
    if not os.path.isdir(out):
        os.makedirs(out)
    paths = [os.path.join(SRC, 'thirdparty', 'protobuf-2.5.0', 'src'), os.path.dirname(src), os.path.join(SRC, 'gcsdk'),
             os.path.join(SRC, 'game', 'shared'), os.path.join(SRC, 'game', 'shared', 'cstrike15'), os.path.join(SRC, 'common')]
    cmd = [PROTOC] + ['--proto_path=' + p for p in paths] + ['--cpp_out=' + out, src]
    if subprocess.call(cmd) != 0:
        problems.append('%s: protoc failed on %s' % (folder, proto))


def embed(text, array, header, check):
    # devtools/bin/texttoarray.pl: a text file as a zero-terminated unsigned char array
    out = 'static unsigned char %s[] = {\n    ' % array
    for i, byte in enumerate(bytearray(open(text, 'rb').read())):
        out += '0x%02x,' % byte
        if i % 20 == 19:
            out += '\n    '
    out += '0x00\n};\n'
    if not check and (not os.path.exists(header) or open(header).read() != out):
        with open(header, 'w', newline='\n') as f:
            f.write(out)


def known_targets():
    # every Waf target name: the generated ones plus PROJECT_NAME of the hand-maintained wscripts
    names = set(cfg['target'] for cfg in PROJECTS.values())
    for root, dirs, files in os.walk(SRC):
        dirs[:] = [d for d in dirs if d not in ('build', '.git', 'thirdparty', 'lib')]
        if 'wscript' in files and root != SRC:
            text = open(os.path.join(root, 'wscript'), encoding='latin-1').read()
            m = (re.search(r'^PROJECT_NAME\s*=\s*[\'"]([^\'"]+)', text, re.M) or
                 re.search(r'\btarget\s*=\s*[\'"]([^\'"]+)', text))
            if m:
                names.add(m.group(1))
    return names


def main():
    args = sys.argv[1:]
    check = '--check' in args
    wanted = [a.replace('\\', '/').strip('/') for a in args if not a.startswith('--')] or sorted(PROJECTS)
    known = known_targets()
    problems = []
    for folder in wanted:
        generate(folder, PROJECTS[folder], known, check, problems)
    for p in sorted(set(problems)):
        print('  ! ' + p)


main()
