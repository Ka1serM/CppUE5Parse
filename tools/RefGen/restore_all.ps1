$p = Get-Process UnrealEditor*,UE4Editor* -ErrorAction SilentlyContinue
if ($p) { "editors running: " + ($p.Name -join ','); exit 1 }
foreach ($d in Get-ChildItem 'C:\Program Files\Epic Games' -Directory -Filter 'UE_*') {
  & C:\Python312\python.exe -I C:\RefGenWin\tool\lib\egs_restore.py $d.FullName auto 2>&1 | Select-String -NotMatch '^  restored ' | % { $_.Line }
  "exit $LASTEXITCODE"
}
