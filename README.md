# \# FH3ArcFix

# 

# A small compatibility fix for \*\*Forza Horizon 3 (PC/UWP)\*\* failing to start on modern  

# \*\*Intel Arc\*\* GPUs.

# 

# \## Status

# 

# \*\*v1.0.0 — working on the tested system.\*\*

# 

# Tested with:

# 

# \- Forza Horizon 3 / `Microsoft.OpusPG` \*\*1.0.125.2\*\*

# \- \*\*Intel Arc Pro B50\*\*

# \- Windows 11 24H2 (26100-series)

# 

# The game previously crashed shortly after launch with `0xc0000005` at  

# `forza\_x64\_release\_final.exe + 0x1f28dec`. With FH3ArcFix installed, the game launches and  

# normal gameplay works on the tested machine.

# 

# > \*\*Known issue:\*\* FH3 may still show \*\*FH204 — Unsupported graphics card detected\*\*.  

# > Choose \*\*Ignore and continue\*\*. FH204 suppression is intentionally not part of v1.0.

# 

# \## What it fixes

# 

# FH3 creates a \*\*null D3D12 depth-stencil view\*\* with `DXGI\_FORMAT\_UNKNOWN`. On the tested Arc  

# system, the D3D12 runtime rejects this call, removes the device with `DXGI\_ERROR\_INVALID\_CALL`,  

# and FH3 enters its `Video card` fatal path.

# 

# FH3ArcFix proxies the system `d3d12.dll` and intercepts only that exact invalid call. It changes  

# the null DSV format from `DXGI\_FORMAT\_UNKNOWN` to `DXGI\_FORMAT\_D32\_FLOAT`. Every other D3D12  

# call passes through unchanged.

# 

# See \[docs/TECHNICAL.md](docs/TECHNICAL.md) for the debugging details.

# 

# \## Install (binary release)

# 

# 1\. Download and extract the release ZIP.

# 2\. Open PowerShell in the extracted folder.

# 3\. Run:

# 

# ```powershell

# powershell -ExecutionPolicy Bypass -File .\\install.ps1

# ```

# 

# The installer finds the registered `Microsoft.OpusPG` package and places `d3d12.dll` in FH3's  

# install directory.

# 

# FH3ArcFix does \*\*not\*\* require global Vulkan/DXVK variables, D3D debug settings, driver changes,  

# or registry changes.

# 

# \## Uninstall

# 

# ```powershell

# powershell -ExecutionPolicy Bypass -File .\\uninstall.ps1

# ```

# 

# \## Build from source

# 

# The build scripts and source files are included in the GitHub source repository/source archive,  

# not in the binary release ZIP.

# 

# Requirements:

# 

# \- Visual Studio Build Tools with \*\*Desktop development with C++\*\*

# \- Windows SDK

# 

# From an \*\*x64 Native Tools Command Prompt\*\*:

# 

# ```cmd

# build.cmd

# ```

# 

# Output:

# 

# ```text

# build\\d3d12.dll

# ```

# 

# For diagnostic `OutputDebugString` messages usable with DebugView:

# 

# ```cmd

# build-debug.cmd

# ```

# 

# \## Safety / scope

# 

# \- The runtime patch is enabled only for an adapter with Intel vendor ID `0x8086` whose name  

# &#x20; contains `Arc`.

# \- The installer refuses untested FH3 package versions by default. Use `-Force` only if you are  

# &#x20; deliberately testing another build.

# \- The installer refuses to overwrite a different existing `d3d12.dll` unless `-Force` is used.

# \- No game files are distributed by this project.

# \- This project does not bypass Microsoft Store licensing or ownership checks.

# 

# \## Compatibility

# 

# Currently confirmed only on the system listed above. Reports from other Arc GPUs are welcome.  

# Please include Windows build, driver version, and FH3 package version.

# 

# \## Disclaimer

# 

# Unofficial community compatibility project. Not affiliated with or endorsed by Microsoft,  

# Playground Games, Turn 10 Studios, or Intel.

# 

# \## License

# 

# MIT. See \[LICENSE](LICENSE).

