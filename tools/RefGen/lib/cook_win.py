# Cooks a generated RefGen project for Windows and packages it like make_fixtures.sh does for Linux:
# a versioned cook, then one encrypted + compressed .pak (4.x) or .pak + IoStore .utoc/.ucas with a global container (5.x).
# usage: cook_win.py <UE_x.y> [--rhi d3d|vulkan] [--out C:\RefGenWin\paks]
#   d3d     PCD3D_SM5 (+ PCD3D_SM6 on 5.x): DXBC and DXIL shader code
#   vulkan  SF_VULKAN_SM5: SPIR-V shader code, cooked for Windows
# Output: <out>\Windows\<x.y.z>.* or <out>\Windows-Vulkan\<x.y.z>.*
import argparse, base64, glob, json, os, re, shlex, shutil, subprocess, sys, time

KEY = 'RefGenTestKey0123456789abcdefghi'
ap = argparse.ArgumentParser()
ap.add_argument('ver')
ap.add_argument('--rhi', default='d3d', choices=['d3d', 'vulkan'])
ap.add_argument('--out', default=r'C:\RefGenWin\paks')
ap.add_argument('--pak-only', action='store_true', help='reuse the last cook (its log is kept in Saved)')
a = ap.parse_args()

E = r'C:\Program Files\Epic Games' + '\\' + a.ver
name_file = r'C:\RefGenWin\%s.dir' % a.ver
P = r'C:\RefGenWin' + '\\' + (open(name_file).read().strip() if os.path.exists(name_file) else a.ver)
bv = E + r'\Engine\Build\Build.version'
if not os.path.exists(bv):
    bv = r'C:\RefGenWin\%s.Build.version' % a.ver
v = json.load(open(bv, encoding='utf-8-sig'))
MAJOR, MINOR = v['MajorVersion'], v['MinorVersion']
VER = '%d.%d.%d' % (MAJOR, MINOR, v['PatchVersion'])
MM = MAJOR * 100 + MINOR
OUT = os.path.join(a.out, 'Windows' if a.rhi == 'd3d' else 'Windows-Vulkan')
W = os.path.join(P, 'Saved', 'fixtures-work-' + a.rhi)
keep_log = os.path.join(P, 'Saved', 'cook-win-%s.log' % a.rhi)
shutil.rmtree(W, ignore_errors=True)
os.makedirs(W)
os.makedirs(OUT, exist_ok=True)
UP = E + r'\Engine\Binaries\Win64\UnrealPak.exe'
UPROJ = os.path.join(P, 'RefGen.uproject')


def die(msg):
    print('%s %s: %s' % (VER, a.rhi, msg))
    sys.exit(1)


# ---------------------------------------------------------------- project config
def set_section(path, section, lines):
    text = open(path, encoding='utf-8').read() if os.path.exists(path) else ''
    text = re.sub(r'(?ms)^\[%s\]\n.*?(?=^\[|\Z)' % re.escape(section), '', text).rstrip() + '\n\n'
    text += '[%s]\n%s\n' % (section, '\n'.join(lines))
    open(path, 'w', encoding='utf-8').write(text)


engine_ini = os.path.join(P, 'Config', 'DefaultEngine.ini')
lines = ['!TargetedRHIs=ClearArray']
if MM >= 501:
    lines += ['!D3D12TargetedShaderFormats=ClearArray', '!D3D11TargetedShaderFormats=ClearArray', '!VulkanTargetedShaderFormats=ClearArray']
    if a.rhi == 'd3d':
        lines += ['+D3D12TargetedShaderFormats=PCD3D_SM6', '+D3D12TargetedShaderFormats=PCD3D_SM5', '+D3D11TargetedShaderFormats=PCD3D_SM5']
    else:
        lines += ['+VulkanTargetedShaderFormats=SF_VULKAN_SM5']
    lines += ['DefaultGraphicsRHI=%s' % ('DefaultGraphicsRHI_DX12' if a.rhi == 'd3d' else 'DefaultGraphicsRHI_Vulkan')]
elif a.rhi == 'd3d':
    lines += ['+TargetedRHIs=PCD3D_SM5'] + (['+TargetedRHIs=PCD3D_SM6'] if MAJOR >= 5 else [])
else:
    lines += ['+TargetedRHIs=SF_VULKAN_SM5']
set_section(engine_ini, '/Script/WindowsTargetPlatform.WindowsTargetSettings', lines)

game_ini = os.path.join(P, 'Config', 'DefaultGame.ini')
g = open(game_ini, encoding='utf-8').read()
# plugin content would be cooked too with bCookAll; /Game is cooked through DirectoriesToAlwaysCook (4.13+)
g = re.sub(r'(?m)^bCookAll=.*$', 'bCookAll=%s' % ('True' if MM < 413 else 'False'), g)
open(game_ini, 'w', encoding='utf-8').write(g)

# ---------------------------------------------------------------- receipt: stage without a game executable
d = os.path.join(P, 'Binaries', 'Win64')
editor_receipt = os.path.join(d, 'RefGenEditor.target')
if not os.path.exists(editor_receipt):
    old = sorted(glob.glob(os.path.join(d, 'RefGenEditor-Win64-*.target')))
    if old:
        shutil.copy(old[-1], editor_receipt)
if os.path.exists(editor_receipt):
    r = json.load(open(editor_receipt, encoding='utf-8-sig'))
    receipt = {k: r[k] for k in ['Platform', 'Configuration', 'BuildId', 'BuildSettingsVersion', 'Architecture', 'Project', 'Version'] if k in r}
    receipt.update(TargetName='RefGen', TargetType='Game', TargetBuildEnvironment='Unique', IsTestTarget=False,
                   Launch='$(ProjectDir)/Binaries/Win64/RefGen.exe', LaunchCmd='$(ProjectDir)/Binaries/Win64/RefGen.exe',
                   BuildProducts=[], RuntimeDependencies=[], BuildPlugins=r.get('BuildPlugins', []), AdditionalProperties=[])
    json.dump(receipt, open(os.path.join(d, 'RefGen.target'), 'w'), indent=1)
    for o in glob.glob(os.path.join(d, 'RefGenEditor-Win64-*.target')):
        json.dump(receipt, open(os.path.join(d, os.path.basename(o).replace('RefGenEditor-', 'RefGen-', 1)), 'w'), indent=1)
if MM < 413 and not os.path.exists(os.path.join(d, 'RefGen.exe')):
    shutil.copy(os.path.join(os.environ['WINDIR'], 'System32', 'where.exe'), os.path.join(d, 'RefGen.exe'))  # placeholder

# ---------------------------------------------------------------- cook
cooked_root = os.path.join(P, 'Saved', 'Cooked')
if not a.pak_only:
    shutil.rmtree(cooked_root, ignore_errors=True)
ddc = os.path.join(P, 'DerivedDataCache')
cooker = '-LocalDataCachePath=%s -ZenDataPath=%s' % (ddc, os.path.join(ddc, 'Zen'))
if MAJOR >= 5:
    cooker += ' -SkipZenStore'
cook = ['-cook'] + (['-CookAll'] if MM < 413 else [])
# launcher 4.x builds ship precompiled UAT scripts; compiling them needs NuGet tasks the VS2017 build tools lack
if MAJOR < 5 and glob.glob(os.path.join(E, 'Engine', 'Binaries', 'DotNET', 'AutomationScripts', '*.Automation.dll')):
    cook = ['-nocompile'] + cook
iostore = MAJOR >= 5
uat = [E + r'\Engine\Build\BatchFiles\RunUAT.bat', 'BuildCookRun', '-AdditionalCookerOptions=%s' % cooker, '-project=' + UPROJ,
       '-platform=Win64', '-clientconfig=Development', '-noP4', '-nocompileeditor', '-skipbuild'] + cook + \
      ['-stage', '-VersionCookedContent', '-skipcookingeditorcontent', '-stagingdirectory=' + os.path.join(W, 'stage'), '-unattended', '-utf8output'] + \
      (['-pak', '-iostore'] if iostore else [])
cook_log = keep_log
t = time.time()
rc = 0
if MAJOR < 5:
    # 4.x UAT drops -CookAll / AdditionalCookerOptions (old) or skips unreferenced assets: run the cook commandlet directly
    # (the pak is made below, so no staging is needed)
    uat = [E + r'\Engine\Binaries\Win64\UE4Editor-Cmd.exe', UPROJ, '-run=Cook', '-TargetPlatform=WindowsNoEditor'] + (['-CookAll'] if MM < 413 else []) + [
           '-COOKDIR=' + os.path.join(P, 'Content'), '-LocalDataCachePath=' + ddc, '-unattended', '-stdout', '-FORCELOGFLUSH', '-CrashForUAT']
if not a.pak_only:
    with open(cook_log, 'w', encoding='utf-8', errors='replace') as f:
        rc = subprocess.call(subprocess.list2cmdline(uat), stdout=f, stderr=subprocess.STDOUT, shell=True)
    print('%s %s: cook exit %d in %ds' % (VER, a.rhi, rc, time.time() - t))
if rc != 0:
    for line in open(cook_log, encoding='utf-8', errors='replace'):
        if re.search(r'Error:|error:|Fatal|Assertion failed', line):
            print('  ' + line.rstrip()[:300])
    die('cook failed, see ' + cook_log)

C = next(iter(glob.glob(os.path.join(cooked_root, '*', 'RefGen'))), None)
if not C:
    die('nothing cooked in ' + cooked_root)
fmt = 'SF_VULKAN' if a.rhi == 'vulkan' else 'PCD3D'
files = []
for root, _, names in os.walk(C):
    for n in names:
        rel = os.path.relpath(os.path.join(root, n), C).replace('\\', '/')
        if not (rel.startswith('Content/') or rel == 'AssetRegistry.bin'):
            continue
        if n.endswith('.stinfo') or n.endswith('.assetinfo.json') or n.startswith('ShaderArchive-Global-'):
            continue
        if n.startswith('ShaderArchive-') and fmt not in n:
            continue
        if rel.startswith('Content/ShaderCode-') and not rel.startswith('Content/ShaderCode-' + fmt):
            continue
        files.append(rel)
files.sort()
if not files:
    die('nothing to package in ' + C)
if not any('ShaderArchive-' in f or '/ShaderCode-' in f for f in files):
    if MM >= 425:  # shared shader code is on by default there; older cooks keep the shaders inline in the assets
        die('no %s shader library in the cook' % fmt)
    print('%s %s: no shared shader library (shaders inline in the assets)' % (VER, a.rhi))

# ---------------------------------------------------------------- pak / IoStore
codec = ['-compressionformats=Oodle'] if MAJOR >= 5 else (['-compressionformats=Zlib'] if MM >= 422 else [])
crypt, encrypt = [], ''
if MAJOR >= 5 or MM >= 420:
    crypto = os.path.join(W, 'Crypto.json')
    json.dump({'EncryptionKey': {'Name': 'RefGen', 'Guid': '00000000-0000-0000-0000-000000000000', 'Key': base64.b64encode(KEY.encode()).decode()},
               'SecondaryEncryptionKeys': [], 'bEnablePakIndexEncryption': True, 'bEnablePakIniEncryption': True,
               'bEnablePakUAssetEncryption': True, 'bEnablePakFullAssetEncryption': True, 'bDataCryptoRequired': True}, open(crypto, 'w'))
    crypt, encrypt = ['-cryptokeys=' + crypto, '-encryptindex'], '-encrypt'
elif MM >= 416:
    crypt, encrypt = ['-aes=' + KEY, '-encryptindex'], '-encrypt'


def response(path, rels):
    with open(path, 'w', encoding='utf-8') as f:
        for rel in rels:
            f.write('"%s" "../../../RefGen/%s" -compress %s\n' % (os.path.join(C, rel.replace('/', '\\')), rel, encrypt))


def run(args, log):
    with open(log, 'w', encoding='utf-8', errors='replace') as f:
        return subprocess.call(subprocess.list2cmdline(args), stdout=f, stderr=subprocess.STDOUT, shell=True)


response(os.path.join(W, 'pak.txt'), files)
for old in glob.glob(os.path.join(OUT, VER + '.*')) + glob.glob(os.path.join(OUT, VER + '-global.*')):
    os.remove(old)
if run([UP, os.path.join(OUT, VER + '.pak'), '-create=' + os.path.join(W, 'pak.txt'), '-compress'] + codec + crypt, os.path.join(W, 'pak.log')):
    die('UnrealPak failed, see ' + os.path.join(W, 'pak.log'))

if iostore:
    io = os.path.join(W, 'io')
    os.makedirs(io)
    # UnrealPak takes a .uexp with its .uasset; 5.0 asserts on a listed .uexp
    response(os.path.join(W, 'io.txt'), [f for f in files if re.search(r'\.(uasset|umap|ubulk|uptnl|ushaderbytecode)$', f)])
    with open(os.path.join(W, 'iocmds.txt'), 'w') as f:
        f.write('-Output="%s" -ContainerName=RefGen -ResponseFile="%s" %s\n' % (os.path.join(io, 'RefGen.utoc'), os.path.join(W, 'io.txt'), encrypt))
    line = next((l for l in open(cook_log, encoding='utf-8', errors='replace') if 'Running:' in l and 'UnrealPak' in l and '-CreateGlobalContainer=' in l), None)
    if not line:
        die('no IoStore UnrealPak command in the cook log')
    args = [x.replace('"', '') for x in shlex.split(line.split('Running: ', 1)[1].strip(), posix=False)]
    keep = []
    for x in args[1:]:
        k = x.split('=', 1)[0].lower()
        if k in ('-compressionformats', '-compresslevel', '-compressmethod', '-cryptokeys', '-encryptindex'):
            continue
        if k == '-createglobalcontainer':
            x = '-CreateGlobalContainer=' + os.path.join(io, 'global.utoc')
        elif k == '-commands':
            x = '-Commands=' + os.path.join(W, 'iocmds.txt')
        keep.append(x)
    if run([UP] + keep + codec + crypt, os.path.join(W, 'io.log')):
        die('IoStore UnrealPak failed, see ' + os.path.join(W, 'io.log'))
    for src, dst in (('RefGen.utoc', VER + '.utoc'), ('RefGen.ucas', VER + '.ucas'), ('global.utoc', VER + '-global.utoc'), ('global.ucas', VER + '-global.ucas')):
        shutil.move(os.path.join(io, src), os.path.join(OUT, dst))

size = sum(os.path.getsize(p) for p in glob.glob(os.path.join(OUT, VER + '*')))
print('%s %s: %.1f MB (%d files) -> %s' % (VER, a.rhi, size / 1e6, len(files), OUT))
shutil.rmtree(W, ignore_errors=True)
