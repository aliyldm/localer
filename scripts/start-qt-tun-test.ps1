$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$administrator = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $administrator) {
    Write-Host 'Qt sanal ag testi icin Windows yonetici izni bekleniyor.'
    Start-Process -FilePath powershell.exe -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$PSCommandPath+'"')) -Verb RunAs -WindowStyle Normal
    exit
}
Set-Location -LiteralPath $projectRoot
$env:PATH=(Join-Path $projectRoot 'vendor\qt-sdk\6.10.3\mingw_64\bin')+';'+(Join-Path $projectRoot 'vendor\qt-sdk\Tools\mingw1310_64\bin')+';'+$env:PATH
$env:LOCALER_ENGINE_DIR=Join-Path $projectRoot 'dist\Localer-0.3.0-Qt-Windows-x64\easytier'
$env:LOCALER_ARTIFACT_DIR=Join-Path $projectRoot 'artifacts\qt'
Write-Host 'Qt / C++ gercek Windows ag testi calisiyor.'
Write-Host 'Yalnizca test uygulamasi, test arayuzu ve test portlarina gecici izin eklenir ve sonunda kaldirilir.'
& (Join-Path $projectRoot 'build\qt\native\localer_tun_tests.exe') -o (Join-Path $projectRoot 'artifacts\qt\tun-test.txt,txt')
Write-Host ('Test cikis kodu: '+$LASTEXITCODE)
Get-Content -LiteralPath (Join-Path $projectRoot 'artifacts\qt\tun-test.txt') -Tail 18
Read-Host 'Kapatmak icin Enter'
