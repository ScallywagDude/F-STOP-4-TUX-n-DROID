#!/usr/bin/env python
# Rewrites direct animation layer member access (m_flCycle, m_nSequence...) to the accessors that
# the CS:GO partner engine's C_AnimationLayer / CAnimationLayer have (their members are private
# on the client). usage: layeraccess.py <file> ...
import re, sys
FIELDS = {'m_flCycle': 'Cycle', 'm_flPrevCycle': 'PrevCycle', 'm_flPlaybackRate': 'PlaybackRate',
          'm_nSequence': 'Sequence', 'm_flWeight': 'Weight', 'm_nOrder': 'Order'}
OBJ = r'((?:\w+(?:\[[^\]\[]*\])?\s*(?:\.|->)\s*)*\w+(?:\[[^\]\[]*\])?\s*->\s*)'
for path in sys.argv[1:]:
    t = open(path, encoding='latin-1', newline='').read()
    n0 = t
    for f, acc in FIELDS.items():
        # compound assignment: x->f += v;
        t = re.sub(OBJ + f + r'\s*([+\-*/])=\s*([^;]+);',
                   lambda m: '%sSet%s( %sGet%s() %s ( %s ) );' % (m.group(1), acc, m.group(1), acc, m.group(2), m.group(3).strip()), t)
        # plain assignment (not ==)
        t = re.sub(OBJ + f + r'\s*=(?!=)\s*([^;]+);',
                   lambda m: '%sSet%s( %s );' % (m.group(1), acc, m.group(2).strip()), t)
        # reads
        t = re.sub(OBJ + f + r'\b', lambda m: '%sGet%s()' % (m.group(1), acc), t)
    if t != n0:
        open(path, 'w', encoding='latin-1', newline='').write(t)
        print('rewrote', path)
