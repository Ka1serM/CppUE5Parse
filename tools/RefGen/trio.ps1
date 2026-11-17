param([string]$List)
$Vers = $List -split ","
foreach ($v in $Vers) {
  $o = & C:\RefGenWin\build_refgen.ps1 $v 2>&1 | Out-String
  if ($o -match 'build exit 0 ') { $o += & C:\RefGenWin\gen_refgen.ps1 $v 2>&1 | Out-String }
  Set-Content "C:\RefGenWin\results\$v.txt" $o
  "== $v"; $o -split "`n" | ? { $_ -match 'exit|finished|assets|error|Error' } | select -first 8
}
