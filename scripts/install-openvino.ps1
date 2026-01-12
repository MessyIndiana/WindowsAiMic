<#
.SYNOPSIS
    Install OpenVINO for Intel NPU acceleration
    
.DESCRIPTION
    Downloads and installs Intel OpenVINO toolkit to enable
    AI processing on the Intel Core Ultra's NPU.
#>

$ErrorActionPreference = "Stop"

Write-Host @"

╔═══════════════════════════════════════════════════════════╗
║          OpenVINO NPU Acceleration Setup                  ║
║                                                           ║
║   This will enable AI processing on the Intel NPU        ║
║   (Neural Processing Unit) in your Core Ultra 7 165U     ║
╚═══════════════════════════════════════════════════════════╝

"@ -ForegroundColor Cyan

# Check if running as admin
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "Please run as Administrator" -ForegroundColor Red
    exit 1
}

# Check for Intel Core Ultra
$cpu = Get-WmiObject Win32_Processor | Select-Object -First 1
Write-Host "Detected CPU: $($cpu.Name)"

if ($cpu.Name -notmatch "Core.*Ultra") {
    Write-Host ""
    Write-Host "Warning: This doesn't appear to be an Intel Core Ultra processor." -ForegroundColor Yellow
    Write-Host "NPU acceleration may not be available." -ForegroundColor Yellow
    Write-Host ""
    $continue = Read-Host "Continue anyway? (Y/N)"
    if ($continue -ne "Y" -and $continue -ne "y") {
        exit 0
    }
}

# Download OpenVINO
Write-Host ""
Write-Host "Downloading OpenVINO..." -ForegroundColor Cyan

$openVinoVersion = "2024.0.0"
$downloadUrl = "https://storage.openvinotoolkit.org/repositories/openvino/packages/$openVinoVersion/windows/w_openvino_toolkit_windows_$openVinoVersion.zip"
$tempDir = "$env:TEMP\openvino_install"
$zipPath = "$tempDir\openvino.zip"

if (-not (Test-Path $tempDir)) {
    New-Item -ItemType Directory -Path $tempDir | Out-Null
}

# Use BITS for reliable download
try {
    Start-BitsTransfer -Source $downloadUrl -Destination $zipPath
} catch {
    # Fallback to Invoke-WebRequest
    Invoke-WebRequest -Uri $downloadUrl -OutFile $zipPath -UseBasicParsing
}

Write-Host "Extracting OpenVINO..." -ForegroundColor Cyan

$installPath = "$env:ProgramFiles\Intel\OpenVINO"
if (-not (Test-Path $installPath)) {
    New-Item -ItemType Directory -Path $installPath | Out-Null
}

Expand-Archive -Path $zipPath -DestinationPath $installPath -Force

# Set environment variables
Write-Host "Setting environment variables..." -ForegroundColor Cyan

$openVinoPath = Get-ChildItem $installPath -Directory | Select-Object -First 1
$runtimePath = Join-Path $openVinoPath.FullName "runtime\bin\intel64\Release"
$libPath = Join-Path $openVinoPath.FullName "runtime\lib\intel64\Release"

# Add to PATH
$currentPath = [Environment]::GetEnvironmentVariable("PATH", "Machine")
if ($currentPath -notmatch "OpenVINO") {
    [Environment]::SetEnvironmentVariable("PATH", "$currentPath;$runtimePath", "Machine")
}

# Set OpenVINO-specific variables
[Environment]::SetEnvironmentVariable("OPENVINO_DIR", $openVinoPath.FullName, "Machine")
[Environment]::SetEnvironmentVariable("OpenVINO_DIR", $openVinoPath.FullName, "Machine")

# Install NPU driver if available
Write-Host ""
Write-Host "Checking for NPU driver..." -ForegroundColor Cyan

$npuDriver = Get-WmiObject Win32_PnPEntity | Where-Object { $_.Name -match "NPU|Neural" }
if ($npuDriver) {
    Write-Host "NPU device found: $($npuDriver.Name)" -ForegroundColor Green
} else {
    Write-Host "NPU device not detected in Device Manager." -ForegroundColor Yellow
    Write-Host "Intel NPU driver might need to be installed separately." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Download from: https://www.intel.com/content/www/us/en/download/794734/intel-npu-driver-windows.html" -ForegroundColor Cyan
}

# Clean up
Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host @"
╔═══════════════════════════════════════════════════════════════════════════╗
║                     OpenVINO Installation Complete!                        ║
╠═══════════════════════════════════════════════════════════════════════════╣
║                                                                            ║
║  OpenVINO has been installed to:                                          ║
║  $installPath
║                                                                            ║
║  To use NPU acceleration in WindowsAiMic:                                 ║
║  1. Rebuild with: .\scripts\build.ps1 -ENABLE_OPENVINO                    ║
║  2. Reinstall:    .\scripts\install.ps1                                   ║
║                                                                            ║
║  NOTE: You may need to restart your computer for PATH changes.            ║
║                                                                            ║
╚═══════════════════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Green

Read-Host "Press Enter to exit"
