# Builds and runs the unit tests (tests/unit, doctest) for the engine-free
# parts of the mod, with the toolchain kdev uses: MSVC x86, /std:c++17.
# Warnings are errors here. Exit code 0 means every test passed.
#
#   pwsh -File tools/tests/run_unit_tests.ps1

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$src  = Join-Path $repo 'patches\Accessibility'
$out  = Join-Path $repo 'build\unit_tests'

# The sources under test. Each must build without the game: no hooks, no
# engine addresses, no Windows API.
$underTest = @(
    'bridge_http.cpp',
    'camera_orient_pick.cpp',
    'dialog_speech_marker.cpp',
    'engine_compass.cpp',
    'engine_keymap_codes.cpp',
    'floor_puzzle_plates.cpp',
    'minigame_turret_lead.cpp',
    'save_points.cpp',
    'shortest_paths.cpp',
    'stuck_detect.cpp',
    'text_match.cpp',
    'walkmesh_geometry.cpp'
)

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ build tools not found (vswhere).' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars32.bat'

New-Item -ItemType Directory -Force $out | Out-Null
$files = @(Get-ChildItem (Join-Path $repo 'tests\unit') -Filter *.cpp | ForEach-Object FullName) +
         @($underTest | ForEach-Object { Join-Path $src $_ })
$quoted = ($files | ForEach-Object { "`"$_`"" }) -join ' '
$doctest = Join-Path $repo 'third_party\doctest'

# A batch file sidesteps nested quoting through cmd /c. DOCTEST_CONFIG_USE_STD_
# HEADERS: doctest otherwise forward-declares std::tuple, which current MSVC
# rejects (C5285).
$bat = Join-Path $out 'build.bat'
@"
@echo off
call "$vcvars" >nul || exit /b 1
cl /nologo /EHsc /W3 /WX /std:c++17 /MD /DDOCTEST_CONFIG_USE_STD_HEADERS /I"$src" /I"$doctest" /Fo"$out\\" /Fe"$out\unit_tests.exe" $quoted
"@ | Set-Content -Encoding ascii $bat
& cmd /c "`"$bat`""
if ($LASTEXITCODE -ne 0) { throw "Unit test build failed ($LASTEXITCODE)." }

& (Join-Path $out 'unit_tests.exe')
exit $LASTEXITCODE
