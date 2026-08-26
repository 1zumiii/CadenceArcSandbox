[CmdletBinding()]
param(
    [string]$Filter = "CadenceArc",
    [string]$EngineRoot,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectPath = Join-Path $ProjectRoot "CadenceArcSandbox.uproject"

if (-not (Test-Path -LiteralPath $ProjectPath -PathType Leaf)) {
    throw "CadenceArcSandbox.uproject was not found at '$ProjectPath'."
}

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $ProjectDescriptor = Get-Content -LiteralPath $ProjectPath -Raw | ConvertFrom-Json
    $EngineAssociation = [string]$ProjectDescriptor.EngineAssociation

    if ([string]::IsNullOrWhiteSpace($EngineAssociation)) {
        throw "The project does not define an EngineAssociation. Pass -EngineRoot explicitly."
    }

    $LauncherRegistryPath = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$EngineAssociation"
    if (Test-Path -LiteralPath $LauncherRegistryPath) {
        $EngineRoot = (Get-ItemProperty -LiteralPath $LauncherRegistryPath).InstalledDirectory
    }
}

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    throw "Unable to locate the Unreal Engine installation. Pass -EngineRoot explicitly."
}

$BuildScript = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$EditorCommand = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

if (-not (Test-Path -LiteralPath $BuildScript -PathType Leaf)) {
    throw "Unreal Build Tool script was not found at '$BuildScript'."
}
if (-not (Test-Path -LiteralPath $EditorCommand -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe was not found at '$EditorCommand'."
}

if (-not $SkipBuild) {
    $RunningEditor = Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
    if ($RunningEditor) {
        throw "Close Unreal Editor before running the reliable cold-build test command."
    }

    Write-Host "=== Building CadenceArcSandboxEditor ===" -ForegroundColor Cyan
    & $BuildScript `
        "CadenceArcSandboxEditor" `
        "Win64" `
        "Development" `
        $ProjectPath `
        "-WaitMutex" `
        "-NoHotReloadFromIDE"

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed with exit code $LASTEXITCODE." -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

Write-Host "=== Running Unreal Automation Tests: $Filter ===" -ForegroundColor Cyan

$EditorArguments = @(
    $ProjectPath
    "-Unattended"
    "-NullRHI"
    "-NoSound"
    "-NoSplash"
    "-stdout"
    "-AllowStdOutLogVerbosity"
    "-culture=en"
    "-ExecCmds=Automation RunTests $Filter;Quit"
    "-TestExit=Automation Test Queue Empty"
    "-Log"
)

& $EditorCommand @EditorArguments 2>&1 | ForEach-Object {
    $Line = [string]$_
    $IsSummary =
        $Line -match "LogAutomationCommandLine: Display: (Found|\*{4} TEST COMPLETE)" -or
        $Line -match "LogAutomationController: Display: Test Completed" -or
        $Line -match "LogAutomation(Test|Controller|CommandLine): Error" -or
        $Line -match "Fatal error"

    if ($IsSummary) {
        Write-Host $Line
    }
}
$TestExitCode = $LASTEXITCODE
$FullLogPath = Join-Path $ProjectRoot "Saved\Logs\CadenceArcSandbox.log"

Write-Host "Full Unreal log: $FullLogPath"

if ($TestExitCode -eq 0) {
    Write-Host "=== CadenceArc tests passed ===" -ForegroundColor Green
} else {
    Write-Host "=== CadenceArc tests failed with exit code $TestExitCode ===" -ForegroundColor Red
}

exit $TestExitCode
