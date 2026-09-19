# FH3ArcFix

A small compatibility fix for **Forza Horizon 3 (PC/UWP)** failing to start on modern
**Intel Arc** GPUs.

## Status

**v1.0.1 — confirmed working on multiple Intel Arc GPUs.**

Primary test system:

- Forza Horizon 3 / `Microsoft.OpusPG` **1.0.125.2**
- **Intel Arc Pro B50**
- Windows 11 24H2 (26100-series)

Also community-confirmed working on an **Intel Arc B580** with Windows 11 Pro 25H2.

The game previously crashed shortly after launch with `0xc0000005` at
`forza_x64_release_final.exe + 0x1f28dec`. With FH3ArcFix installed, the game launches and
normal gameplay works on the tested machine.

> **Known issues:**
>
> - FH3 may still show **FH204 — Unsupported graphics card detected**. Choose **Ignore and continue**. FH204 suppression is intentionally not part of v1.0.x.
> - **Disable Frame Smoothing in FH3.** On the tested Arc system, enabling it causes a severe performance drop.
> - Minor pop-in or shadow/lighting flicker may occur. It is currently unclear whether this is Arc-specific or normal FH3 rendering behavior.

## What it fixes

FH3 creates a **null D3D12 depth-stencil view** with `DXGI_FORMAT_UNKNOWN`. On the tested Arc
system, the D3D12 runtime rejects this call, removes the device with `DXGI_ERROR_INVALID_CALL`,
and FH3 enters its `Video card` fatal path.

FH3ArcFix proxies the system `d3d12.dll` and intercepts only that exact invalid call. It changes
the null DSV format from `DXGI_FORMAT_UNKNOWN` to `DXGI_FORMAT_D32_FLOAT`. Every other D3D12
call passes through unchanged.

See [docs/TECHNICAL.md](docs/TECHNICAL.md) for the debugging details.

## Install (binary release)

1. Download and extract the release ZIP.
2. Open PowerShell in the extracted folder.
3. Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

The installer finds the registered `Microsoft.OpusPG` package and places `d3d12.dll` in FH3's
install directory.

FH3ArcFix does **not** require global Vulkan/DXVK variables, D3D debug settings, driver changes,
or registry changes.

You can also manually place `d3d12.dll` in the Forza Horizon 3 game directory.

## Uninstall

```powershell
powershell -ExecutionPolicy Bypass -File .\uninstall.ps1
```

If you installed the DLL manually, removing the local `d3d12.dll` from the game directory also
removes FH3ArcFix.

## Build from source

The build scripts and source files are included in the GitHub source repository/source archive,
not in the binary release ZIP.

Requirements:

- Visual Studio Build Tools with **Desktop development with C++**
- Windows SDK

From an **x64 Native Tools Command Prompt**:

```cmd
build.cmd
```

Output:

```text
build\d3d12.dll
```

For diagnostic `OutputDebugString` messages usable with DebugView:

```cmd
build-debug.cmd
```

## Safety / scope

- The standard release build enables the runtime patch automatically only for an adapter with
  Intel vendor ID `0x8086` whose name contains `Arc`.
- The experimental all-GPUs build removes that adapter restriction but keeps the exact same
  narrowly-scoped D3D12 workaround.
- The installer refuses untested FH3 package versions by default. Use `-Force` only if you are
  deliberately testing another build.
- The installer refuses to overwrite a different existing `d3d12.dll` unless `-Force` is used.
- No game files are distributed by this project.
- This project does not bypass Microsoft Store licensing or ownership checks.

## Compatibility

Confirmed working on:

- **Intel Arc Pro B50** — Windows 11 24H2, FH3 / `Microsoft.OpusPG` 1.0.125.2
- **Intel Arc B580** — Windows 11 Pro 25H2 (community-confirmed)

Reports from other Arc GPUs are welcome.

For non-Arc hardware, use the experimental all-GPUs build described below.

Please include Windows build, driver version, FH3 package version, and exact GPU model when
reporting results.

### Experimental support for non-Arc GPUs

An **experimental all-GPUs build** is available for testing on D3D12 hardware that is not covered
by the standard Intel Arc auto-detection.

This experimental variant removes the Intel Arc adapter check, but keeps the exact same
narrowly-scoped workaround:

- only the confirmed null `CreateDepthStencilView` call is intercepted;
- `DXGI_FORMAT_UNKNOWN` is changed to `DXGI_FORMAT_D32_FLOAT`;
- all other D3D12 calls are forwarded unchanged.

> **Warning:** The experimental build has not been validated on non-Arc GPUs.
> It is intended for testing and game-preservation work on older Intel GPUs,
> AMD/NVIDIA hardware, and less common GPUs such as Moore Threads and other
> alternative D3D12 implementations.

If you test the experimental build, please report:

- exact GPU model
- driver version
- Windows build
- FH3 package version
- whether the game launches successfully
- any crashes, visual artifacts, or performance issues

See [EXPERIMENTAL_README.md](EXPERIMENTAL_README.md) for details.

## Development

FH3ArcFix was developed using **ChatGPT (OpenAI)** through hands-on debugging on real hardware,
including crash-dump analysis, D3D12 Debug Layer/DRED tracing, and iterative testing of the
compatibility shim.

## Disclaimer

Unofficial community compatibility project. Not affiliated with or endorsed by Microsoft,
Playground Games, Turn 10 Studios, Intel, or OpenAI.

## License

MIT. See [LICENSE](LICENSE).
