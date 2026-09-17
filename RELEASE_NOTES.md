# FH3ArcFix v1.0.1

FH3ArcFix is a small compatibility shim for **Forza Horizon 3 (PC/UWP)** on modern **Intel Arc** GPUs.

v1.0.1 is a **hardening and cleanup release**. The actual FH3 compatibility workaround is unchanged from v1.0.0.

On the tested Arc Pro B50 system, FH3 1.0.125.2 crashed at startup after an invalid null D3D12 depth-stencil view caused device removal with `DXGI_ERROR_INVALID_CALL`. FH3ArcFix proxies `d3d12.dll` locally and corrects only that confirmed call.

## What changed in v1.0.1

- Removed mutex/hash-map lookup from the `CreateDepthStencilView` hot path.
- Added a fixed hook-record table with explicit C++ release/acquire publication semantics.
- Added a compile-time requirement for lock-free pointer atomics on the supported x64 target.
- Added runtime `VirtualQuery` sanity checking before patching the D3D12 device vtable.
- Improved defensive failure handling and ABI documentation.
- Kept the exact v1.0.0 null-DSV fix, Intel Arc targeting, and proxy exports unchanged.

## Tested

- Forza Horizon 3 / `Microsoft.OpusPG` 1.0.125.2
- Intel Arc Pro B50
- Windows 11 24H2 / 26100-series

## Install

Extract `FH3ArcFix-v1.0.1-win64.zip` and run:

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

Alternatively, place the included `d3d12.dll` directly in the Forza Horizon 3 game directory.

## Known issues

- FH3 may still show **FH204 — Unsupported graphics card detected**. Choose **Ignore and continue**.
- **Disable Frame Smoothing in FH3.** On the tested Arc system, enabling it causes a severe performance drop.
- Minor pop-in or shadow/lighting flicker may occur; it is not yet known whether this is Arc-specific or normal FH3 rendering behavior.

## Notes

- No game files are included.
- No licensing or ownership checks are bypassed.
- No DXVK/VKD3D or global Vulkan settings are required.
- Please report results from other Intel Arc GPUs, including GPU model, driver version, Windows build, and FH3 package version.
