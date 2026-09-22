$ErrorActionPreference = 'Stop'

$runtimeCandidates = @(
    'C:\Program Files\Streaming Service\openxr_runtime_pc\PicoStreamingXRRuntime\picostreaming-openxr.json',
    'C:\Program Files\PICO Connect\Business Streaming\openvr_driver\resources\ps_xrt\picostreaming-openxr.json'
)
$runtime = $runtimeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
$server = 'C:\Program Files\Streaming Service\ps_server.exe'
$frontend = 'C:\Program Files\PICO Connect\Business Streaming\Business Streaming.exe'
$demo = Join-Path $PSScriptRoot 'artifacts\RemoteXRDemo-Win64\Windows\RemoteXRDemo.exe'
$watchdog = Join-Path $PSScriptRoot 'pico-mr-reconnect-watchdog.ps1'
$runtimeLog = Join-Path $env:APPDATA 'PICO Connect\logs\openxr\PSOpenXR.log'

if (-not $runtime) {
    throw "PICO Business Streaming OpenXR runtime was not found in: $($runtimeCandidates -join ', ')"
}

foreach ($required in @($runtime, $server, $frontend, $demo, $watchdog)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required file is missing: $required"
    }
}

if (-not (Get-Process -Name 'Business Streaming' -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath $frontend
    Start-Sleep -Seconds 2
}

if (-not (Get-Process -Name ps_server -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath $server -WindowStyle Hidden
    Start-Sleep -Seconds 2
}

$env:XR_RUNTIME_JSON = $runtime
Write-Host "Using the PICO OpenXR runtime for the demo process: $runtime"
Write-Host 'Open Streaming Assistant on the headset and connect it to this PC over 5 GHz Wi-Fi.'
Start-Process -FilePath $demo -WorkingDirectory (Split-Path $demo) -ArgumentList '-PicoBusinessMR -PicoMREnvironmentBlendOnly -vr'
$watchdogArgs = "-NoProfile -ExecutionPolicy Bypass -File `"$watchdog`" -Demo `"$demo`" -RuntimeLog `"$runtimeLog`""
Start-Process -FilePath 'powershell.exe' -WindowStyle Hidden -ArgumentList $watchdogArgs
