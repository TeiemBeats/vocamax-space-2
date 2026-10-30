# VocaMax Space 2 - local Windows build
# Requires: Visual Studio 2022 (Desktop C++ workload), CMake, Inno Setup 6 (optional).
#
#   powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1
#
# Outputs:
#   build\VocaMaxSpace2_artefacts\Release\VST3\VocaMax Space 2.vst3
#   build\VocaMaxSpace2_artefacts\Release\Standalone\VocaMax Space 2.exe
#   dist\VocaMaxSpace2_Setup.exe

param(
    [string]$Configuration = "Release",
    [switch]$SkipInstaller
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

Write-Host "Configuring CMake ..." -ForegroundColor Cyan
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DVOCOMAX_BUILD_DSP_TESTS=ON
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

Write-Host "Building VST3 + Standalone ($Configuration) ..." -ForegroundColor Cyan
cmake --build build --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "Running DSP checks ..." -ForegroundColor Cyan
ctest --test-dir build -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "DSP checks failed" }

$vst = "build\VocaMaxSpace2_artefacts\$Configuration\VST3\VocaMax Space 2.vst3"
$exe = "build\VocaMaxSpace2_artefacts\$Configuration\Standalone\VocaMax Space 2.exe"

if (-not (Test-Path $vst)) { throw "VST3 bundle missing: $vst" }
if (-not (Test-Path $exe)) { throw "Standalone missing: $exe" }

Write-Host "Plugin built:" -ForegroundColor Green
Write-Host "  $vst"
Write-Host "  $exe"

if ($SkipInstaller) {
    Write-Host "Skipping installer (-SkipInstaller)." -ForegroundColor Yellow
    exit 0
}

$iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
    Write-Warning "Inno Setup 6 not found. Install it from https://jrsoftware.org/isdl.php and rerun to build the setup .exe."
    exit 0
}

Write-Host "Building installer ..." -ForegroundColor Cyan
& $iscc "installer\VocaMaxSpace2.iss"
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed" }

Write-Host "Installer built: dist\VocaMaxSpace2_Setup.exe" -ForegroundColor Green
