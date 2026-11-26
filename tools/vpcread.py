#!/usr/bin/env python
# Reads a VPC project the way VPC does for one platform: follows $Include, expands macros, evaluates
# [$COND] expressions on statements and on the blocks they open, skips Debug configurations and files
# marked $ExcludedFromBuild. Gives the sources, libraries, preprocessor definitions and include folders.
#
# As a script:  vpcread.py <project.vpc> <platform> [--all|--libs|--defines|--includes] [-D NAME ...]
#   platform: win64 | win32 | linux64 | android64 | osx64
import os
import re
import sys

PLATFORMS = {
    'win64': {'WINDOWS', 'WIN64'},
    'win32': {'WINDOWS', 'WIN32'},
    # VPC itself turns on $GL (the Direct3D-to-OpenGL renderer) for every POSIX platform
    'linux64': {'POSIX', 'POSIX64', 'LINUX', 'LINUXALL', 'LINUX64', 'GL'},
    'android64': {'POSIX', 'POSIX64', 'LINUX', 'LINUXALL', 'LINUX64', 'ANDROID', 'GL'},
    'osx64': {'POSIX', 'POSIX64', 'OSXALL', 'OSX64', 'GL'},
}
SOURCE_EXT = ('.cpp', '.c', '.cc', '.cxx', '.masm', '.rc', '.mm', '.proto')
FILE_KEYS = ('file', 'file_nopch', 'dynamicfile', 'dynamicfile_nopch', 'schemafile', 'qtfile')
LIB_KEYS = ('lib', 'implib', 'libexternal', 'implibexternal')


class Project(object):
    def __init__(self, vpc, platform, extra=(), off=()):
        self.vpc = os.path.abspath(vpc)
        self.base = os.path.dirname(self.vpc)
        self.platform = platform
        self.defs = set(PLATFORMS[platform]) | set(d.upper() for d in extra)
        self.off = set(d.upper() for d in off)     # conditionals that stay off whatever the scripts set
        win = platform.startswith('win')
        self.macros = {
            'PLATFORM': platform, 'PLATSUBDIR': '/' + platform,
            '_DLL_EXT': '.dll' if win else '.so', '_STATICLIB_EXT': '.lib' if win else '.a',
            '_IMPLIB_EXT': '.lib' if win else '.so', '_EXE_EXT': '.exe' if win else '',
            'QUOTE': '"',
        }
        self.files = []
        self.libs = []
        self.defines = []
        self.includes = []
        self.seen = set()
        self._parse(self.vpc)

    # ---- conditions -------------------------------------------------------------------------
    def _eval(self, expr):
        tokens = re.findall(r'\$\w+|\|\||&&|!|\(|\)', expr)
        pos = [0]

        def atom():
            tok = tokens[pos[0]] if pos[0] < len(tokens) else ''
            pos[0] += 1
            if tok == '!':
                return not atom()
            if tok == '(':
                val = either()
                pos[0] += 1
                return val
            name = tok[1:].upper()
            if name in self.defs:
                return True
            return self.macros.get(name, '0') not in ('0', '')

        def both():
            val = atom()
            while pos[0] < len(tokens) and tokens[pos[0]] == '&&':
                pos[0] += 1
                val = atom() and val
            return val

        def either():
            val = both()
            while pos[0] < len(tokens) and tokens[pos[0]] == '||':
                pos[0] += 1
                val = both() or val
            return val

        return either() if tokens else True

    def _expand(self, text):
        for _ in range(8):
            new = re.sub(r'\$(\w+)', lambda m: self.macros.get(m.group(1).upper(), m.group(0)), text)
            if new == text:
                break
            text = new
        return text

    # ---- parsing ----------------------------------------------------------------------------
    @staticmethod
    def _lines(path):
        pending = ''
        text = open(path, encoding='latin-1').read()
        # block comments can span statements; keep the line count out of it, nothing here needs it
        text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
        for raw in text.split('\n'):
            line = re.sub(r'(?<!:)//.*', '', raw).strip()
            if line.endswith('\\'):
                pending += line[:-1] + ' '
                continue
            line = (pending + line).strip()
            pending = ''
            while line:
                # braces normally sit on their own line, but split them off when they do not
                m = re.match(r'^([{}])\s*(.*)$', line)
                if m:
                    yield m.group(1)
                    line = m.group(2)
                    continue
                m = re.match(r'^(.*?[^\s{}])\s*([{}])\s*$', line)
                if m and line.count('"') % 2 == 0:
                    yield m.group(1)
                    yield m.group(2)
                else:
                    yield line
                break

    def _parse(self, path):
        path = os.path.normpath(path)
        if path.lower() in self.seen or not os.path.exists(path):
            return
        self.seen.add(path.lower())
        stack = [(True, None)]          # (active, files this block belongs to)
        pending = (True, None)
        for line in self._lines(path):
            if line == '{':
                stack.append((stack[-1][0] and pending[0], pending[1] or stack[-1][1]))
                pending = (True, None)
                continue
            if line == '}':
                if len(stack) > 1:
                    stack.pop()
                pending = (True, None)
                continue
            active, owner = stack[-1]
            cond = re.search(r'\[([^\]]*)\]\s*$', line)
            ok = active and (self._eval(cond.group(1)) if cond else True)
            body = line[:cond.start()].strip() if cond else line
            m = re.match(r'(-?)\$(\w+)\s*(.*)', body)
            pending = (ok, None)
            if not m:
                continue
            minus, key, rest = m.group(1), m.group(2).lower(), m.group(3)
            args = re.findall(r'"([^"]*)"', rest) or rest.split()
            if key == 'configuration':
                if args and args[0].lower() == 'debug':
                    pending = (False, None)
                continue
            multi = (key in FILE_KEYS or key in LIB_KEYS) and len(re.findall(r'"[^"]*"', rest)) > 1
            if not ok and not (multi and active):
                continue
            if key == 'macro' and len(args) >= 1:
                name = rest.split()[0].strip('"').upper()
                self.macros[name] = self._expand(args[-1] if '"' in rest else (rest.split() + [''])[1])
            elif key == 'macrorequired' or key == 'macrorequiredallowempty':
                name = rest.split()[0].strip('"').upper()
                if len(args) > 1 and name not in self.macros:
                    self.macros[name] = self._expand(args[-1])
            elif key == 'conditional' and rest.split():
                name = rest.split()[0].strip('"').upper()
                value = (args[-1] if '"' in rest else (rest.split() + ['1'])[1])
                (self.defs.add if value not in ('0', '') and name not in self.off else self.defs.discard)(name)
            elif key == 'include' and args:
                self._parse(os.path.join(self.base, self._expand(args[0])))
            elif key in FILE_KEYS or key in LIB_KEYS:
                owned = []          # one statement can name several files that share the block after it
                # A statement continued over several lines gives each file its own [condition]; the one at the end
                # of the line belongs to the last file only, not to the whole statement.
                pairs = re.findall(r'"([^"]*)"\s*(?:\[([^\]]*)\])?', m.group(3) + (' [' + cond.group(1) + ']' if cond else ''))
                if len(pairs) > 1:
                    args = [n for n, c in pairs if not c or self._eval(c)]
                for name in args:
                    name = self._expand(name).replace('\\', '/')
                    if key in LIB_KEYS or name.lower().endswith(('.lib', '.a')):
                        self.libs.append(('-' if minus else '') + name)
                        continue
                    rel = os.path.normpath(os.path.join(self.base, name)).replace('\\', '/')
                    if minus:
                        self.files = [f for f in self.files if f.lower() != rel.lower()]
                    elif rel.lower() not in (f.lower() for f in self.files):
                        self.files.append(rel)
                    owned.append(rel.lower())
                    pending = (ok, owned)
            elif key == 'excludedfrombuild' and owner and args and args[0].lower().startswith('y'):
                self.files = [f for f in self.files if f.lower() not in owner]
            elif key == 'preprocessordefinitions' and not owner and args:
                self._merge(self.defines, args[0], ';,')
            elif key == 'additionalincludedirectories' and not owner and args:
                self._merge(self.includes, args[0], ';,')
            elif key == 'additionaldependencies' and not owner and args:
                # libraries given straight to the linker ("$BASE a.lib b.lib")
                for dep in self._expand(args[0]).split():
                    dep = dep.replace('\\', '/')
                    if dep.upper() != '$BASE' and dep not in self.libs:
                        self.libs.append(dep)

    def _merge(self, target, value, seps):
        parts = [p.strip() for p in re.split('[' + seps + ']', value) if p.strip()]
        if '$BASE' not in [p.upper() for p in parts]:
            del target[:]
        for part in parts:
            if part.upper() == '$BASE':
                continue
            part = self._expand(part)
            if part not in target:
                target.append(part)

    # ---- results ----------------------------------------------------------------------------
    def rel(self, path):
        return os.path.relpath(path, self.base).replace('\\', '/')

    def sources(self, everything=False):
        return [self.rel(f) for f in self.files if everything or f.lower().endswith(SOURCE_EXT)]

    def include_dirs(self):
        out = []
        for inc in self.includes:
            if '$' in inc:
                continue
            inc = inc.replace('\\', '/').strip('"')
            path = inc if os.path.isabs(inc) else os.path.normpath(os.path.join(self.base, inc))
            r = self.rel(path) if not os.path.isabs(inc) else inc
            if r not in out:
                out.append(r)
        return out


def main():
    args = sys.argv[1:]
    extra = []
    while '-D' in args:
        i = args.index('-D')
        extra.append(args[i + 1])
        del args[i:i + 2]
    flags = [a for a in args if a.startswith('--')]
    args = [a for a in args if not a.startswith('--')]
    project = Project(args[0], args[1], extra)
    if '--libs' in flags:
        print('\n'.join(project.libs))
    elif '--defines' in flags:
        print('\n'.join(project.defines))
    elif '--includes' in flags:
        print('\n'.join(project.include_dirs()))
    else:
        print('\n'.join(project.sources('--all' in flags)))


if __name__ == '__main__':
    main()
