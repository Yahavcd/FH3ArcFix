param(
    [string]$GamePath,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$pkg = Get-AppxPackage Microsoft.OpusPG -ErrorAction SilentlyContinue
if (-not $GamePath -and $pkg) { $GamePath = $pkg.InstallLocation }
if (-not $GamePath -or -not (Test-Path $GamePath)) {
    throw "FH3 install folder was not found: $GamePath"
}

$dst = Join-Path $GamePath 'd3d12.dll'
$backup = Join-Path $GamePath 'd3d12.dll.FH3ArcFix-backup'
$candidates = @(
    (Join-Path $PSScriptRoot 'd3d12.dll'),
    (Join-Path $PSScriptRoot 'release\d3d12.dll'),
    (Join-Path $PSScriptRoot 'build\d3d12.dll')
)
$known = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

Get-Process forza_x64_release_final -ErrorAction SilentlyContinue | Stop-Process -Force

if (Test-Path $dst) {
    if ($known -and -not $Force) {
        $dstHash = (Get-FileHash $dst -Algorithm SHA256).Hash
        $knownHash = (Get-FileHash $known -Algorithm SHA256).Hash
        if ($dstHash -ne $knownHash) {
            throw 'The active d3d12.dll does not match this FH3ArcFix build. Refusing to remove it. Use -Force only if you know it is safe.'
        }
    } elseif (-not $known -and -not $Force) {
        throw 'Cannot verify the active d3d12.dll because the FH3ArcFix binary is missing from this package. Use -Force only if you know it is safe.'
    }

    Remove-Item $dst -Force
    Write-Host "Removed: $dst"
}

if (Test-Path $backup) {
    Move-Item $backup $dst
    Write-Host 'Restored the previous d3d12.dll backup.'
}
