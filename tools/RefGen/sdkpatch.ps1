$f = "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt\wchar.h"
if (-not (Test-Path "$f.orig")) { Copy-Item $f "$f.orig" }
$s = [IO.File]::ReadAllText($f)
if ($s -notmatch 'RefGen v140 shim') {
  $anchor = '    __inline wchar_t _CONST_RETURN* __CRTDECL wmemchr('
  if (-not $s.Contains($anchor)) { throw "anchor missing" }
  $shim = "    // RefGen v140 shim: VS2015 lacks _mm_loadu_si64 (backup: wchar.h.orig)`r`n    #if defined(_MSC_VER) && _MSC_VER < 1910 && !defined(_mm_loadu_si64)`r`n    #define _mm_loadu_si64(p) _mm_loadl_epi64((__m128i const*)(p))`r`n    #endif`r`n`r`n"
  $s = $s.Replace($anchor, $shim + $anchor)
  [IO.File]::WriteAllText($f, $s)
}
Select-String $f -Pattern 'RefGen v140 shim' | % LineNumber
