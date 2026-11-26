#!/usr/bin/env python
# Wraps line ranges of a source file in a preprocessor guard (default: CS:GO-only code -> #ifdef CSTRIKE15),
# keeping the file's encoding and line endings. Ranges are 1-based and inclusive, as numbered before the edit.
# usage: guard.py <file> <start-end>[:else text] ... [--macro NAME]
#   e.g. guard.py game/client/c_baseviewmodel.cpp 25-29 185-231 "302-306"
#   An ":else text" part adds "#else" and that text (\n for new lines) before the #endif.
import re
import sys

args = sys.argv[1:]
macro = 'CSTRIKE15'
if '--macro' in args:
    i = args.index('--macro')
    macro = args[i + 1]
    del args[i:i + 2]
path, specs = args[0], args[1:]
raw = open(path, 'rb').read()
nl = b'\r\n' if b'\r\n' in raw else b'\n'
lines = raw.split(nl)
ranges = []
for spec in specs:
    rng, _, alt = spec.partition(':')
    a, b = (int(x) for x in rng.split('-'))
    ranges.append((a, b, alt))
for a, b, alt in sorted(ranges, reverse=True):
    tail = [b'#endif // ' + macro.encode()]
    if alt:
        tail = [b'#else'] + [t.replace('\\t', '\t').encode() for t in alt.split('\\n')] + tail
    lines[b:b] = tail
    lines[a - 1:a - 1] = [b'#ifdef ' + macro.encode()]
open(path, 'wb').write(nl.join(lines))
print('%s: %d ranges guarded with %s' % (path, len(ranges), macro))
