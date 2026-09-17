# Changelog

## v1.0.1

- Hardening and cleanup release; the actual FH3 compatibility workaround is unchanged.
- Removes mutex/hash-map lookup from the `CreateDepthStencilView` hot path.
- Uses a fixed hook-record table with explicit C++ release/acquire publication semantics.
- Requires lock-free pointer atomics at compile time on the supported x64 target.
- Adds `VirtualQuery` validation of the target vtable entry before patching.
- Documents the stable `ID3D12Device::CreateDepthStencilView` vtable slot and base-interface ABI.
- Fails closed if hook bookkeeping cannot be established safely.
- Keeps the same Intel Arc targeting, null-DSV match conditions, `DXGI_FORMAT_D32_FLOAT` substitution, and proxy exports as v1.0.0.
- Validated on the original Intel Arc Pro B50 test system.

## v1.0.0

- First stable public release.
- Fixes the FH3 startup device-removal crash observed on Intel Arc Pro B50.
- Patches only the confirmed invalid null `CreateDepthStencilView` call.
- Limits runtime activation to Intel Arc adapters.
- No Vulkan/DXVK layer, driver downgrade, global environment variable, or persistent debug setting is required.
- Adds safe install/uninstall scripts, source build scripts, CI build workflow, technical notes, and issue template.
- FH204 warning suppression is intentionally deferred; choose **Ignore and continue** when it appears.

## v1.0.0-rc1

- First cleaned release candidate validated on the original test system.
