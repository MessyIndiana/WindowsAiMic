<#
.SYNOPSIS
    Quick setup script - Downloads dependencies, builds, and installs in one go.
    
.DESCRIPTION
    This script is meant to be run on a fresh Windows machine.
    It will:
    1. Check/install prerequisites (Chocolatey, CMake, VS Build Tools)
    2. Clone required dependencies (RNNoise, Virtual-Audio-Driver)
    3. Build the project
    4. Run the installer
    
.NOTES
    Run this script from PowerShell as Administrator:
    Set-ExecutionPolicy Bypass -Scope Process -Force; .\quick-setup.ps1
#>

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

function Write-Step { param($msg) Write-Host "`n[*] $msg" -ForegroundColor Cyan }
function Write-Success { param($msg) Write-Host "[+] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "[!] $msg" -ForegroundColor Yellow }
function Write-Fail { param($msg) Write-Host "[-] $msg" -ForegroundColor Red }

Write-Host @"

    ╔═══════════════════════════════════════════════════════════╗
    ║            WindowsAiMic Quick Setup                       ║
    ║                                                           ║
    ║   This will download, build, and install everything       ║
    ╚═══════════════════════════════════════════════════════════╝

"@ -ForegroundColor Magenta

# Check if running as admin
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Fail "Please run this script as Administrator"
    Write-Host "Right-click PowerShell and select 'Run as Administrator'"
    exit 1
}

# ============================================================================
# Step 1: Check/Install Chocolatey
# ============================================================================
Write-Step "Checking for Chocolatey..."
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Warn "Chocolatey not found. Installing..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    
    # Refresh environment
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    
    Write-Success "Chocolatey installed"
} else {
    Write-Success "Chocolatey found"
}

# ============================================================================
# Step 2: Install build dependencies
# ============================================================================
Write-Step "Installing build dependencies..."

$packages = @(
    "cmake",
    "git",
    "visualstudio2022buildtools",
    "visualstudio2022-workload-vctools"
)

foreach ($pkg in $packages) {
    Write-Host "  Installing $pkg..."
    choco install $pkg -y --no-progress | Out-Null
}

# Refresh environment
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

Write-Success "Build dependencies installed"

# ============================================================================
# Step 3: Clone RNNoise
# ============================================================================
Write-Step "Setting up RNNoise..."

$rnnoiseDir = Join-Path $RootDir "engine\libs\rnnoise"
$rnnoiseSrcDir = Join-Path $rnnoiseDir "src"

if (-not (Test-Path $rnnoiseSrcDir)) {
    New-Item -ItemType Directory -Path $rnnoiseSrcDir -Force | Out-Null
}

if (-not (Test-Path (Join-Path $rnnoiseSrcDir "denoise.c"))) {
    Push-Location $RootDir
    
    # Clone to temp
    git clone --depth 1 https://github.com/xiph/rnnoise.git rnnoise_temp
    
    # Create target directories
    New-Item -ItemType Directory -Path (Join-Path $rnnoiseDir "src") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $rnnoiseDir "include") -Force | Out-Null
    
    # Copy files
    Copy-Item "rnnoise_temp\src\*" (Join-Path $rnnoiseDir "src") -Recurse -Force
    Copy-Item "rnnoise_temp\include\*" (Join-Path $rnnoiseDir "include") -Recurse -Force
    
    # Cleanup
    Remove-Item "rnnoise_temp" -Recurse -Force
    
    Pop-Location
    Write-Success "RNNoise source downloaded"
} else {
    Write-Success "RNNoise already present"
}

# ============================================================================
# Step 4: Clone/Setup Virtual Audio Driver
# ============================================================================
Write-Step "Setting up Virtual Audio Driver..."

$driverDir = Join-Path $RootDir "driver"

if (-not (Test-Path $driverDir)) {
    Push-Location $RootDir
    git clone --depth 1 https://github.com/VirtualDrivers/Virtual-Audio-Driver.git driver
    Pop-Location
    Write-Success "Virtual Audio Driver cloned"
} else {
    Write-Success "Virtual Audio Driver already present"
}

# ============================================================================
# Step 5: Build
# ============================================================================
Write-Step "Building WindowsAiMic..."

& (Join-Path $ScriptDir "build.ps1") -Configuration Release -Package

if ($LASTEXITCODE -ne 0) {
    Write-Fail "Build failed!"
    exit 1
}

Write-Success "Build completed"

# ============================================================================
# Step 6: Install
# ============================================================================
Write-Step "Running installer..."

$response = Read-Host "Build complete. Install now? (Y/N)"
if ($response -eq "Y" -or $response -eq "y") {
    & (Join-Path $ScriptDir "install.ps1")
}

Write-Host @"

╔═══════════════════════════════════════════════════════════════════════════╗
║                         Setup Complete!                                    ║
╠═══════════════════════════════════════════════════════════════════════════╣
║                                                                            ║
║  The distributable package is at:                                         ║
║  $RootDir\dist\WindowsAiMic-Release.zip
║                                                                            ║
║  Copy this ZIP to any Windows PC and run Install.bat                      ║
║                                                                            ║
╚═══════════════════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Green
