<#
.SYNOPSIS
    WindowsAiMic Build Script
    
.DESCRIPTION
    Builds all components and creates a distributable package.
    Optimized for Intel Core Ultra processors with AVX2/AVX-512 and NPU.
    
.PARAMETER Configuration
    Build configuration: Debug or Release (default: Release)
    
.PARAMETER SkipDriver
    Skip building the driver (useful if you're using a pre-built driver)
    
.PARAMETER Package
    Create a distributable zip package after building
    
.PARAMETER EnableOpenVINO
    Enable OpenVINO for NPU acceleration (requires OpenVINO installed)
    
.PARAMETER EnableAVX512
    Enable AVX-512 instructions (Intel Core Ultra supports this)
#>

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$SkipDriver,
    [switch]$Package,
    [switch]$EnableOpenVINO,
    [switch]$EnableAVX512
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $RootDir "build"

function Write-Step { param($msg) Write-Host "`n[*] $msg" -ForegroundColor Cyan }
function Write-Success { param($msg) Write-Host "[+] $msg" -ForegroundColor Green }
function Write-Fail { param($msg) Write-Host "[-] $msg" -ForegroundColor Red }

Write-Host @"

    ╔═══════════════════════════════════════════════════════════╗
    ║               WindowsAiMic Build Script                   ║
    ╚═══════════════════════════════════════════════════════════╝

"@ -ForegroundColor Magenta

# Check prerequisites
Write-Step "Checking prerequisites..."

# Check CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Fail "CMake not found. Please install CMake and add it to PATH."
    exit 1
}
Write-Success "CMake found: $($cmake.Source)"

# Check Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath
    Write-Success "Visual Studio found: $vsPath"
} else {
    Write-Fail "Visual Studio not found. Please install Visual Studio 2022 with C++ workload."
    exit 1
}

# Check for RNNoise
$rnnoiseDir = Join-Path $RootDir "engine\libs\rnnoise"
$rnnoiseSrc = Join-Path $rnnoiseDir "src"
if (-not (Test-Path (Join-Path $rnnoiseSrc "denoise.c"))) {
    Write-Step "RNNoise source not found. Cloning..."
    
    # Backup our CMakeLists.txt
    $cmakeBackup = Join-Path $rnnoiseDir "CMakeLists.txt.bak"
    $cmakeFile = Join-Path $rnnoiseDir "CMakeLists.txt"
    if (Test-Path $cmakeFile) {
        Copy-Item $cmakeFile $cmakeBackup
    }
    
    # Clone RNNoise
    Push-Location (Split-Path $rnnoiseDir -Parent)
    git clone https://github.com/xiph/rnnoise.git rnnoise_temp
    
    # Copy source files
    Copy-Item "rnnoise_temp\src\*" (Join-Path $rnnoiseDir "src") -Force -Recurse
    Copy-Item "rnnoise_temp\include\*" (Join-Path $rnnoiseDir "include") -Force -Recurse
    
    # Clean up
    Remove-Item "rnnoise_temp" -Recurse -Force
    
    # Restore our CMakeLists.txt
    if (Test-Path $cmakeBackup) {
        Copy-Item $cmakeBackup $cmakeFile -Force
        Remove-Item $cmakeBackup
    }
    
    Pop-Location
    Write-Success "RNNoise source cloned"
}

# Create build directory
Write-Step "Setting up build directory..."
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Configure CMake
Write-Step "Configuring CMake..."
Push-Location $BuildDir

# Build CMake arguments
$cmakeArgs = @(
    "..",
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_BUILD_TYPE=$Configuration",
    "-DBUILD_ENGINE=ON",
    "-DBUILD_APP=ON",
    "-DENABLE_AVX2=ON"  # Always enable AVX2 for Intel Core Ultra
)

if ($EnableAVX512) {
    Write-Host "  Enabling AVX-512 optimizations (Intel Core Ultra)" -ForegroundColor Cyan
    $cmakeArgs += "-DENABLE_AVX512=ON"
} else {
    $cmakeArgs += "-DENABLE_AVX512=OFF"
}

if ($EnableOpenVINO) {
    Write-Host "  Enabling OpenVINO for NPU acceleration" -ForegroundColor Cyan
    $cmakeArgs += "-DENABLE_OPENVINO=ON"
} else {
    $cmakeArgs += "-DENABLE_OPENVINO=OFF"
}

cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Fail "CMake configuration failed"
    Pop-Location
    exit 1
}

Write-Success "CMake configured"

# Build
Write-Step "Building ($Configuration)..."
cmake --build . --config $Configuration --parallel

if ($LASTEXITCODE -ne 0) {
    Write-Fail "Build failed"
    Pop-Location
    exit 1
}

Write-Success "Build completed"
Pop-Location

# Build driver (if not skipped)
if (-not $SkipDriver) {
    Write-Step "Building Virtual Audio Driver..."
    
    $driverDir = Join-Path $RootDir "driver"
    if (Test-Path (Join-Path $driverDir "VirtualAudioDriver.sln")) {
        Push-Location $driverDir
        
        # Find MSBuild
        $msbuild = & $vsWhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
        
        if ($msbuild) {
            & $msbuild VirtualAudioDriver.sln /p:Configuration=$Configuration /p:Platform=x64
            
            if ($LASTEXITCODE -eq 0) {
                Write-Success "Driver built"
            } else {
                Write-Fail "Driver build failed (continuing anyway)"
            }
        } else {
            Write-Fail "MSBuild not found (continuing without driver)"
        }
        
        Pop-Location
    } else {
        Write-Fail "Driver solution not found. Please clone Virtual-Audio-Driver to 'driver' folder."
    }
}

# Create package
if ($Package) {
    Write-Step "Creating distribution package..."
    
    $distDir = Join-Path $RootDir "dist"
    $packageDir = Join-Path $distDir "WindowsAiMic"
    
    # Clean and create dist directory
    if (Test-Path $packageDir) {
        Remove-Item $packageDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $packageDir -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $packageDir "drivers") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $packageDir "config") -Force | Out-Null
    
    # Copy executables
    $binDir = Join-Path $BuildDir "bin\$Configuration"
    if (-not (Test-Path $binDir)) {
        $binDir = Join-Path $BuildDir "bin"
    }
    Copy-Item "$binDir\*.exe" $packageDir -ErrorAction SilentlyContinue
    
    # Copy config
    Copy-Item (Join-Path $RootDir "config\default_config.json") (Join-Path $packageDir "config")
    
    # Copy scripts
    Copy-Item (Join-Path $ScriptDir "install.ps1") $packageDir
    
    # Copy driver files (if available)
    $driverOutput = Join-Path $RootDir "driver\x64\$Configuration"
    if (Test-Path $driverOutput) {
        Copy-Item "$driverOutput\*" (Join-Path $packageDir "drivers") -Recurse
    }
    
    # Create install batch file for easy launching
    @"
@echo off
echo Starting WindowsAiMic Installer...
echo.
echo This will install WindowsAiMic with:
echo - Virtual Audio Driver
echo - Audio Engine
echo - System Tray Application
echo.
echo Administrator privileges are required.
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0install.ps1"
pause
"@ | Out-File (Join-Path $packageDir "Install.bat") -Encoding ASCII
    
    # Create uninstall batch file
    @"
@echo off
echo Uninstalling WindowsAiMic...
powershell -ExecutionPolicy Bypass -File "%~dp0install.ps1" -Uninstall
pause
"@ | Out-File (Join-Path $packageDir "Uninstall.bat") -Encoding ASCII
    
    # Create README
    @"
WindowsAiMic - AI-Powered Virtual Microphone
============================================

INSTALLATION:
1. Double-click "Install.bat"
2. Click "Yes" when prompted for Administrator
3. Follow the on-screen instructions
4. Reboot if prompted

USAGE:
1. The tray icon will appear after installation
2. Right-click for settings and presets
3. In your apps, select "Virtual Mic Driver" as input

UNINSTALL:
Double-click "Uninstall.bat"

For issues, check:
- Device Manager (for driver status)
- Event Viewer (for engine logs)
"@ | Out-File (Join-Path $packageDir "README.txt") -Encoding ASCII
    
    # Create ZIP
    $zipPath = Join-Path $distDir "WindowsAiMic-$Configuration.zip"
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    
    Compress-Archive -Path $packageDir -DestinationPath $zipPath
    
    Write-Success "Package created: $zipPath"
}

Write-Host @"

╔═══════════════════════════════════════════════════════════════════════════╗
║                           Build Complete!                                  ║
╠═══════════════════════════════════════════════════════════════════════════╣
║                                                                            ║
║  Output: $BuildDir\bin\$Configuration
║                                                                            ║
║  To install: Run scripts\install.ps1 as Administrator                    ║
║                                                                            ║
"@ -ForegroundColor Green

if ($Package) {
    Write-Host "║  Package: dist\WindowsAiMic-$Configuration.zip" -ForegroundColor Green
    Write-Host "║" -ForegroundColor Green
}

Write-Host @"
╚═══════════════════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Green
