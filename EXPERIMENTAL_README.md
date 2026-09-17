# FH3ArcFix v1.0.1 — Experimental All-GPUs Build

> **EXPERIMENTAL / UNVALIDATED OUTSIDE INTEL ARC PRO B50**
>
> This build removes the stable release's Intel Arc adapter gate so the same
> FH3 D3D12 workaround can be tested on other GPUs, including non-Intel and
> uncommon/Chinese GPUs. It has **not** been validated on those GPUs.

## What is different from stable v1.0.1?

Stable v1.0.1 installs the vtable hook only when DXGI reports Intel vendor ID
`0x8086` and an adapter description containing `Arc`.

This experimental build installs the hook for **any D3D12 device created by FH3**.

The actual compatibility workaround is unchanged and remains narrowly scoped:
only a null Texture2D depth-stencil view matching all of these conditions is altered:

- `resource == nullptr`
- `Format == DXGI_FORMAT_UNKNOWN`
- `ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D`
- `Flags == D3D12_DSV_FLAG_NONE`
- `MipSlice == 0`

For that exact call only, the format is changed to `DXGI_FORMAT_D32_FLOAT`.
All other D3D12 calls are forwarded unchanged.

## Intended use

Use this build only to test FH3 on GPUs that are not automatically covered by
stable v1.0.1 — for example Intel Xe/older Intel GPUs, Moore Threads, other
Chinese GPUs, AMD, NVIDIA, or future hardware.

## Safety / disclaimer

- Untested on non-Arc hardware.
- A successful game launch does not prove full rendering correctness.
- Watch for visual corruption, crashes, device-removal errors, or performance regressions.
- Do not treat a result from one GPU model as validation for an entire vendor/family.
- Keep a copy of any existing local `d3d12.dll`; this build should not overwrite another wrapper/mod.
- No game files or licensing bypasses are included.

## Testing feedback

Please report:

- exact GPU model
- driver version
- Windows build
- FH3 package version
- whether the game launches
- whether gameplay is stable
- any visual artifacts or unusual performance behavior

This experimental build is intended to gather evidence before expanding the
stable release's automatic hardware scope.
