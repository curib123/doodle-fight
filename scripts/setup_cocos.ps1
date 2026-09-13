$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$CocosPath = Join-Path $RepoRoot "cocos2d"
$CocosCMake = Join-Path $CocosPath "CMakeLists.txt"

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $FilePath $($Arguments -join ' ')"
    }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git is not installed or is not available in PATH."
}

if (-not (Test-Path $CocosCMake)) {
    Write-Host "Cloning Cocos2d-x v4 into $CocosPath..."
    Invoke-NativeCommand git clone --depth 1 --branch v4 https://github.com/cocos2d/cocos2d-x.git $CocosPath
}
else {
    Write-Host "Cocos2d-x source already exists at $CocosPath"
}

Push-Location $CocosPath
try {
    Write-Host "Initializing Cocos2d-x submodules..."
    Invoke-NativeCommand git submodule update --init --recursive

    $DownloadDeps = Join-Path $CocosPath "download-deps.py"
    if (Test-Path $DownloadDeps) {
        $PythonCommand = $null
        $PythonArgs = @()

        if (Get-Command py -ErrorAction SilentlyContinue) {
            $PythonCommand = "py"
            $PythonArgs = @("-3", $DownloadDeps)
        }
        elseif (Get-Command python -ErrorAction SilentlyContinue) {
            $PythonCommand = "python"
            $PythonArgs = @($DownloadDeps)
        }

        if ($PythonCommand) {
            Write-Host "Downloading Cocos2d-x external dependencies..."
            Invoke-NativeCommand $PythonCommand @PythonArgs
        }
        else {
            Write-Warning "Python was not found. Cocos2d-x download-deps.py was not run. Install Python 3, then run .\scripts\setup_cocos.ps1 again if the build reports missing external libraries."
        }
    }
}
finally {
    Pop-Location
}

Write-Host "Cocos2d-x setup complete."
