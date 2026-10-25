$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$vendorRoot = Join-Path $projectRoot 'vendor'
$licenseRoot = Join-Path $projectRoot 'licenses'
New-Item -ItemType Directory -Force -Path $vendorRoot, $licenseRoot | Out-Null

function Get-VerifiedArchive([string]$Url, [string]$Destination, [string]$Hash) {
    if (-not (Test-Path -LiteralPath $Destination)) {
        Invoke-WebRequest -Uri $Url -OutFile $Destination -TimeoutSec 180
    }
    if ((Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash -ne $Hash) {
        throw "SHA-256 mismatch: $Destination. Remove the corrupt archive and retry."
    }
}

$engineZip = Join-Path $vendorRoot 'easytier.zip'
Get-VerifiedArchive 'https://github.com/EasyTier/EasyTier/releases/download/v2.6.4/easytier-windows-x86_64-v2.6.4.zip' $engineZip '27af91e270e554709b048bd32327fefd2dfce5062ae1e8701af7550c6f525f84'
Expand-Archive -LiteralPath $engineZip -DestinationPath (Join-Path $vendorRoot 'easytier') -Force

$wintunZip = Join-Path $vendorRoot 'wintun-0.14.1.zip'
Get-VerifiedArchive 'https://www.wintun.net/builds/wintun-0.14.1.zip' $wintunZip '07c256185d6ee3652e09fa55c0b673e2624b565e02c4b9091c79ca7d2f24ef51'
Expand-Archive -LiteralPath $wintunZip -DestinationPath (Join-Path $vendorRoot 'wintun') -Force
Copy-Item -LiteralPath (Join-Path $vendorRoot 'wintun\wintun\LICENSE.txt') -Destination (Join-Path $licenseRoot 'Wintun-LICENSE.txt') -Force
Copy-Item -LiteralPath (Join-Path $vendorRoot 'wintun\wintun\bin\amd64\wintun.dll') -Destination (Join-Path $vendorRoot 'easytier\easytier-windows-x86_64\wintun.dll') -Force

$sourceZip = Join-Path $vendorRoot 'EasyTier-v2.6.4-source.zip'
if (-not (Test-Path -LiteralPath $sourceZip)) {
    Invoke-WebRequest -Uri 'https://codeload.github.com/EasyTier/EasyTier/zip/refs/tags/v2.6.4' -OutFile $sourceZip -TimeoutSec 180
}
if (-not (Test-Path -LiteralPath (Join-Path $licenseRoot 'EasyTier-LGPL-3.0.txt'))) {
    Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/EasyTier/EasyTier/v2.6.4/LICENSE' -OutFile (Join-Path $licenseRoot 'EasyTier-LGPL-3.0.txt') -TimeoutSec 30
}
if (-not (Test-Path -LiteralPath (Join-Path $licenseRoot 'GPL-3.0.txt'))) {
    Invoke-WebRequest -Uri 'https://www.gnu.org/licenses/gpl-3.0.txt' -OutFile (Join-Path $licenseRoot 'GPL-3.0.txt') -TimeoutSec 30
}
Write-Output 'EasyTier 2.6.4 and Wintun 0.14.1 are ready. Run npm start.'
