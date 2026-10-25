$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$python = Join-Path $projectRoot 'vendor\qt-tools\Scripts\python.exe'
$sourceRoot = Join-Path $projectRoot 'vendor\qt-sources'
$archiveRoot = Join-Path $projectRoot 'vendor\qt-source-archives'
$source = Join-Path $sourceRoot '6.10.3\Src\qtbase'
if (-not (Test-Path -LiteralPath $source)) {
    & $python -m aqt install-src windows 6.10.3 --archives qtbase -O $sourceRoot -k -d $archiveRoot
    if ($LASTEXITCODE -ne 0) { throw 'Official Qt source download failed.' }
}
$archive = Join-Path $archiveRoot 'qtbase-6.10.3-source.7z'
if (-not (Test-Path -LiteralPath $archive)) {
    $sevenZip = (Get-Command 7z.exe -ErrorAction SilentlyContinue).Source
    if (-not $sevenZip -and (Test-Path -LiteralPath 'C:\Program Files\7-Zip\7z.exe')) { $sevenZip='C:\Program Files\7-Zip\7z.exe' }
    if (-not $sevenZip) { throw '7-Zip is required to package matching Qt sources.' }
    & $sevenZip a -t7z $archive $source -mx=7 -mmt=4 -bso0
    if ($LASTEXITCODE -ne 0) { throw 'Qt source compression failed.' }
}
Write-Output ('Matching Qt source archive ready: '+$archive)
