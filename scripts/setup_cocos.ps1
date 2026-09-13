$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$CocosPath = Join-Path $RepoRoot "cocos2d"

if (Test-Path (Join-Path $CocosPath "CMakeLists.txt")) {
    Write-Host "Cocos2d-x is already available at $CocosPath"
    exit 0
}

Write-Host "Cloning Cocos2d-x v4..."
git clone --depth 1 --branch v4 https://github.com/cocos2d/cocos2d-x.git $CocosPath

Push-Location $CocosPath
try {
    git submodule update --init --recursive
}
finally {
    Pop-Location
}

Write-Host "Cocos2d-x setup complete."
