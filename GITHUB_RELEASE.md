# FH3ArcFix v1.0.1

Hardening and cleanup release. **The actual FH3 compatibility workaround is unchanged from v1.0.0.**

FH3ArcFix fixes the Forza Horizon 3 startup crash observed on Intel Arc by correcting the specific invalid null D3D12 depth-stencil view that causes device removal with `DXGI_ERROR_INVALID_CALL`.

## v1.0.1 changes

- Removes locking and hash-map lookup from the `CreateDepthStencilView` hot path.
- Uses a fixed hook-record table with explicit C++ release/acquire publication semantics.
- Requires lock-free pointer atomics at compile time on x64.
- Adds runtime sanity checking of the target D3D12 vtable entry before patching.
- Improves defensive failure handling and documents the `ID3D12Device` ABI assumptions.
- Keeps the exact v1.0.0 `DXGI_FORMAT_UNKNOWN` -> `DXGI_FORMAT_D32_FLOAT` workaround unchanged.

## Tested

- Forza Horizon 3 / `Microsoft.OpusPG` 1.0.125.2
- Intel Arc Pro B50
- Windows 11 24H2 / 26100-series

## Install

Extract `FH3ArcFix-v1.0.1-win64.zip` and run:

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

Or place `d3d12.dll` directly in the Forza Horizon 3 game directory.

## Known issues

- FH3 may still show **FH204 — Unsupported graphics card detected**. Choose **Ignore and continue**.
- **Disable Frame Smoothing in FH3.** On the tested Arc system, enabling it causes a severe performance drop.
- Minor pop-in or shadow/lighting flicker may occur; it is not yet known whether this is Arc-specific or normal FH3 rendering behavior.

No game files are included and no licensing/ownership checks are bypassed.

Feedback from other Intel Arc GPUs is very welcome. Please include GPU model, driver version, Windows build, and FH3 package version.
