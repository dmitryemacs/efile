param(
    [ValidateSet('debug', 'release')]
    [string]$Config = 'debug',
    [switch]$Deploy,
    [switch]$Run,
    [string[]]$AppArgs
)

$ErrorActionPreference = 'Stop'

$root   = Split-Path -Parent $PSScriptRoot
$cmake  = 'D:\Qt\Tools\CMake_64\bin\cmake.exe'
$qtBin  = 'D:\Qt\6.12.0\llvm-mingw_64\bin'
$llvmBin = 'D:\Qt\Tools\llvm-mingw2217_64\bin'

if (-not (Test-Path $cmake)) { throw "cmake not found: $cmake" }

Write-Host "==> Configure ($Config)" -ForegroundColor Cyan
& $cmake --preset $Config -S $root

Write-Host "==> Build ($Config)" -ForegroundColor Cyan
& $cmake --build --preset $Config

$exe = Join-Path $root "build\$Config\file_manager.exe"
if (-not (Test-Path $exe)) { throw "build failed, exe not found: $exe" }

if ($Deploy) {
    $windeployqt = Join-Path $qtBin 'windeployqt.exe'
    Write-Host "==> Deploy to $(Split-Path $exe)" -ForegroundColor Cyan
    & $windeployqt --no-translations --no-system-d3d-compiler $exe
}

if ($Run) {
    Write-Host "==> Run $exe" -ForegroundColor Cyan
    $env:PATH = "$qtBin;$llvmBin;$env:PATH"
    & $exe @AppArgs
}

Write-Host "OK: $exe" -ForegroundColor Green
