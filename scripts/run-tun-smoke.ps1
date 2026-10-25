$ErrorActionPreference = 'Stop'
$testRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $testRoot
$testNode = (Get-Command node.exe -ErrorAction Stop).Source
& $testNode (Join-Path $PSScriptRoot 'tun-smoke.js') *> (Join-Path $testRoot 'artifacts\tun-smoke-output.txt')
exit $LASTEXITCODE
