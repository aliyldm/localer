$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$application = Join-Path $projectRoot 'dist\Localer-0.3.0-Qt-Windows-x64\Localer.exe'
if (-not (Test-Path -LiteralPath $application)) { throw 'Qt package not found. Run scripts/build-qt.ps1 first.' }
# Interactive application: show its window and let Windows request its standard UAC consent.
Start-Process -FilePath $application -WorkingDirectory (Split-Path -Parent $application) -WindowStyle Normal
