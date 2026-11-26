# Builds the APK asset root for Android from a staged Windows folder (tools/stage_win.sh / stage_desktop.ps1):
#   <out>/fstop_data/fstop/..., <out>/fstop_data/platform/...  hard links to the staged files (no copies)
#   <out>/fstop_manifest.txt  "<size>\t<path>" per file, read by launcher MainActivity to unpack on first run
# Leaves out the Windows binaries and everything a run of the game or a test leaves behind, the same set
# deck/package.sh leaves out of the Linux package.
# usage: python stage_android_assets.py [source] [out]
import os
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
SOURCE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, '.port-build', 'win-run')
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'out', 'android-assets')

SKIP_DIRS = {'fstop/bin', 'fstop/SAVE', 'fstop/screenshots', 'fstop/testscripts'}
SKIP_NAMES = {'modelsounds.cache', 'fstop_gamestats.dat', 'voice_ban.dt', 'stats.txt', 'glshaders.cfg', 'README.md',
              'old_portalgun_demo.bsp'}
SKIP_EXT = ('.log', '.tmp', '.mdmp', '.dmp', '.pdb')
SKIP_FILES = {'fstop/cfg/config.cfg', 'fstop/cfg/video.txt', 'fstop/cfg/menu_lifecycle.cfg'}


def wanted(rel):
    name = rel.rsplit('/', 1)[-1]
    return not (name in SKIP_NAMES or name.lower().endswith(SKIP_EXT) or rel in SKIP_FILES)


def main():
    if not os.path.isfile(os.path.join(SOURCE, 'fstop', 'gameinfo.txt')):
        sys.exit('no staged game in %s' % SOURCE)
    data = os.path.join(OUT, 'fstop_data')
    if os.path.exists(data):
        sys.exit('%s already exists; remove it or pick another output folder' % data)
    entries = []
    for top in ('fstop', 'platform'):
        for dirpath, dirnames, filenames in os.walk(os.path.join(SOURCE, top)):
            reldir = os.path.relpath(dirpath, SOURCE).replace(os.sep, '/')
            dirnames[:] = sorted(d for d in dirnames if '%s/%s' % (reldir, d) not in SKIP_DIRS)
            for name in sorted(filenames):
                rel = '%s/%s' % (reldir, name)
                if not wanted(rel):
                    continue
                src = os.path.join(dirpath, name)
                dst = os.path.join(data, rel)
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                os.link(src, dst)
                entries.append((os.path.getsize(src), rel))
    with open(os.path.join(OUT, 'fstop_manifest.txt'), 'w', newline='\n') as f:
        for size, rel in entries:
            f.write('%d\t%s\n' % (size, rel))
    print('%d files, %.2f GB -> %s' % (len(entries), sum(s for s, _ in entries) / 1e9, OUT))


if __name__ == '__main__':
    main()
