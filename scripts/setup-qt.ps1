param([string]$PythonExe = 'python')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$toolsRoot = Join-Path $projectRoot 'vendor\qt-tools'
if (-not (Test-Path -LiteralPath (Join-Path $toolsRoot 'Scripts\python.exe'))) {
    & $PythonExe -m venv $toolsRoot
    if ($LASTEXITCODE -ne 0) { throw 'Python 3.10 or newer is required for build setup.' }
}
$toolPython = Join-Path $toolsRoot 'Scripts\python.exe'
& $toolPython -m pip install --disable-pip-version-check 'aqtinstall==3.3.0' 'cmake==4.4.4' 'ninja==1.13.2'
if ($LASTEXITCODE -ne 0) { throw 'Build tools installation failed.' }
$sdk = Join-Path $projectRoot 'vendor\qt-sdk'
& $toolPython -m aqt install-qt windows desktop 6.10.3 win64_mingw --archives qtbase -O $sdk
if ($LASTEXITCODE -ne 0) { throw 'Qt installation failed.' }
& $toolPython -m aqt install-tool windows desktop tools_mingw1310 qt.tools.win64_mingw1310 -O $sdk
if ($LASTEXITCODE -ne 0) { throw 'Compiler installation failed.' }
