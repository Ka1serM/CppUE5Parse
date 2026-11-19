# Bakes the generated RefGen maps with the engine's own builders, between generation and cooking:
#   - static lighting (Lightmass) for L_RefLit: MapBuildData (4.14+) or in-level lightmaps, shadowmaps,
#     volume lighting samples and precomputed visibility
#   - HLOD: World Partition HLOD actors for L_RefLitWP (5.0+; UE4 LOD actors are built during generation)
# usage: bake_win.py <UE_x.y> [--only lighting|wphlod|manifest] [--name <project folder>]
# Last, it writes the reference values for the parser tests: C:\RefGenWin\results\manifest\<UE_x.y>.json.
# Prints one line per step ("lighting: ok (123s)" / "lighting: FAILED ..."); logs go to <project>\Saved\bake-<step>.log.
import contextlib, json, os, re, shutil, stat, subprocess, sys, time

ver = sys.argv[1]
only = sys.argv[sys.argv.index('--only') + 1] if '--only' in sys.argv else None
engine = r'C:\Program Files\Epic Games\%s' % ver
major, minor = (int(x) for x in ver[3:].split('.'))
mm = major * 100 + minor
name = sys.argv[sys.argv.index('--name') + 1] if '--name' in sys.argv else \
    open(r'C:\RefGenWin\%s.dir' % ver).read().strip() if os.path.exists(r'C:\RefGenWin\%s.dir' % ver) else ver
project = os.path.join(r'C:\RefGenWin', name)
uproject = os.path.join(project, 'RefGen.uproject')
editor = next(p for p in (os.path.join(engine, r'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'),
                          os.path.join(engine, r'Engine\Binaries\Win64\UE4Editor-Cmd.exe')) if os.path.exists(p))
common = ['-unattended', '-nopause', '-nosplash', '-NoSound', '-stdout', '-FullStdOutLogOutput']
# the VM has no GPU: WARP, the Windows software rasterizer, which only the D3D12 RHI accepts ("Failed to choose a D3D11
# Adapter", UE4 and 5). The early UE4 D3D12 RHI may lose the device (4.11: DXGI_ERROR_DEVICE_REMOVED, then it hangs).
render = ['-AllowCommandletRendering', '-d3d12', '-warp', '-AllowSoftwareRendering']


@contextlib.contextmanager
def engine_read_only():
    # ResavePackages may resave engine maps (4.8 resaves Entry.umap despite -ProjectOnly): make the engine's packages
    # read-only while the editor runs, so it can't. The list survives a killed run and is undone on the next one.
    record = r'C:\RefGenWin\readonly-%s.txt' % ver

    def undo():
        if os.path.exists(record):
            for path in open(record, encoding='utf-8').read().splitlines():
                if os.path.exists(path):
                    os.chmod(path, stat.S_IREAD | stat.S_IWRITE)
            os.remove(record)

    undo()
    changed = []
    for top in ('Content', 'Plugins'):
        for folder, _, files in os.walk(os.path.join(engine, 'Engine', top)):
            for f in files:
                if f.endswith(('.uasset', '.umap')):
                    path = os.path.join(folder, f)
                    if os.stat(path).st_mode & stat.S_IWRITE:
                        changed.append(path)
    with open(record, 'w', encoding='utf-8') as out:
        out.write('\n'.join(changed))
    for path in changed:
        os.chmod(path, stat.S_IREAD)
    try:
        yield
    finally:
        undo()


def run(step, args, timeout=3600, extra=()):
    log = os.path.join(project, 'Saved', 'bake-%s.log' % step)
    os.makedirs(os.path.dirname(log), exist_ok=True)
    t = time.time()
    with engine_read_only(), open(log, 'w', errors='replace') as out:
        try:
            code = subprocess.run([editor, uproject] + args + common + list(extra), stdout=out, stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            code = 'timeout'
    text = open(log, errors='replace').read()
    return code, text, int(time.time() - t)


def report(step, code, text, seconds, ok):
    if ok:
        print('%s: ok (%ds)' % (step, seconds), flush=True)
        return
    lines = [l.strip() for l in text.splitlines() if 'Waiting for the agent' not in l]  # Swarm's start-up wait is logged as an error
    err = next((l for l in lines if re.search(r'Assertion failed|Fatal error|Unhandled Exception|^Error: ', l)), None) or \
        next((l for l in lines if re.search(r'Error:', l)), 'exit %s' % code)
    print('%s: FAILED exit %s: %s (%ds)' % (step, code, re.sub(r'^\[[^]]*\]\[[^]]*\]', '', err)[:300], seconds), flush=True)


def outside_project(text):
    # packages a resave wrote outside the project (engine content): must never happen, see lighting()
    saved = re.findall(r'(?:Correctly saved|Resaving package):?\s*\[([^\]]+)\]', text)
    return [p for p in saved if 'RefGenWin' not in p.replace('/', '\\')]


def lighting():
    # ResavePackages builds the lighting of each map given and saves it. -buildlighting arrived in 4.11: older versions
    # only resave (4.3-4.7 also lack -ProjectOnly and resave the engine's maps), so they get no lighting bake.
    if mm < 411:
        print('lighting: skipped (ResavePackages before 4.11 cannot build lighting)', flush=True)
        return
    maps = os.path.join(project, 'Content', 'Maps')
    backup = os.path.join(project, 'Saved', 'maps-before-lighting')
    shutil.rmtree(backup, ignore_errors=True)
    shutil.copytree(maps, backup)
    started = time.time()
    code, text, s = run('lighting', ['-run=ResavePackages', '-buildlighting', '-MapsOnly', '-ProjectOnly', '-Quality=Preview',
                                     '-Map=L_RefLit', '-IgnoreChangelist', '-SkipSkinVerify'], timeout=2400 if mm >= 500 else 900, extra=render)
    # success: the map was resaved with new lightmaps (texture builds of HQ_Lightmap*), and from 4.14 on a _BuiltData
    # package was written by this run. Older engines don't log "Lightmass" itself.
    built = os.path.join(project, r'Content\Maps\L_RefLit_BuiltData.uasset')
    ok = code == 0 and not re.search(r'Lighting build failed', text)
    if not re.search(r'Correctly saved:?\s*\[[^\]]*L_RefLit\.umap', text):
        ok = False
        text += '\nError: L_RefLit was not resaved'
    elif not re.search(r'Lightmass|HQ_Lightmap', text):
        ok = False
        text += '\nError: no lightmaps were built'
    elif mm >= 414 and not (os.path.exists(built) and os.path.getmtime(built) >= started):
        ok = False
        text += '\nError: no L_RefLit_BuiltData.uasset'
    outside = outside_project(text)
    if outside:
        ok = False
        text += '\nError: MODIFIED ENGINE CONTENT: ' + ', '.join(outside[:5])
    if not ok:
        # an unbaked map rather than a half-baked one (4.14 resaves the map but not its new _BuiltData package)
        shutil.rmtree(maps)
        shutil.copytree(backup, maps)
        text += '\n(maps restored)'
    report('lighting', code, text, s, ok)


def wphlod():
    code, text, s = run('wphlod', ['-run=WorldPartitionBuilderCommandlet', '/Game/Maps/L_RefLitWP', '-Builder=WorldPartitionHLODsBuilder',
                                   '-SCCProvider=None'], extra=render)
    ok = code == 0 and not re.search(r'HLOD.*(failed|error)', text, re.I)
    report('wphlod', code, text, s, ok)


def manifest():
    # the reference values for the parser tests, from the baked content (RefGenManifest.cpp)
    out = r'C:\RefGenWin\results\manifest\%s.json' % (ver if name == ver else name)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    if os.path.exists(out):
        os.remove(out)
    code, text, s = run('manifest', ['-run=RefGen', '-Manifest=' + out])
    ok = code == 0 and os.path.exists(out)
    if ok and mm >= 420:
        # the other value kinds (materials, meshes, textures, sounds, animations, curves, tables, blueprints) from the
        # editor's Python API (refgen_expected.py, 4.20+); level.actor stays the C++ one, which leaves out what the
        # cook drops
        values = out[:-len('.json')] + '-values.json'
        if os.path.exists(values):
            os.remove(values)
        os.environ['REFGEN_EXPECTED_OUT'] = values
        # the Python plugins, enabled in the .uproject for this run only (-EnablePlugins is newer than 4.2x)
        original = open(uproject, encoding='utf-8-sig').read()
        descriptor = json.loads(original)
        plugins = descriptor.setdefault('Plugins', [])
        for plugin in ('PythonScriptPlugin', 'EditorScriptingUtilities'):
            if not any(p.get('Name') == plugin for p in plugins):
                plugins.append({'Name': plugin, 'Enabled': True})
        with open(uproject, 'w', encoding='utf-8') as f:
            json.dump(descriptor, f, indent='\t')
        try:
            code2, text2, s2 = run('values', ['-run=pythonscript', '-script=' + os.path.join(os.path.dirname(os.path.abspath(__file__)), 'refgen_expected.py')])
        finally:
            with open(uproject, 'w', encoding='utf-8') as f:
                f.write(original)
        s += s2
        if os.path.exists(values):
            merged = json.load(open(out, encoding='utf-8'))
            extra = json.load(open(values, encoding='utf-8'))
            for kind, entries in extra['values'].items():
                if kind != 'level.actor':
                    merged['values'][kind] = entries
            merged['notes'] = extra.get('notes', [])
            with open(out, 'w', encoding='utf-8') as f:
                json.dump(merged, f, indent=1, sort_keys=True)
        else:
            ok = False
            code, text = code2, text2 + '\nError: refgen_expected.py wrote no values'
    report('manifest', code, text, s, ok)


steps = [('lighting', lighting)]  # UE4 HLODs are built by the generator itself (RefGenMaps.cpp)
if mm >= 500:
    steps.append(('wphlod', wphlod))
steps.append(('manifest', manifest))
for step, fn in steps:
    if only in (None, step):
        fn()
