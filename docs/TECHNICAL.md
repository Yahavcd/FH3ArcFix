# Technical notes

## Observed failure

On the tested machine, Forza Horizon 3 package `Microsoft.OpusPG` version `1.0.125.2`
crashed during startup on Intel Arc Pro B50 with `0xc0000005` at
`forza_x64_release_final.exe + 0x1f28dec`.

D3D12 Debug Layer + WinDbg identified the earlier failure that leads to that fatal path:

- `ID3D12Device::CreateDepthStencilView`
- `pResource == nullptr`
- `D3D12_DEPTH_STENCIL_VIEW_DESC::Format == DXGI_FORMAT_UNKNOWN`
- `ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D`
- The D3D12 runtime reports `CREATEDEPTHSTENCILVIEW_INVALIDFORMAT` and removes the device with `DXGI_ERROR_INVALID_CALL`.

The original call is legal only if the descriptor provides enough type information for a null
view. `DXGI_FORMAT_UNKNOWN` cannot inherit a format when there is no resource.

## Compatibility shim

FH3ArcFix is a local `d3d12.dll` proxy. It forwards D3D12 entry points to the system
`d3d12.dll`, hooks only `ID3D12Device::CreateDepthStencilView`, and changes only the exact
invalid null-DSV pattern observed in FH3:

```
resource == nullptr
Format == DXGI_FORMAT_UNKNOWN
ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D
Flags == D3D12_DSV_FLAG_NONE
MipSlice == 0
```

to `DXGI_FORMAT_D32_FLOAT`.

All other D3D12 calls and DSVs pass through unchanged.

## Why D32_FLOAT?

The descriptor is null and has no backing resource. The runtime still requires a valid depth
format to define the null DSV. `D32_FLOAT` is a valid depth format and is the value empirically
validated on the tested Arc Pro B50 system. The patch does not reinterpret any real resource.

## Scope

The release build activates the vtable hook only when the adapter reports Intel vendor ID
`0x8086` and its description contains `Arc`.
