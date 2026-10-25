param([string]$QtRoot = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $QtRoot) { $QtRoot = Join-Path $projectRoot 'vendor\qt-sdk\6.10.3\mingw_64' }
$compilerRoot = Join-Path $projectRoot 'vendor\qt-sdk\Tools\mingw1310_64'
$env:PATH = (Join-Path $QtRoot 'bin') + ';' + (Join-Path $compilerRoot 'bin') + ';' + $env:PATH
$stage = Join-Path $projectRoot 'dist\Localer-0.3.0-Qt-Windows-x64'
$build = Join-Path $projectRoot 'build\qt\native'
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'Localer.exe') -Destination $stage -Force
& (Join-Path $QtRoot 'bin\windeployqt.exe') --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-compiler-runtime --dir $stage (Join-Path $stage 'Localer.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }
foreach ($dll in @('libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll')) { Copy-Item -LiteralPath (Join-Path $compilerRoot ('bin\'+$dll)) -Destination $stage -Force }
$engine = Join-Path $stage 'easytier'
$licenses = Join-Path $stage 'licenses'
$sources = Join-Path $stage 'sources'
New-Item -ItemType Directory -Force -Path $engine,$licenses,$sources | Out-Null
foreach ($dll in @('easytier-core.exe','easytier-cli.exe','wintun.dll')) { Copy-Item -LiteralPath (Join-Path $projectRoot ('vendor\easytier\easytier-windows-x86_64\'+$dll)) -Destination $engine -Force }
Copy-Item -LiteralPath (Join-Path $projectRoot 'vendor\packet-api\Packet.dll') -Destination $engine -Force
Copy-Item -Path (Join-Path $projectRoot 'licenses\*') -Destination $licenses -Recurse -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'vendor\qt-sources\6.10.3\Src\qtbase\LICENSES') -Destination (Join-Path $licenses 'Qt') -Recurse -Force
foreach ($part in @('gcc','mingw-w64','winpthreads')) { Copy-Item -LiteralPath (Join-Path $compilerRoot ('licenses\'+$part)) -Destination (Join-Path $licenses $part) -Recurse -Force }
Copy-Item -LiteralPath (Join-Path $projectRoot 'vendor\EasyTier-v2.6.4-source.zip') -Destination $sources -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'vendor\qt-source-archives\qtbase-6.10.3-source.7z') -Destination $sources -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'LICENSE') -Destination $stage -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'native\THIRD-PARTY-NOTICES.md') -Destination $stage -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'native\README.md') -Destination (Join-Path $stage 'OKU-BENI.md') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'native\BASLA.txt') -Destination $stage -Force
$sourcePayload = Join-Path $projectRoot 'artifacts\qt\source-payload'
New-Item -ItemType Directory -Force -Path $sourcePayload | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt'),(Join-Path $projectRoot 'LICENSE') -Destination $sourcePayload -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'native'),(Join-Path $projectRoot 'assets') -Destination $sourcePayload -Recurse -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'licenses') -Destination $sourcePayload -Recurse -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'Localer-Ag-Testi.cmd') -Destination $sourcePayload -Force
New-Item -ItemType Directory -Force -Path (Join-Path $sourcePayload 'scripts') | Out-Null
foreach ($script in @('setup-qt.ps1','setup-qt-source.ps1','setup-engine.ps1','setup-packet-api.ps1','build-qt.ps1','package-qt.ps1','start-qt.ps1','test-qt.ps1','start-qt-tun-test.ps1')) { Copy-Item -LiteralPath (Join-Path $PSScriptRoot $script) -Destination (Join-Path $sourcePayload 'scripts') -Force }
if (Test-Path -LiteralPath (Join-Path $projectRoot 'native\TEST-RESULTS.md')) { Copy-Item -LiteralPath (Join-Path $projectRoot 'native\TEST-RESULTS.md') -Destination (Join-Path $stage 'TEST-SONUCLARI.md') -Force }
Compress-Archive -Path (Join-Path $sourcePayload '*') -DestinationPath (Join-Path $sources 'Localer-0.3.0-source.zip') -Force
$archive = Join-Path $projectRoot 'dist\Localer-0.3.0-Qt-Windows-x64.zip'
Compress-Archive -LiteralPath $stage -DestinationPath $archive -CompressionLevel Optimal -Force
$hash = Get-FileHash -LiteralPath $archive -Algorithm SHA256
[System.IO.File]::WriteAllText((Join-Path $projectRoot 'dist\Localer-0.3.0-Qt-SHA256.txt'),($hash.Hash.ToLowerInvariant()+'  '+[System.IO.Path]::GetFileName($archive)+[Environment]::NewLine))
Write-Output ('Qt package ready: '+$archive)
Get-Item -LiteralPath $archive | Select-Object FullName,Length
