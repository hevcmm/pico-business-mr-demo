$ErrorActionPreference = 'Stop'

$steamVrRoot = 'C:\Program Files (x86)\Steam\steamapps\common\SteamVR'
$runtime = Join-Path $steamVrRoot 'steamxr_win64.json'
$vrMonitor = Join-Path $steamVrRoot 'bin\win64\vrmonitor.exe'
$frontend = 'C:\Program Files\PICO Connect\Business Streaming\Business Streaming.exe'
$demo = Join-Path $PSScriptRoot 'artifacts\RemoteXRDemo-Win64\Windows\RemoteXRDemo.exe'

foreach ($required in @($runtime, $vrMonitor, $frontend, $demo)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required file is missing: $required"
    }
}

if (-not (Get-Process -Name 'Business Streaming' -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath $frontend
    Start-Sleep -Seconds 2
}

if (-not (Get-Process -Name vrmonitor -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath $vrMonitor -WindowStyle Hidden
    Start-Sleep -Seconds 5
}

$env:XR_RUNTIME_JSON = $runtime
Write-Host "Using SteamVR OpenXR runtime for the demo process: $runtime"
Write-Host 'Using PICO alpha composition through SteamVR with an opaque OpenXR environment blend mode.'
$demoArgs = '-PicoBusinessMR -PicoMREnvironmentBlendOnly -vr -ExecCmds="xr.OpenXREnvironmentBlendMode 1"'
Start-Process -FilePath $demo -WorkingDirectory (Split-Path $demo) -ArgumentList $demoArgs
