$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$CocosPath = Join-Path $RepoRoot "cocos2d"
$BuildPath = Join-Path $RepoRoot "build"

if (-not (Test-Path (Join-Path $CocosPath "CMakeLists.txt"))) {
    throw "Cocos2d-x is missing. Run .\scripts\setup_cocos.ps1 first."
}

cmake -S $RepoRoot -B $BuildPath -G "Visual Studio 17 2022" -A x64
cmake --build $BuildPath --config Release

Write-Host "Build complete. Look under $BuildPath for the doodle_fight executable."
