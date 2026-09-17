param(
    [switch]$RemoveDumps,
    [switch]$ResetDevOverride
)

$ErrorActionPreference = 'Stop'

# Remove environment variables used only during FH3ArcFix investigation.
$vars = @(
    'VKD3D_CONFIG','VKD3D_DISABLE_EXTENSIONS','VKD3D_DEBUG','VKD3D_LOG_FILE',
    'VKD3D_SHADER_CACHE_PATH','VK_DRIVER_FILES','VK_LOADER_LAYERS_DISABLE',
    'DXVK_CONFIG_FILE','DXVK_LOG_LEVEL','DXVK_LOG_PATH'
)
foreach ($v in $vars) {
    [Environment]::SetEnvironmentVariable($v, $null, 'User')
}

# Reset per-app D3D debug-layer / DRED settings if D3DConfig is installed.
if (Get-Command d3dconfig.exe -ErrorAction SilentlyContinue) {
    & d3dconfig.exe --reset --confirm | Out-Host
}

# Remove the per-executable WER full-dump configuration used during debugging.
$dumpKey = 'HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\forza_x64_release_final.exe'
if (Test-Path $dumpKey) {
    Remove-Item $dumpKey -Recurse -Force
}

if ($RemoveDumps -and (Test-Path 'C:\FH3-Dumps')) {
    Remove-Item 'C:\FH3-Dumps' -Recurse -Force
}

# DevOverrideEnable was only needed for Application.Local experiments, not the
# final root-local d3d12 proxy. It is opt-in here because it is machine-wide.
if ($ResetDevOverride) {
    $ifeo = 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options'
    Remove-ItemProperty -Path $ifeo -Name DevOverrideEnable -ErrorAction SilentlyContinue
}

Write-Host 'FH3ArcFix debugging environment cleaned.'
Write-Host 'Sign out/in once if old user environment variables are still visible in already-running processes.'
