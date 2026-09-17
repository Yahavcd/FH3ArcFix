$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$dll = Join-Path $root 'build-experimental\d3d12.dll'
if (-not (Test-Path $dll)) { throw 'Build first: build-experimental\d3d12.dll is missing.' }

$staging = Join-Path $env:TEMP ('FH3ArcFix-exp-all-gpus-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $staging | Out-Null
Copy-Item $dll (Join-Path $staging 'd3d12.dll')
Copy-Item (Join-Path $root 'EXPERIMENTAL_README.md') $staging

$out = Join-Path $root 'FH3ArcFix-v1.0.1-experimental-all-gpus-win64.zip'
if (Test-Path $out) { Remove-Item $out -Force }
Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $out -Force
Remove-Item $staging -Recurse -Force

$sum = Get-FileHash $out -Algorithm SHA256
$sumFile = Join-Path $root 'FH3ArcFix-v1.0.1-experimental-all-gpus-SHA256.txt'
"$($sum.Hash.ToLower())  $(Split-Path $out -Leaf)" | Set-Content -Encoding ascii $sumFile

Write-Host "Created: $out"
Write-Host "Created: $sumFile"
