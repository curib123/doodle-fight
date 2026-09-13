param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$CocosPath = Join-Path $RepoRoot "cocos2d"
$BuildPath = Join-Path $RepoRoot "build"
$CocosCMake = Join-Path $CocosPath "CMakeLists.txt"
$CachePath = Join-Path $BuildPath "CMakeCache.txt"

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [string[]]$Arguments = @()
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $FilePath $($Arguments -join ' ')"
    }
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake is not installed or is not available in PATH. Install CMake and reopen PowerShell."
}

if (-not (Test-Path $CocosCMake)) {
    throw "Cocos2d-x is missing. Run .\scripts\setup_cocos.ps1 first."
}

if ($Clean -and (Test-Path $BuildPath)) {
    Write-Host "Removing previous build directory: $BuildPath"
    Remove-Item $BuildPath -Recurse -Force
}

# A stale cache from another generator/architecture commonly breaks Visual Studio builds.
if (Test-Path $CachePath) {
    $Cache = Get-Content $CachePath -Raw
    $WrongGenerator = $Cache -notmatch "CMAKE_GENERATOR:INTERNAL=Visual Studio 17 2022"
    $WrongPlatform = $Cache -match "CMAKE_GENERATOR_PLATFORM:INTERNAL=(?!x64)"

    if ($WrongGenerator -or $WrongPlatform) {
        Write-Host "Removing incompatible CMake cache..."
        Remove-Item $BuildPath -Recurse -Force
    }
}

Write-Host "Configuring Doodle Fight ($Configuration, Visual Studio 2022 x64)..."

# CMake 4.x is stricter with older third-party CMake projects used by Cocos2d-x v4.
# Supplying the policy floor keeps those dependencies compatible without modifying them.
$ConfigureArgs = @(
    "-S", $RepoRoot,
    "-B", $BuildPath,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
)
Invoke-NativeCommand -FilePath "cmake" -Arguments $ConfigureArgs

Write-Host "Building doodle_fight target..."
$BuildArgs = @(
    "--build", $BuildPath,
    "--config", $Configuration,
    "--target", "doodle_fight",
    "--parallel"
)
Invoke-NativeCommand -FilePath "cmake" -Arguments $BuildArgs

$Exe = Get-ChildItem -Path $BuildPath -Filter "doodle_fight.exe" -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "\\$Configuration\\" } |
    Select-Object -First 1

if (-not $Exe) {
    $Exe = Get-ChildItem -Path $BuildPath -Filter "doodle_fight.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
}

if (-not $Exe) {
    throw "Build completed but doodle_fight.exe was not found under $BuildPath."
}

Write-Host ""
Write-Host "Build complete."
Write-Host "Executable: $($Exe.FullName)"
