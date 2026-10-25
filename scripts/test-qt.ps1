$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$env:PATH=(Join-Path $projectRoot 'vendor\qt-sdk\6.10.3\mingw_64\bin')+';'+(Join-Path $projectRoot 'vendor\qt-sdk\Tools\mingw1310_64\bin')+';'+$env:PATH
$env:LOCALER_ARTIFACT_DIR=Join-Path $projectRoot 'artifacts\qt'
& (Join-Path $projectRoot 'vendor\qt-tools\Scripts\ctest.exe') --test-dir (Join-Path $projectRoot 'build\qt') --output-on-failure
exit $LASTEXITCODE
