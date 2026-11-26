// Import the non-conflicting F-STOP sources onto the partner source tree.
// Shared engine/game files require an API-aware port and are never overwritten.
// Usage: node tools/import_fstop.mjs <F-STOP-checkout> [--apply]
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const destination = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const source = path.resolve(process.argv[2] || '../F-STOP-4-TUX-n-DROID');
const apply = process.argv.includes('--apply');
const git = (root, args) => execFileSync('git', args, { cwd: root, encoding: 'utf8', maxBuffer: 32 * 1024 * 1024 });
const digest = file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const files = git(source, ['ls-files', '--cached', '--others', '--exclude-standard', '-z']).split('\0').filter(Boolean);
const partnerFiles = new Set(git(destination, ['ls-tree', '-r', '--name-only', '-z', 'upstream/main', '--', 'src']).split('\0').filter(Boolean).map(p => p.toLowerCase()));
const reportPath = path.join(destination, '.port-build/source-import.json');
const previous = fs.existsSync(reportPath) ? JSON.parse(fs.readFileSync(reportPath, 'utf8')).imported : {};
const report = { donor: git(source, ['rev-parse', 'HEAD']).trim(), imported: {}, shared: [], skipped: [] };
const supportedTop = new Set(['deck', 'launcher', 'p2k', 'tools']);
const replacedByPartner = new Set([
    'engine/tier1/tokenreader.cpp', 'engine/tier1/ilocalize.cpp',
    'engine/public/tier1/tokenreader.h', 'engine/public/tier1/ilocalize.h',
    'engine/public/bitmap/float_bm.h'
]);
let changed = 0;

for (const relative of files) {
    // These implementations moved in the partner engine. Compatibility headers
    // forward to its definitions; importing the old class layout would violate ABI.
    if (replacedByPartner.has(relative)) continue;
    // Runtime source additions and deployment resources only. Build outputs,
    // game assets, caches, signing keys, and .git metadata are not enumerated.
    let target;
    if (relative.startsWith('engine/')) target = 'src/' + relative.slice(7);
    else if (supportedTop.has(relative.split('/')[0]) || relative === 'CREDITS.md') target = relative;
    else continue;
    if (partnerFiles.has(target.toLowerCase())) { report.shared.push({ source: relative, target }); continue; }
    const incoming = path.resolve(source, relative);
    const outgoing = path.resolve(destination, target);
    if (!incoming.startsWith(source + path.sep) || !outgoing.startsWith(destination + path.sep)) throw new Error('Path escapes checkout');
    if (fs.lstatSync(incoming).isSymbolicLink()) throw new Error(`Refusing symlink: ${relative}`);
    const hash = digest(incoming);
    if (fs.existsSync(outgoing)) {
        const existing = digest(outgoing);
        if (existing !== hash && previous[target] !== existing) {
            report.skipped.push(target);
            continue;
        }
    }
    report.imported[target] = hash;
    if (!fs.existsSync(outgoing) || digest(outgoing) !== hash) {
        changed++;
        if (apply) { fs.mkdirSync(path.dirname(outgoing), { recursive: true }); fs.copyFileSync(incoming, outgoing); }
    }
}
if (apply) {
    fs.mkdirSync(path.dirname(reportPath), { recursive: true });
    fs.writeFileSync(reportPath, JSON.stringify(report, null, 2) + '\n');
}
console.log(JSON.stringify({ apply, donor: report.donor, additions: Object.keys(report.imported).length, changed, shared: report.shared.length, protectedLocalEdits: report.skipped.length }, null, 2));
