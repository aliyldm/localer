$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$outputRoot = Join-Path $projectRoot 'vendor\packet-api'
$archiveRoot = Join-Path $projectRoot 'vendor\winpcap'
$archive = Join-Path $archiveRoot 'WinPcap_4_1_3.exe'
New-Item -ItemType Directory -Force -Path $outputRoot,$archiveRoot | Out-Null
if (-not (Test-Path -LiteralPath $archive)) { Invoke-WebRequest -Uri 'https://www.winpcap.org/install/bin/WinPcap_4_1_3.exe' -OutFile $archive -TimeoutSec 90 }
# Official archive fingerprint published at https://www.winpcap.org/install/ .
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA1).Hash -ne 'E2516FCD1573E70334C8F50BEE5241CDFDF48A00') { throw 'WinPcap official archive checksum mismatch.' }
$sevenZip = (Get-Command 7z.exe -ErrorAction SilentlyContinue).Source
if (-not $sevenZip -and (Test-Path -LiteralPath 'C:\Program Files\7-Zip\7z.exe')) { $sevenZip='C:\Program Files\7-Zip\7z.exe' }
if (-not $sevenZip) { throw '7-Zip is required to extract the DLL without running the installer.' }
$extractRoot = Join-Path $archiveRoot 'extracted'
if (-not (Test-Path -LiteralPath $extractRoot)) {
    & $sevenZip x $archive ('-o' + $extractRoot) -aou -bso0
    if ($LASTEXITCODE -ne 0) { throw 'WinPcap extraction failed.' }
}
$expectedDllHash = 'AB3CCE674F5216895FD26A073771F82B05D4C8B214A89F0F288A59774A06B14B'
$candidate = Get-ChildItem -LiteralPath $extractRoot -Recurse -Filter 'Packet*.dll' | Where-Object { (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash -eq $expectedDllHash } | Select-Object -First 1
if (-not $candidate) { throw 'Expected amd64 BSD Packet API DLL not found.' }
Copy-Item -LiteralPath $candidate.FullName -Destination (Join-Path $outputRoot 'Packet.dll') -Force
$license = Join-Path $projectRoot 'licenses\WinPcap-LICENSE.html'
if (-not (Test-Path -LiteralPath $license)) { Invoke-WebRequest -Uri 'https://www.winpcap.org/misc/copyright.htm' -OutFile $license -TimeoutSec 30 }
Write-Output 'Packet API compatibility DLL ready. No installer or capture driver was executed.'
