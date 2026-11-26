#!/usr/bin/env python
# Summarizes compiler and linker errors in a waf build log (MSVC, clang or gcc).
# usage: errsum.py <log> [max rows]
import collections
import re
import sys

log = sys.argv[1]
rows = int(sys.argv[2]) if len(sys.argv) > 2 else 40
lines = open(log, encoding='latin-1').read().splitlines()

msvc = re.compile(r'^(?P<file>[^()]+)\((?P<line>\d+)(?:,\d+)?\)\s*:\s*(?:fatal )?error (?P<code>\w+): (?P<msg>.*)')
gnu = re.compile(r'^(?P<file>[^:]+(?::[\\/][^:]+)?):(?P<line>\d+):(?:\d+:)? (?:fatal )?error: (?P<msg>.*)')
link = re.compile(r'(error LNK\d+|undefined (reference|symbol)|fatal error RC\d+)')
failed = re.compile(r"task in '([^']+)' failed")

files = collections.Counter()
msgs = collections.Counter()
first = {}
links = []
targets = collections.Counter()
for text in lines:
    m = msvc.match(text) or gnu.match(text)
    if m:
        name = re.split(r'[\\/]', m.group('file'))
        name = '/'.join(name[-2:])
        files[name] += 1
        code = m.groupdict().get('code') or ''
        key = (code + ': ' if code else '') + m.group('msg')[:120]
        msgs[key] += 1
        first.setdefault(key, name + ':' + m.group('line'))
        continue
    if link.search(text):
        links.append(text[:260])
    m = failed.search(text)
    if m:
        targets[m.group(1)] += 1

print('failed targets:', ', '.join('%s x%d' % kv for kv in targets.most_common()) or 'none')
print('compile errors: %d in %d files' % (sum(files.values()), len(files)))
for name, count in files.most_common(rows):
    print('%5d %s' % (count, name))
print()
for key, count in msgs.most_common(rows):
    print('%5d %s   @ %s' % (count, key, first[key]))
if links:
    print()
    print('link errors: %d' % len(links))
    seen = collections.Counter(re.sub(r'^.*?(error LNK\d+: |undefined (reference to|symbol:?) )', '', t)[:200] for t in links)
    for key, count in seen.most_common(rows):
        print('%5d %s' % (count, key))
