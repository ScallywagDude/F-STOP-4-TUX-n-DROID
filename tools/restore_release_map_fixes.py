"""Carry release lab_intro entity fixes into its VMF for partner-tool builds."""
import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[1]
source = root / '.port-build/mapbuild/src/scratch/lab_intro.vmf'
dest = root / 'mapsrc/lab_intro.vmf'
text = source.read_text(encoding='latin1')
for name in ('soundscape_box_training', 'soundscape_cleanser'):
    # These point entities have no nested solids. Match the entire entity so
    # key order cannot accidentally select an adjacent entity's start state.
    pattern = r'entity\s*\{[^{}]*"targetname"\s*"' + name + r'"[^{}]*(?:editor\s*\{[^{}]*\}\s*)?\}'
    match = re.search(pattern, text)
    if not match:
        raise ValueError('Missing soundscape: ' + name)
    entity, count = re.subn(r'("StartDisabled"\s*)"1"', r'\g<1>"0"', match.group())
    assert count == 1, name
    text = text[:match.start()] + entity + text[match.end():]

at = text.index('"targetname" "rv_intro_trigger"')
connections = text.index('connections', at)
brace = text.index('{', connections) + 1
text = text[:brace] + '\n\t\t"OnTrigger" "fstop_shoreline\x1bDisable\x1b\x1b0\x1b-1"\n\t\t"OnTrigger" "fstop_silence\x1bEnable\x1b\x1b0\x1b-1"' + text[brace:]
for ident, name, origin, soundscape, radius, disabled in (
    (9000001, 'fstop_shoreline', '9504 7520 10300', 'coast.general_shoreline', 6000, 0),
    (9000002, 'fstop_silence', '-8416 -3568 -2600', 'fstop.silence', 900, 1),
):
    text += f'\nentity\n{{\n"id" "{ident}"\n"classname" "env_soundscape"\n"targetname" "{name}"\n"origin" "{origin}"\n"soundscape" "{soundscape}"\n"radius" "{radius}"\n"StartDisabled" "{disabled}"\n}}\n'
dest.parent.mkdir(parents=True, exist_ok=True)
dest.write_bytes(text.replace('\r\n', '\n').encode('latin1'))
print('Prepared release shoreline and soundscape fixes:', dest)
