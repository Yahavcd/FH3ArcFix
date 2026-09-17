# FH3ArcFix v1.0.0

FH3ArcFix is a small compatibility shim for **Forza Horizon 3 (PC/UWP)** on modern **Intel Arc** GPUs.

On the tested Arc Pro B50 system, FH3 1.0.125.2 crashed at startup after an invalid null D3D12 depth-stencil view caused device removal with `DXGI_ERROR_INVALID_CALL`. FH3ArcFix proxies `d3d12.dll` locally and corrects only that confirmed call.

## Tested

- Forza Horizon 3 / Microsoft.OpusPG 1.0.125.2
- Intel Arc Pro B50
- Windows 11 24H2 / 26100-series

## Install

Extract `FH3ArcFix-v1.0.0-win64.zip` and run:

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

FH204 may still appear. Choose **Ignore and continue**. FH204 suppression is intentionally deferred to a later version.

## Notes

- No game files are included.
- No licensing/ownership checks are bypassed.
- No DXVK/VKD3D or global Vulkan settings are required.
- Please report results from other Intel Arc GPUs.
