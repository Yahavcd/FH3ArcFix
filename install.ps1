param(
    [string]$GamePath,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$ExpectedVersion = [Version]'1.0.125.2'

$pkg = Get-AppxPackage Microsoft.OpusPG -ErrorAction SilentlyContinue
if (-not $pkg) {
    throw 'Microsoft.OpusPG (Forza Horizon 3) is not registered for the current user.'
}

if (-not $GamePath) { $GamePath = $pkg.InstallLocation }
if (-not $GamePath -or -not (Test-Path $GamePath)) {
    throw "FH3 install folder was not found: $GamePath"
}

if ([Version]$pkg.Version -ne $ExpectedVersion -and -not $Force) {
    throw "Untested FH3 package version $($pkg.Version). Tested version is $ExpectedVersion. Re-run with -Force only if you accept the risk."
}

# Binary release: d3d12.dll sits next to this script.
# Source checkout: package-release.ps1 also stages release\d3d12.dll; local builds use build\d3d12.dll.
$candidates = @(
    (Join-Path $PSScriptRoot 'd3d12.dll'),
    (Join-Path $PSScriptRoot 'release\d3d12.dll'),
    (Join-Path $PSScriptRoot 'build\d3d12.dll')
)
$src = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $src) {
    throw 'Bundled d3d12.dll not found. Use the binary release package or build the project first.'
}

$dst = Join-Path $GamePath 'd3d12.dll'
$backup = Join-Path $GamePath 'd3d12.dll.FH3ArcFix-backup'

Get-Process forza_x64_release_final -ErrorAction SilentlyContinue | Stop-Process -Force

if (Test-Path $dst) {
    $srcHash = (Get-FileHash $src -Algorithm SHA256).Hash
    $dstHash = (Get-FileHash $dst -Algorithm SHA256).Hash
    if ($srcHash -eq $dstHash) {
        Write-Host 'FH3ArcFix is already installed.'
        exit 0
    }

    if (-not $Force) {
        throw "A different d3d12.dll already exists at $dst. FH3ArcFix will not overwrite another wrapper/mod. Remove it first or use -Force to back it up."
    }

    if (Test-Path $backup) {
        throw "Backup already exists: $backup. Resolve it manually before forcing installation."
    }
    Move-Item $dst $backup
    Write-Host "Backed up existing d3d12.dll to: $backup"
}

Copy-Item $src $dst -Force
Write-Host "Installed FH3ArcFix: $dst"
Write-Host "Package version: $($pkg.Version)"
Write-Host 'Known issue: FH204 (unsupported graphics card) may still appear; choose Ignore and continue.'
Write-Host 'No global registry values or environment variables were changed.'
