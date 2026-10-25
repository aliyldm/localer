param([switch]$AllowTestPorts)
$ErrorActionPreference = 'Stop'
$testRoot = Split-Path -Parent $PSScriptRoot
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host 'Windows yonetici izni bekleniyor. Goreceginiz izin penceresinde Evet secin.'
    try {
        $scriptArgument = '"' + $PSCommandPath + '"'
        $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $scriptArgument)
        if ($AllowTestPorts) { $arguments += '-AllowTestPorts' }
        Start-Process -FilePath powershell.exe -ArgumentList $arguments -Verb RunAs -WindowStyle Normal -ErrorAction Stop
    } catch {
        Write-Host 'Windows yonetici izni verilemedi. UAC ayarlarini degistirmeniz gerekmez.' -ForegroundColor Yellow
        Read-Host 'Kapatmak icin Enter'
        exit 1
    }
    exit 0
}

Set-Location -LiteralPath $testRoot
Write-Host 'Localer - Gercek Windows sanal ag testi' -ForegroundColor Cyan
Write-Host 'Gecici bagdastirici ve TCP/UDP aktarimi test ediliyor. Bu pencereyi test bitene kadar kapatmayin.'
Write-Host 'UAC ayarlariniz ve mevcut guvenlik duvari kurallariniz degistirilmez.'
if ($AllowTestPorts) { Write-Host 'Gerekirse yalnizca test bagdastiricisi ve test portlari icin gecici izin eklenip test sonunda kaldirilir.' }
try {
    $testNode = (Get-Command node.exe -ErrorAction Stop).Source
    $nodeArguments = @((Join-Path $PSScriptRoot 'tun-smoke.js'))
    if ($AllowTestPorts) { $nodeArguments += '--allow-test-ports' }
    & $testNode @nodeArguments
    $testExitCode = $LASTEXITCODE
    Write-Host ''
    if ($testExitCode -eq 0) { Write-Host 'Test basarili. Sonuc artifacts\tun-smoke.json dosyasina kaydedildi.' -ForegroundColor Green }
    else { Write-Host 'Test tamamlanamadi. Ayrintilar artifacts\tun-smoke.json dosyasinda.' -ForegroundColor Yellow }
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    $testExitCode = 1
}
Read-Host 'Kapatmak icin Enter'
exit $testExitCode
