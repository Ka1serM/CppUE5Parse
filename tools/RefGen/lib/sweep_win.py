# Builds, generates and cooks (Windows D3D) RefGen for each engine, then frees the build space.
# usage: sweep_win.py UE_4.3+UE_4.4+...|all [--no-cook] [--no-bake] [--keep]
# Between generating and cooking, bake_win.py builds lighting and HLODs and writes the test reference values.
# One line per engine goes to C:\RefGenWin\results\sweep.txt; the per-engine logs stay in C:\RefGenWin.
import os, re, shutil, subprocess, sys, time

root = r'C:\Program Files\Epic Games'
args = [x for x in sys.argv[1:] if not x.startswith('--')]
flags = [x for x in sys.argv[1:] if x.startswith('--')]
key = lambda n: tuple(int(x) for x in n[3:].split('.'))
vers = sorted((d for d in os.listdir(root) if re.match(r'UE_\d+\.\d+$', d)), key=key) if args[0] == 'all' else args[0].split('+')
summary = r'C:\RefGenWin\results\sweep.txt'
ps = ['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']


def note(line):
    print(line, flush=True)
    with open(summary, 'a') as f:
        f.write(time.strftime('%H:%M ') + line + '\n')


for ver in vers:
    t = time.time()
    out = subprocess.run(ps + [r'C:\RefGenWin\trio.ps1', ver], capture_output=True, text=True, errors='replace').stdout
    result = open(r'C:\RefGenWin\results\%s.txt' % ver, errors='replace').read() if os.path.exists(r'C:\RefGenWin\results\%s.txt' % ver) else out
    m = re.search(r'RefGen finished with (\d+) failures', result)
    assets = re.search(r'assets: (\d+)', result)
    if not m:
        err = next((l.strip() for l in result.splitlines() if re.search(r'error|Assertion|exit [1-9]', l)), 'no result')
        note('%s: FAILED build/gen: %s' % (ver, err[:250]))
        continue
    gen = '%s: gen %s failures, %s assets (%ds)' % (ver, m.group(1), assets.group(1) if assets else '?', time.time() - t)
    if '--no-cook' in flags or m.group(1) != '0':
        note(gen + ('' if m.group(1) == '0' else ' ' + '; '.join(sorted(set(re.findall(r'REFGEN FAIL: [^\r\n]+', result)))[:6])))
        continue
    t = time.time()
    if '--no-bake' not in flags:
        bake = subprocess.run([r'C:\Python312\python.exe', '-I', r'C:\RefGenWin\tool\lib\bake_win.py', ver],
                              capture_output=True, text=True, errors='replace').stdout.strip().splitlines()
        gen += ' | bake: ' + '; '.join(l.strip() for l in bake if re.match(r'\w+: ', l))[:500] + ' (%ds)' % (time.time() - t)
        t = time.time()
    cook = subprocess.run([r'C:\Python312\python.exe', '-I', r'C:\RefGenWin\tool\lib\cook_win.py', ver, '--rhi', 'd3d'],
                          capture_output=True, text=True, errors='replace').stdout.strip().splitlines()
    note(gen + ' | ' + ' / '.join(l.strip() for l in cook[-4:])[:600] + ' (%ds)' % (time.time() - t))
    if '--keep' not in flags:
        name = open(r'C:\RefGenWin\%s.dir' % ver).read().strip() if os.path.exists(r'C:\RefGenWin\%s.dir' % ver) else ver
        proj = os.path.join(r'C:\RefGenWin', name)
        for sub in ('Intermediate', 'DerivedDataCache', r'Saved\Cooked', r'Saved\StagedBuilds'):
            shutil.rmtree(os.path.join(proj, sub), ignore_errors=True)
