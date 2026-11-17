# build_refgen.ps1 <UE_x.y> : stage RefGen for that launcher engine and build RefGenEditor Win64 Development.
param([Parameter(Mandatory)][string]$Ver,[string]$Name=$Ver)
$ErrorActionPreference = 'Stop'
$eng = "C:\Program Files\Epic Games\$Ver"
$proj = "C:\RefGenWin\$Name"
$tool = 'C:\RefGenWin\tool'
$verh = "$eng\Engine\Source\Runtime\Launch\Resources\Version.h"
if (-not (Test-Path $verh)) { throw "$Ver not installed" }
$buildVersion = "$eng\Engine\Build\Build.version"
if (-not (Test-Path $buildVersion)) {
  # Build.version is 4.9+: derive one from Version.h
  $v = @{}; Select-String $verh -Pattern '#define ENGINE_(MAJOR|MINOR|PATCH)_VERSION\s+(\d+)' | % { $v[$_.Matches[0].Groups[1].Value] = [int]$_.Matches[0].Groups[2].Value }
  $buildVersion = "C:\RefGenWin\$Ver.Build.version"
  @{ MajorVersion = $v.MAJOR; MinorVersion = $v.MINOR; PatchVersion = $v.PATCH } | ConvertTo-Json | Set-Content $buildVersion
}
if (Test-Path $proj) { Remove-Item -Recurse -Force $proj -EA SilentlyContinue }
if (Test-Path $proj) { $Name = "$Name-$(Get-Date -Format yyyyMMddHHmmss)"; $proj = "C:\RefGenWin\$Name"; "stuck old folder, staging into $proj" }
Set-Content "C:\RefGenWin\$Ver.dir" $Name
Copy-Item -Recurse "$tool\RefGen" $proj
& C:\Python312\python.exe -I "$tool\lib\fix_uproject.py" "$proj\RefGen.uproject" $eng
& C:\Python312\python.exe -I "$tool\lib\ue4_compat.py" "$proj\Source" $buildVersion
# Per-engine toolchain: the old UBTs pick the newest MSVC they find (VS 2026's 14.51), which they don't support.
$bv = Get-Content $buildVersion | ConvertFrom-Json
$mm = 100 * $bv.MajorVersion + $bv.MinorVersion
$extra = ''
$env:REFGEN_MSVC = ''
if ($mm -le 419) {
  # VS2015 (v140) is their default; these UBTs don't find the VS2017 Build Tools. v140 has no __has_include.
  & C:\Python312\python.exe -I "$tool\lib\resolve_has_include.py" "$proj\Source" $eng
} elseif ($mm -le 421) {
  $env:REFGEN_MSVC = '14.16.27023'
} elseif ($mm -eq 502 -or $mm -eq 503) {
  $env:REFGEN_MSVC = '14.38.33130'  # 14.4x+ breaks 5.2/5.3's __has_feature checks
}
if ($mm -le 410) { $extra += ' -rocket' }  # 4.10 and older UBT only detect an installed engine with -rocket
"toolchain: msvc=$env:REFGEN_MSVC extra=$extra"
$log = "C:\RefGenWin\build-$Name.log"
$sw = [Diagnostics.Stopwatch]::StartNew()
& cmd.exe /c "`"$eng\Engine\Build\BatchFiles\Build.bat`" RefGenEditor Win64 Development `"-Project=$proj\RefGen.uproject`" -WaitMutex $extra > `"$log`" 2>&1"
$rc = $LASTEXITCODE
$dlls = @(Get-ChildItem "$proj\Binaries\Win64" -Filter *.dll -EA SilentlyContinue)
if ($rc -eq 0 -and $dlls.Count -lt 2) { $rc = 99 }  # the old Build.bat returns 0 even when the build fails
"$Ver build exit $rc in $([int]$sw.Elapsed.TotalSeconds)s"
if ($rc -ne 0) { Select-String -Path $log -Pattern 'error|Error' | Select -First 25 | % { $_.Line.Substring(0,[Math]::Min(300,$_.Line.Length)) } }
else { $dlls | Select Name,Length | ft -AutoSize | Out-String }
