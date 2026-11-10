# Builds and runs the unit tests (tests/unit, doctest) for the engine-free
# parts of the mod. Warnings are errors here. Exit code 0 means every test
# passed.
#
#   pwsh -File tools/tests/run_unit_tests.ps1                  # MSVC x86, as kdev builds
#   pwsh -File tools/tests/run_unit_tests.ps1 -Mode asan       # the same under AddressSanitizer
#   pwsh -File tools/tests/run_unit_tests.ps1 -Mode coverage   # line coverage per source
#
# Coverage builds with clang-cl for x64 (the LLVM install has the profile
# runtime for x64 only); the sources under test do not depend on the word size.

param(
    [ValidateSet('test', 'asan', 'coverage')] [string] $Mode = 'test'
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$src  = Join-Path $repo 'patches\Accessibility'
$out  = Join-Path $repo "build\unit_tests\$Mode"

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
$vcvars = Join-Path $vs ('VC\Auxiliary\Build\' +
    $(if ($Mode -eq 'coverage') { 'vcvars64.bat' } else { 'vcvars32.bat' }))

New-Item -ItemType Directory -Force $out | Out-Null
$tested  = @($underTest | ForEach-Object { Join-Path $src $_ })
$files   = @(Get-ChildItem (Join-Path $repo 'tests\unit') -Filter *.cpp | ForEach-Object FullName) +
           $tested
$quoted  = ($files | ForEach-Object { "`"$_`"" }) -join ' '
$doctest = Join-Path $repo 'third_party\doctest'
$exe     = Join-Path $out 'unit_tests.exe'

# DOCTEST_CONFIG_USE_STD_HEADERS: doctest otherwise forward-declares
# std::tuple, which current MSVC rejects (C5285).
$common = "/nologo /EHsc /std:c++17 /MD /DDOCTEST_CONFIG_USE_STD_HEADERS " +
          "/I`"$src`" /I`"$doctest`" /Fo`"$out\\`" /Fe`"$exe`" $quoted"

$llvm = Join-Path $env:ProgramFiles 'LLVM\bin'
$steps = switch ($Mode) {
    'test' { "cl /W4 /WX $common || exit /b 2", "`"$exe`"" }
    # /Zi gives the sanitizer file and line; its runtime DLL sits beside cl,
    # so the tests run inside the compiler environment.
    'asan' { "cl /W4 /WX /fsanitize=address /Zi /Fd`"$out\\`" $common || exit /b 2", "`"$exe`"" }
    'coverage' {
        if (-not (Test-Path (Join-Path $llvm 'llvm-cov.exe'))) { throw "LLVM not found in $llvm." }
        $profraw  = Join-Path $out 'unit.profraw'
        $profdata = Join-Path $out 'unit.profdata'
        $sources  = ($tested | ForEach-Object { "`"$_`"" }) -join ' '
        "`"$llvm\clang-cl.exe`" -fprofile-instr-generate -fcoverage-mapping $common || exit /b 2",
        "set LLVM_PROFILE_FILE=$profraw",
        "`"$exe`" || exit /b 3",
        "`"$llvm\llvm-profdata.exe`" merge -sparse `"$profraw`" -o `"$profdata`" || exit /b 4",
        "`"$llvm\llvm-cov.exe`" report `"$exe`" -instr-profile=`"$profdata`" $sources"
    }
}

# A batch file sidesteps nested quoting through cmd /c.
$bat = Join-Path $out 'build.bat'
(@('@echo off', "call `"$vcvars`" >nul || exit /b 1") + $steps) | Set-Content -Encoding ascii $bat
& cmd /c "`"$bat`""
exit $LASTEXITCODE
