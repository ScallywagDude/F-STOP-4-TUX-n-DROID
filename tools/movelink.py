#!/usr/bin/env python
# The partner engine's client LINK_ENTITY_TO_CLASS refers to the class's ClientClass global, which
# IMPLEMENT_CLIENTCLASS* declares, so it has to come after that block. Moves each client
# LINK_ENTITY_TO_CLASS that precedes its class's IMPLEMENT_CLIENTCLASS* to just after the block's
# END_RECV_TABLE(). usage: movelink.py <file> ...
import re, sys
for path in sys.argv[1:]:
    t = open(path, encoding='latin-1', newline='').read()
    eol = '\r\n' if '\r\n' in t else '\n'
    lines = t.split(eol)
    changed = False
    for i in range(len(lines)):
        m = re.match(r'\s*LINK_ENTITY_TO_CLASS\s*\(\s*\w+\s*,\s*(\w+)\s*\)\s*;?\s*$', lines[i])
        if not m:
            continue
        cls = m.group(1)
        impl = next((k for k in range(i + 1, len(lines)) if re.match(r'\s*IMPLEMENT_CLIENTCLASS\w*\s*\(\s*' + cls + r'\b', lines[k])), None)
        if impl is None:
            continue
        end = next((k for k in range(impl, len(lines)) if re.match(r'\s*END_RECV_TABLE\s*\(', lines[k])), None)
        if end is None:
            continue
        line = lines[i]
        lines[end] = lines[end] + eol + eol + line
        lines[i] = '// (LINK_ENTITY_TO_CLASS moved below the client class)'
        changed = True
    if changed:
        open(path, 'w', encoding='latin-1', newline='').write(eol.join(lines))
        print('moved in', path)
