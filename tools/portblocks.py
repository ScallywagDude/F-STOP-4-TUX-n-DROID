#!/usr/bin/env python
# Carries F-STOP's PORTAL2 preprocessor blocks from the original F-STOP base files
# (fsb/fstop_src/game/...) into the CS:GO partner base files, renamed to FSTOP_PORTAL2.
#
# For every top-level #if/#ifdef whose condition names PORTAL2, the block (through its #endif) is
# located in the CS:GO file by the nearest non-blank lines before it. If the CS:GO file has a
# PORTAL2 block right there, it is replaced; otherwise the block is inserted after the anchor.
#
#   FSTOP_BASE_GAME=<original F-STOP source>/game portblocks.py <rel path under game/> [...] [--apply]
import os, re, sys

FSB = os.environ.get('FSTOP_BASE_GAME') or sys.exit('set FSTOP_BASE_GAME to the original F-STOP game source folder')
CSGO = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src', 'game')

COND = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b(.*)$')
ENDIF = re.compile(r'^\s*#\s*endif\b')
P2 = re.compile(r'\bPORTAL2\b')


def blocks(lines):
    out = []
    i = 0
    while i < len(lines):
        m = COND.match(lines[i])
        if m and P2.search(m.group(2)) and 'FSTOP_PORTAL2' not in m.group(2):
            depth = 0
            j = i
            while j < len(lines):
                if COND.match(lines[j]):
                    depth += 1
                elif ENDIF.match(lines[j]):
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            out.append((i, j))
            i = j + 1
        else:
            i += 1
    return out


def norm(s):
    return ' '.join(s.split())


def anchor(lines, i, n=3):
    a = []
    k = i - 1
    while k >= 0 and len(a) < n:
        if lines[k].strip():
            a.insert(0, norm(lines[k]))
        k -= 1
    return a


def find_anchor(lines, a):
    idx = [k for k, l in enumerate(lines) if l.strip()]
    nl = [norm(lines[k]) for k in idx]
    for n in range(len(a), 0, -1):
        sub = a[-n:]
        hits = [idx[p + n - 1] for p in range(len(nl) - n + 1) if nl[p:p + n] == sub]
        if len(hits) == 1:
            return hits[0]
        if len(hits) > 1:
            return None
    return None


def rename(text):
    return P2.sub('FSTOP_PORTAL2', text)


def port(rel, apply):
    src = open(os.path.join(FSB, rel), encoding='latin-1', newline='').read().splitlines(True)
    dpath = os.path.join(CSGO, rel)
    dst = open(dpath, encoding='latin-1', newline='').read().splitlines(True)
    eol = '\r\n' if dst and dst[0].endswith('\r\n') else '\n'
    edits = []  # (start, end_exclusive, newlines)
    for (b, e) in blocks(src):
        a = anchor(src, b)
        body = [rename(l).rstrip('\r\n') + eol for l in src[b:e + 1]]
        k = find_anchor(dst, a)
        if k is None:
            # fall back to the lines after the block: insert before them
            after = anchor(src[::-1], len(src) - 1 - e)[::-1]
            r = find_anchor(dst[::-1], after[::-1]) if after else None
            if r is None:
                print('  %s:%d  NO ANCHOR  %s' % (rel, b + 1, a[-1:] if a else ''))
                continue
            k = len(dst) - 1 - r  # first line after
            p = k - 1
            while p >= 0 and not dst[p].strip():
                p -= 1
            if p >= 0 and ENDIF.match(dst[p]) and 'FSTOP_PORTAL2' in dst[p]:
                print('  %s:%d  already ported' % (rel, b + 1))
                continue
            de = [x for x in blocks(dst) if x[1] == p]
            if de:
                print('  %s:%d  replace CS:GO block %d-%d (anchored after)' % (rel, b + 1, de[0][0] + 1, p + 1))
                edits.append((de[0][0], p + 1, body))
                continue
            print('  %s:%d  insert before CS:GO line %d (%d lines)' % (rel, b + 1, k + 1, len(body)))
            edits.append((k, k, body))
            continue
        # skip blank lines after the anchor
        s = k + 1
        while s < len(dst) and not dst[s].strip():
            s += 1
        m = COND.match(dst[s]) if s < len(dst) else None
        if m and 'FSTOP_PORTAL2' in m.group(2):
            print('  %s:%d  already ported' % (rel, b + 1))
            continue
        if m and (P2.search(m.group(2))):
            de = [x for x in blocks(dst) if x[0] == s]
            if de:
                same = [norm(l) for l in dst[s:de[0][1] + 1]] == [norm(l) for l in src[b:e + 1]]
                print('  %s:%d  replace CS:GO block %d-%d%s' % (rel, b + 1, s + 1, de[0][1] + 1, ' (identical)' if same else ''))
                edits.append((s, de[0][1] + 1, body))
                continue
        print('  %s:%d  insert after CS:GO line %d (%d lines)' % (rel, b + 1, k + 1, len(body)))
        edits.append((k + 1, k + 1, body))
    if apply and edits:
        for (s, e, body) in sorted(edits, key=lambda x: -x[0]):
            dst[s:e] = body
        open(dpath, 'w', encoding='latin-1', newline='').write(''.join(dst))


if __name__ == '__main__':
    apply = '--apply' in sys.argv
    for rel in [a for a in sys.argv[1:] if not a.startswith('--')]:
        port(rel.replace('\\', '/'), apply)
