# PICO Business Streaming MR Demo

Minimal Unreal Engine 5.6 mixed-reality demo for PICO 4 Ultra Enterprise over
PICO Business Streaming Wi-Fi.

The headset shows system passthrough while Unreal renders one animated cube.
The cube is anchored about 1.5 metres in front of the first valid HMD pose and
remains fixed in the local tracking space while the user moves around it.

## Scope

- UE 5.6 OpenXR application running on Windows
- PICO Business Streaming owns tracking, encoding, Wi-Fi transport and headset presentation
- PICO OpenXR runtime with projection alpha composition
- no custom Android client, WebRTC signalling service or media transport
- no spatial mesh, real-world occlusion, controllers, hands, shadows or audio

## Requirements

- Unreal Engine 5.6.x with Windows C++ build support
- PICO 4 Ultra Enterprise
- PICO Business Streaming / Streaming Service installed on the PC and headset
- headset and PC connected over a suitable 5 GHz or Wi-Fi 6 network

The verified configuration uses the PICO Streaming OpenXR runtime and AVC/H.264.
HEVC can also be used. AV1 currently fails in the PICO runtime encoder path on
the tested RTX 50-series system. SteamVR accepts AV1 but does not expose the
OpenXR passthrough or alpha-blend functionality required by this MR demo.

## Build

Run from PowerShell:

```powershell
.\package-demo.ps1
```

The staged executable is written to:

```text
artifacts\RemoteXRDemo-Win64\Windows\RemoteXRDemo.exe
```

You can override the Unreal installation directory:

```powershell
.\package-demo.ps1 -EngineRoot 'D:\Epic Games\UE_5.6'
```

## Run

1. In PICO Business Streaming, select OpenXR mode and connect the headset over Wi-Fi.
2. Keep the codec on AVC/H.264 or HEVC.
3. Run:

```powershell
.\run-pico-business-mr.ps1
```

The launcher selects the PICO runtime for the demo process without depending on
the system-wide OpenXR runtime. A watchdog recreates the OpenXR session after a
headset reconnect because the tested PICO runtime otherwise loses passthrough.

See [docs/verified-baseline.md](docs/verified-baseline.md) for the accepted
headset result and critical alpha settings.

