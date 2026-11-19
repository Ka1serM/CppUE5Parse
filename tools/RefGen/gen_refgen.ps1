# gen_refgen.ps1 <UE_x.y> : run the RefGen commandlet in the built project.
param([Parameter(Mandatory)][string]$Ver,[string]$Name=$Ver)
$eng = "C:\Program Files\Epic Games\$Ver"
if ($Name -eq $Ver -and (Test-Path "C:\RefGenWin\$Ver.dir")) { $Name = (Get-Content "C:\RefGenWin\$Ver.dir").Trim() }
$proj = "C:\RefGenWin\$Name"
$exe = @("$eng\Engine\Binaries\Win64\UnrealEditor-Cmd.exe", "$eng\Engine\Binaries\Win64\UE4Editor-Cmd.exe") | ? { Test-Path $_ } | Select -First 1
$log = "C:\RefGenWin\gen-$Name.log"
$sw = [Diagnostics.Stopwatch]::StartNew()
& cmd.exe /c "`"$exe`" `"$proj\RefGen.uproject`" -run=RefGen -Source=C:\RefGenWin\tool\SourceAssets -unattended -nopause -nosplash -NoSound -stdout -FullStdOutLogOutput > `"$log`" 2>&1"
"$Ver gen exit $LASTEXITCODE in $([int]$sw.Elapsed.TotalSeconds)s"
Select-String -Path $log -Pattern 'RefGen finished|REFGEN FAIL|Assertion failed|Unhandled Exception|Fatal error' | % { $_.Line -replace '^\[[^]]*\]\[[^]]*\]','' } | Sort -Unique | Select -First 20
"assets: " + (Get-ChildItem "$proj\Content" -Recurse -Include *.uasset,*.umap -EA SilentlyContinue).Count
