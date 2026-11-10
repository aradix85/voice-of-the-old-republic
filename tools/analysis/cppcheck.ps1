# Runs cppcheck over patches/Accessibility. Exit code 0 means no finding
# outside tools/analysis/cppcheck_suppressions.txt.
#
#   pwsh -File tools/analysis/cppcheck.ps1            # warnings, performance, portability
#   pwsh -File tools/analysis/cppcheck.ps1 -Style     # plus style and unused functions (a report)

param([switch] $Style)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$exe  = Join-Path $env:ProgramFiles 'Cppcheck\cppcheck.exe'
if (-not (Test-Path $exe)) { throw "cppcheck not found at $exe." }

$enable = if ($Style) { 'warning,performance,portability,style,unusedFunction' }
          else { 'warning,performance,portability' }
Push-Location $repo
try {
    $cppArgs = @(
        "--enable=$enable", '--platform=win32W', '-D_WIN32', '-D_MSC_VER=1930',
        '--std=c++17', '--language=c++', '-I', 'patches/Accessibility',
        '--inline-suppr', '--suppressions-list=tools/analysis/cppcheck_suppressions.txt',
        '--template={file}:{line}: {severity}: {message} [{id}]',
        '-q', '--error-exitcode=1', 'patches/Accessibility')
    # unusedFunction needs the whole program in one process.
    if (-not $Style) { $cppArgs += @('-j', [Environment]::ProcessorCount) }
    & $exe @cppArgs
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
