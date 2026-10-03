[CmdletBinding()]
param(
    [ValidateSet('all', 'd3d12', 'dxgi', 'ags')][string]$target = 'all',
    [switch]$RunTests
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$taskBuildRoot = $PSScriptRoot
$taskOutDir = Join-Path $taskBuildRoot 'proxy_build'

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $taskVswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $taskVswhere)) { throw 'Install Microsoft Visual Studio Build Tools with Desktop development with C++ and the Windows SDK.' }
    $taskVsPath = & $taskVswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $taskVsPath) { throw 'The MSVC x64 toolchain was not found.' }
    $taskVcvars = Join-Path $taskVsPath 'VC\Auxiliary\Build\vcvars64.bat'
    $taskEnvironment = & $env:ComSpec /d /c ('call "{0}" >nul && set' -f $taskVcvars)
    if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the MSVC x64 environment.' }
    foreach ($taskEnvironmentLine in $taskEnvironment) {
        if ($taskEnvironmentLine -match '^([^=]+)=(.*)$') {
            $taskEnvironmentName = $Matches[1]
            if ($taskEnvironmentName -notin @('HOME', 'CODEX_HOME')) {
                [Environment]::SetEnvironmentVariable($taskEnvironmentName, $Matches[2], 'Process')
            }
        }
    }
}
if ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64') { throw 'Use the x64 MSVC toolchain.' }
New-Item -ItemType Directory -Force -Path $taskOutDir | Out-Null

$taskTargets = if ($target -eq 'all') { @('d3d12', 'dxgi', 'ags') } else { @($target) }
$taskConfigurations = @{
    d3d12 = @('D3D12Proxy', 'd3d12_proxy', 'd3d12')
    dxgi = @('DXGIProxy', 'dxgi_proxy', 'dxgi')
    ags = @('AGSProxy', 'amd_ags_x64', 'amd_ags_x64')
}
$taskCompilerOptions = @('/nologo', '/std:c++17', '/EHsc', '/O2', '/MT', '/W4', '/utf-8', '/GS', '/guard:cf', '/DWIN32_LEAN_AND_MEAN', '/DNOMINMAX')
$taskLinkerOptions = @('/INCREMENTAL:NO', '/Brepro', '/OPT:REF', '/OPT:ICF', '/DYNAMICBASE', '/NXCOMPAT', '/HIGHENTROPYVA', '/guard:cf', '/CETCOMPAT')
foreach ($taskTargetName in $taskTargets) {
    $taskConfiguration = $taskConfigurations[$taskTargetName]
    $taskBase = $taskConfiguration[0]
    $taskDef = Join-Path $taskBuildRoot ('src\{0}.def' -f $taskConfiguration[1])
    $taskSource = Join-Path $taskBuildRoot ('src\{0}.cpp' -f $taskBase)
    $taskAsm = Join-Path $taskBuildRoot ('src\{0}Stubs.asm' -f $taskBase)
    $taskObject = Join-Path $taskOutDir ($taskBase + '.obj')
    $taskAsmObject = Join-Path $taskOutDir ($taskBase + 'Stubs.obj')
    $taskDll = Join-Path $taskOutDir ($taskConfiguration[2] + '.dll')
    Write-Host ('Building {0} with MSVC x64' -f $taskConfiguration[2])
    & ml64.exe /nologo /c ('/Fo' + $taskAsmObject) $taskAsm
    if ($LASTEXITCODE -ne 0) { throw ('Assembly failed: ' + $taskAsm) }
    & cl.exe @taskCompilerOptions /LD $taskSource $taskAsmObject ('/Fo' + $taskObject) ('/Fe' + $taskDll) /link ('/DEF:' + $taskDef) ('/IMPLIB:' + (Join-Path $taskOutDir ($taskConfiguration[2] + '.lib'))) @taskLinkerOptions
    if ($LASTEXITCODE -ne 0) { throw ('Compilation failed: ' + $taskSource) }
}
if ($RunTests) {
    & (Join-Path $taskBuildRoot 'tests\VerifyAGSHeaders.ps1')
    foreach ($taskTestSource in (Get-ChildItem -LiteralPath (Join-Path $taskBuildRoot 'tests') -Filter '*.cpp' -File | Where-Object { $_.Name -like '*_tests.cpp' -or $_.Name -in @('AGSPolicyTests.cpp', 'AGSTrampolineTests.cpp') } | Sort-Object Name)) {
        $taskTestExe = Join-Path $taskOutDir ($taskTestSource.BaseName + '.exe')
        $taskTestObjects = @()
        if ($taskTestSource.BaseName -eq 'proxy_asm_tests') {
            foreach ($taskAsmTest in @(@('src\D3D12ProxyStubs.asm', 'proxy_test_stubs.obj'), @('tests\proxy_mock_resolver.asm', 'proxy_test_resolver.obj'))) {
                $taskAsmTestObject = Join-Path $taskOutDir $taskAsmTest[1]
                & ml64.exe /nologo /c ('/Fo' + $taskAsmTestObject) (Join-Path $taskBuildRoot $taskAsmTest[0])
                if ($LASTEXITCODE -ne 0) { throw 'Assembly ABI test compilation failed.' }
                $taskTestObjects += $taskAsmTestObject
            }
        }
        if ($taskTestSource.BaseName -eq 'AGSTrampolineTests') {
            foreach ($taskAsmTest in @(@('src\AGSProxyStubs.asm', 'ags_test_stubs.obj'), @('tests\AGSClobberResolver.asm', 'ags_test_resolver.obj'))) {
                $taskAsmTestObject = Join-Path $taskOutDir $taskAsmTest[1]
                & ml64.exe /nologo /c ('/Fo' + $taskAsmTestObject) (Join-Path $taskBuildRoot $taskAsmTest[0])
                if ($LASTEXITCODE -ne 0) { throw 'AGS assembly ABI test compilation failed.' }
                $taskTestObjects += $taskAsmTestObject
            }
        }
        & cl.exe @taskCompilerOptions $taskTestSource.FullName @taskTestObjects ('/Fo' + (Join-Path $taskOutDir ($taskTestSource.BaseName + '.obj'))) ('/Fe' + $taskTestExe) /link @taskLinkerOptions
        if ($LASTEXITCODE -ne 0) { throw ('Test compilation failed: ' + $taskTestSource.Name) }
        & $taskTestExe
        if ($LASTEXITCODE -ne 0) { throw ('Tests failed: ' + $taskTestSource.Name) }
        if ($taskTestSource.BaseName -eq 'proxy_forwarding_tests') {
            foreach ($taskScenario in @('--missing-module', '--missing-symbol', '--bad-system-directory')) {
                & $taskTestExe $taskScenario
                if ($LASTEXITCODE -ne 0) { throw ('Proxy error scenario failed: ' + $taskScenario) }
            }
        }
    }
    $taskInstallerTests = Join-Path $taskBuildRoot 'tests\installer_tests.ps1'
    if (Test-Path -LiteralPath $taskInstallerTests) { & $taskInstallerTests }
}
foreach ($taskTargetName in $taskTargets) {
    Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $taskOutDir ($taskConfigurations[$taskTargetName][2] + '.dll')) | Select-Object Hash, Path
}
