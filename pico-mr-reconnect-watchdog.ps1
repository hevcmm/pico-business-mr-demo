param(
    [Parameter(Mandatory = $true)]
    [string]$Demo,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeLog
)

$ErrorActionPreference = 'Continue'
$watchdogLog = Join-Path $PSScriptRoot 'artifacts\pico-mr-reconnect-watchdog.log'
$mutex = [Threading.Mutex]::new($false, 'Local\PicoBusinessMRReconnectWatchdog')
if (-not $mutex.WaitOne(0)) { exit 0 }

function Write-WatchdogLog([string]$Message) {
    Add-Content -LiteralPath $watchdogLog -Value "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff') $Message"
}

function Stop-MRDemo {
    $processes = @(Get-Process -Name 'RemoteXRDemo' -ErrorAction SilentlyContinue)
    foreach ($process in $processes) {
        if ($process.MainWindowHandle -ne 0) {
            [void]$process.CloseMainWindow()
        }
    }
    foreach ($process in $processes) {
        if (-not $process.WaitForExit(3000)) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }
    }
    Write-WatchdogLog 'Old demo stopped while the headset stream is disconnected.'
}

function Start-MRDemo {
    Start-Process -FilePath $Demo -WorkingDirectory (Split-Path $Demo) -ArgumentList '-PicoBusinessMR -PicoMREnvironmentBlendOnly -vr'
    Write-WatchdogLog 'Demo started with a fresh OpenXR session and passthrough layer.'
}

try {
    Write-WatchdogLog 'Reconnect watchdog started.'
    $offset = if (Test-Path -LiteralPath $RuntimeLog) { (Get-Item -LiteralPath $RuntimeLog).Length } else { 0L }
    $sawStop = $false

    while ($true) {
        Start-Sleep -Milliseconds 500
        if (-not (Test-Path -LiteralPath $RuntimeLog)) { continue }

        $length = (Get-Item -LiteralPath $RuntimeLog).Length
        if ($length -lt $offset) {
            $offset = 0L
            $sawStop = $false
        }
        if ($length -eq $offset) { continue }

        $stream = [IO.File]::Open($RuntimeLog, 'Open', 'Read', 'ReadWrite')
        try {
            [void]$stream.Seek($offset, 'Begin')
            $reader = [IO.StreamReader]::new($stream)
            $didRestart = $false
            try {
                while (($line = $reader.ReadLine()) -ne $null) {
                    if ($line.Contains('HandleStopOpenXRStreaming: stop openxr streaming')) {
                        $sawStop = $true
                        Write-WatchdogLog 'Headset stream stopped; waiting for reconnect.'
                    }
                    elseif ($sawStop -and $line.Contains('HandleStartOpenXRStreaming: start openxr streaming')) {
                        Write-WatchdogLog 'Headset stream reconnected.'
                        Stop-MRDemo
                        Start-MRDemo
                        Start-Sleep -Seconds 5
                        $offset = if (Test-Path -LiteralPath $RuntimeLog) { (Get-Item -LiteralPath $RuntimeLog).Length } else { 0L }
                        $sawStop = $false
                        $didRestart = $true
                        break
                    }
                }
                if (-not $didRestart) {
                    $offset = $stream.Position
                }
            }
            finally { $reader.Dispose() }
        }
        finally { $stream.Dispose() }
    }
}
finally {
    $mutex.ReleaseMutex()
    $mutex.Dispose()
}
