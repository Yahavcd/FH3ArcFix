# Changelog

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
