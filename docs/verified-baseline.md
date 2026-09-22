# PICO Business Streaming MR verified baseline — 2026-09-21

User-verified headset result:
- passthrough visible
- orange cube visible in both eyes
- correct stereo streaming active

Critical configuration:
- PICO 4 Ultra Enterprise over Business Streaming Wi-Fi
- Streaming Service PICO XR Runtime 1.1.46
- AVC / H.264, separate left/right streams, alpha_extension=true
- r.PostProcessing.PropagateAlpha=1
- OpenXR.AlphaInvertPass=0
- r.AlphaInvertPass=0
- xr.OpenXRInvertAlpha=1
- projection flags 0xA: SOURCE_ALPHA + INVERTED_ALPHA_EXT
- no UNPREMULTIPLIED_ALPHA flag
- passthrough composition layer inserted at index 0 behind projection

Why this works:
UE keeps its native inverted alpha in both eye regions. XR_EXT_composition_layer_inverted_alpha makes the PICO runtime interpret both eyes consistently, avoiding the UE 5.6 per-view alpha inversion defect and the transparent-area noise exposed by the unpremultiplied path.

Launch requirement:
Run run-pico-business-mr.ps1 as the normal desktop user so the PICO runtime can write AppData/Roaming/PICO Connect/logs/openxr/PSOpenXR.log.

Build the packaged executable into `artifacts/RemoteXRDemo-Win64` with
`package-demo.ps1`, then launch it with `run-pico-business-mr.ps1`.
