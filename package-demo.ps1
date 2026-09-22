param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6'
)

$ErrorActionPreference = 'Stop'

$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$project = Join-Path $PSScriptRoot 'ue\RemoteXRDemo\RemoteXRDemo.uproject'
$archive = Join-Path $PSScriptRoot 'artifacts\RemoteXRDemo-Win64'

foreach ($required in @($uat, $project)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required file is missing: $required"
    }
}

& $uat BuildCookRun `
    -project="$project" `
    -noP4 `
    -platform=Win64 `
    -clientconfig=Development `
    -build `
    -cook `
    -stage `
    -pak `
    -archive `
    -archivedirectory="$archive"

if ($LASTEXITCODE -ne 0) {
    throw "Unreal BuildCookRun failed with exit code $LASTEXITCODE"
}

Write-Host "Packaged demo: $archive\Windows\RemoteXRDemo.exe"

