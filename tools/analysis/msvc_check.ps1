# Runs the Microsoft compiler over patches/Accessibility without producing
# objects, to collect diagnostics the normal build (/W3) does not ask for.
#
#   pwsh -File tools/analysis/msvc_check.ps1 -Mode w4        # warning level 4
#   pwsh -File tools/analysis/msvc_check.ps1 -Mode analyze   # the /analyze checker
#
# Every distinct warning goes to build/analysis/msvc_<mode>.txt; the console
# gets the count per warning code. Exit code 0 always: this is a report.

param(
    [Parameter(Mandatory)] [ValidateSet('w4', 'analyze')] [string] $Mode,
    [string[]] $Files
)

$ErrorActionPreference = 'Stop'
$repo   = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$src    = Join-Path $repo 'patches\Accessibility'
$common = Join-Path $repo 'third_party\Kotor-Patch-Manager\Patches\Common'
$lib    = Join-Path $repo 'third_party\Kotor-Patch-Manager\lib'
$outDir = Join-Path $repo 'build\analysis'
New-Item -ItemType Directory -Force $outDir | Out-Null

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ build tools not found (vswhere).' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars32.bat'

# The compiler environment, captured once.
foreach ($line in (& cmd /c "`"$vcvars`" >nul 2>&1 && set")) {
    if ($line -match '^([^=]+)=(.*)$') { Set-Item "env:$($Matches[1])" $Matches[2] }
}
$env:VSLANG = '1033'

# Off in analyze mode, both by design: C6320 and C6322 object to
# __except (EXCEPTION_EXECUTE_HANDLER), which is how every read of engine
# memory is guarded.
$flags = if ($Mode -eq 'w4') { '/Zs', '/W4' }
         else { '/analyze:only', '/W3', '/wd6320', '/wd6322' }
$base  = @('/nologo', '/c', '/EHsc', '/std:c++17', '/MD',
           "/I$src", "/I$common", "/I$lib") + $flags

$sources = if ($Files) { $Files | ForEach-Object { Join-Path $src $_ } }
           else { Get-ChildItem $src -Filter *.cpp | ForEach-Object FullName }

# /analyze drops a .nativecodeanalysis.xml per source into the working
# directory; keep those out of the repo.
$scratch = Join-Path $outDir 'msvc_scratch'
New-Item -ItemType Directory -Force $scratch | Out-Null
$lines = $sources | ForEach-Object -ThrottleLimit ([Environment]::ProcessorCount) -Parallel {
    Set-Location $using:scratch
    & cl @using:base $_ 2>&1 | ForEach-Object { "$_" }
}

# A warning in a header repeats for every file that includes it.
$warnings = $lines | Where-Object { $_ -match ': warning C\d+' } |
    ForEach-Object { $_.Replace($repo + '\', '') } | Sort-Object -Unique
$errors = $lines | Where-Object { $_ -match ': (fatal )?error C\d+' } | Sort-Object -Unique

$report = Join-Path $outDir "msvc_$Mode.txt"
Set-Content -Encoding utf8 -Path $report -Value @($warnings)

"{0} files, {1} distinct warnings, {2} errors -> {3}" -f `
    @($sources).Count, @($warnings).Count, @($errors).Count, $report
$errors | Select-Object -First 10
$warnings | ForEach-Object { if ($_ -match 'warning (C\d+)') { $Matches[1] } } |
    Group-Object | Sort-Object Count -Descending |
    ForEach-Object { '{0,5}  {1}' -f $_.Count, $_.Name }
exit 0
