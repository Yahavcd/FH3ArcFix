$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$dll = Join-Path $root 'build\d3d12.dll'
if (-not (Test-Path $dll)) { throw 'Build first: build\d3d12.dll is missing.' }

$releaseDir = Join-Path $root 'release'
New-Item -ItemType Directory -Force $releaseDir | Out-Null
Copy-Item $dll (Join-Path $releaseDir 'd3d12.dll') -Force

$out = Join-Path $root 'FH3ArcFix-v1.0.0-win64.zip'
if (Test-Path $out) { Remove-Item $out -Force }

$staging = Join-Path $env:TEMP ('FH3ArcFix-release-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $staging | Out-Null
New-Item -ItemType Directory -Force (Join-Path $staging 'docs') | Out-Null
Copy-Item (Join-Path $releaseDir 'd3d12.dll') $staging
Copy-Item (Join-Path $root 'install.ps1') $staging
Copy-Item (Join-Path $root 'uninstall.ps1') $staging
Copy-Item (Join-Path $root 'README.md') $staging
Copy-Item (Join-Path $root 'CHANGELOG.md') $staging
Copy-Item (Join-Path $root 'LICENSE') $staging
Copy-Item (Join-Path $root 'docs\TECHNICAL.md') (Join-Path $staging 'docs\TECHNICAL.md')

Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $out -Force
Remove-Item $staging -Recurse -Force
Write-Host "Created: $out"
