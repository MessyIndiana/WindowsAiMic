<#
.SYNOPSIS
    Setup Virtual Audio Driver Build Environment
    
.DESCRIPTION
    Downloads and sets up the Windows Driver Kit (WDK) for building
    the virtual audio driver. This is a complex process that requires
    matching versions of Visual Studio, Windows SDK, and WDK.
    
    For most users, we recommend using a pre-built driver like VB-Cable.
#>

param(
    [switch]$UseVBCable,
    [switch]$SkipDriver
)

$ErrorActionPreference = "Stop"

Write-Host @"

╔═══════════════════════════════════════════════════════════╗
║          Virtual Audio Driver Setup                        ║
╚═══════════════════════════════════════════════════════════╝

"@ -ForegroundColor Cyan

# Option 1: Use VB-Cable (easiest)
if ($UseVBCable -or $SkipDriver) {
    Write-Host @"
RECOMMENDED: Use VB-Cable Virtual Audio Device

VB-Cable is a free, pre-signed virtual audio driver that works perfectly
with WindowsAiMic. No driver building required!

Steps:
1. Download VB-Cable from: https://vb-audio.com/Cable/
2. Extract and run VBCABLE_Setup_x64.exe as Administrator
3. Reboot your PC
4. In WindowsAiMic config, set:
   - Input: Your physical microphone
   - Output: "CABLE Input (VB-Audio Virtual Cable)"
5. In your apps (Teams, etc), select:
   - "CABLE Output (VB-Audio Virtual Cable)" as microphone

"@ -ForegroundColor Yellow
    
    $response = Read-Host "Open VB-Cable download page in browser? (Y/N)"
    if ($response -eq "Y" -or $response -eq "y") {
        Start-Process "https://vb-audio.com/Cable/"
    }
    
    exit 0
}

# Option 2: Build custom driver (advanced)
Write-Host "Building custom virtual audio driver requires:" -ForegroundColor Cyan
Write-Host "  - Visual Studio 2022 with C++ workload"
Write-Host "  - Windows SDK (matching your Windows version)"
Write-Host "  - Windows Driver Kit (WDK) - same version as SDK"
Write-Host ""
Write-Host "This is complex and time-consuming. For personal use," -ForegroundColor Yellow
Write-Host "we strongly recommend using VB-Cable instead." -ForegroundColor Yellow
Write-Host ""

$proceed = Read-Host "Continue with driver build setup? (Y/N)"
if ($proceed -ne "Y" -and $proceed -ne "y") {
    Write-Host "Run this script with -UseVBCable for the easier option."
    exit 0
}

# Check for Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Host "Visual Studio 2022 not found. Please install it first." -ForegroundColor Red
    exit 1
}

$vsPath = & $vsWhere -latest -property installationPath
Write-Host "Visual Studio found: $vsPath" -ForegroundColor Green

# Check for Windows SDK
$sdkPath = "${env:ProgramFiles(x86)}\Windows Kits\10"
if (-not (Test-Path $sdkPath)) {
    Write-Host "Windows SDK not found. Installing via Visual Studio Installer..." -ForegroundColor Yellow
    
    $vsInstaller = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vs_installer.exe"
    Start-Process $vsInstaller -ArgumentList "modify --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --quiet" -Wait
}

# Check for WDK
$wdkPath = "${env:ProgramFiles(x86)}\Windows Kits\10\Include"
$wdkFound = $false

if (Test-Path $wdkPath) {
    $wdkVersions = Get-ChildItem $wdkPath -Directory | Where-Object { Test-Path (Join-Path $_.FullName "km\wdm.h") }
    if ($wdkVersions) {
        $wdkFound = $true
        Write-Host "WDK found: $($wdkVersions[-1].Name)" -ForegroundColor Green
    }
}

if (-not $wdkFound) {
    Write-Host "Windows Driver Kit (WDK) not found. Downloading..." -ForegroundColor Yellow
    
    # Download WDK installer
    $wdkUrl = "https://go.microsoft.com/fwlink/?linkid=2196230"  # WDK for Windows 11
    $wdkInstaller = "$env:TEMP\wdksetup.exe"
    
    Invoke-WebRequest -Uri $wdkUrl -OutFile $wdkInstaller
    
    Write-Host "Installing WDK... This may take a while." -ForegroundColor Cyan
    Start-Process $wdkInstaller -ArgumentList "/quiet /norestart" -Wait
    
    Write-Host "WDK installed." -ForegroundColor Green
}

# Build the driver
Write-Host ""
Write-Host "Building virtual audio driver..." -ForegroundColor Cyan

$driverDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$driverDir = Join-Path (Split-Path -Parent $driverDir) "driver"

# Create driver project if needed
$vcxproj = Join-Path $driverDir "WindowsAiMicDriver.vcxproj"
if (-not (Test-Path $vcxproj)) {
    Write-Host "Driver project file not found. Creating..." -ForegroundColor Yellow
    
    # This would need a full vcxproj file - complex to generate
    Write-Host "Please create the driver project in Visual Studio." -ForegroundColor Red
    Write-Host "For now, use VB-Cable as your virtual audio device." -ForegroundColor Yellow
    exit 1
}

# Build with MSBuild
$msbuild = & $vsWhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1

if ($msbuild) {
    Push-Location $driverDir
    & $msbuild WindowsAiMicDriver.vcxproj /p:Configuration=Release /p:Platform=x64
    Pop-Location
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Driver built successfully!" -ForegroundColor Green
    } else {
        Write-Host "Driver build failed. Use VB-Cable instead." -ForegroundColor Red
    }
} else {
    Write-Host "MSBuild not found. Cannot build driver." -ForegroundColor Red
}
