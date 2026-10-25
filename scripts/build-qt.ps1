param(
    [string]$QtRoot = '',
    [switch]$SkipPackage
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $QtRoot) { $QtRoot = Join-Path $projectRoot 'vendor\qt-sdk\6.10.3\mingw_64' }
$qtBin = Join-Path $QtRoot 'bin'
$compilerBin = Join-Path $projectRoot 'vendor\qt-sdk\Tools\mingw1310_64\bin'
$toolBin = Join-Path $projectRoot 'vendor\qt-tools\Scripts'
$env:PATH = $qtBin + ';' + $compilerBin + ';' + $toolBin + ';' + $env:PATH
$cmake = Join-Path $toolBin 'cmake.exe'
$buildRoot = Join-Path $projectRoot 'build\qt'
& $cmake -S $projectRoot -B $buildRoot -G Ninja '-DCMAKE_BUILD_TYPE=Release' ('-DCMAKE_PREFIX_PATH=' + $QtRoot) ('-DCMAKE_CXX_COMPILER=' + (Join-Path $compilerBin 'g++.exe')) ('-DCMAKE_MAKE_PROGRAM=' + (Join-Path $toolBin 'ninja.exe'))
if ($LASTEXITCODE -ne 0) { throw 'Qt configure failed.' }
& $cmake --build $buildRoot --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Qt build failed.' }
$env:LOCALER_ARTIFACT_DIR = Join-Path $projectRoot 'artifacts\qt'
& (Join-Path $toolBin 'ctest.exe') --test-dir $buildRoot --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Qt tests failed.' }
$engineDestination = Join-Path $buildRoot 'native\easytier'
New-Item -ItemType Directory -Force -Path $engineDestination | Out-Null
foreach ($name in @('easytier-core.exe','easytier-cli.exe','wintun.dll')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot ('vendor\easytier\easytier-windows-x86_64\' + $name)) -Destination $engineDestination -Force
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'vendor\packet-api\Packet.dll') -Destination $engineDestination -Force
if (-not $SkipPackage) { & (Join-Path $PSScriptRoot 'package-qt.ps1') -QtRoot $QtRoot }
