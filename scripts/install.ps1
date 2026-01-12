#Requires -RunAsAdministrator
<#
.SYNOPSIS
    WindowsAiMic One-Click Installer
    
.DESCRIPTION
    Installs WindowsAiMic with all components:
    - Virtual Audio Driver
    - Audio Engine
    - Tray Application
    - Autostart configuration
    
.NOTES
    Must be run as Administrator
    For personal use only (uses test signing)
#>

param(
    [switch]$Uninstall,
    [switch]$Silent
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Configuration
$AppName = "WindowsAiMic"
$InstallPath = "$env:ProgramFiles\$AppName"
$ConfigPath = "$env:APPDATA\$AppName"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

# Colors for output
function Write-Step { param($msg) Write-Host "`n[*] $msg" -ForegroundColor Cyan }
function Write-Success { param($msg) Write-Host "[+] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "[!] $msg" -ForegroundColor Yellow }
function Write-Fail { param($msg) Write-Host "[-] $msg" -ForegroundColor Red }

function Show-Banner {
    Write-Host @"

    ╔═══════════════════════════════════════════════════════════╗
    ║               WindowsAiMic Installer                      ║
    ║         AI-Powered Virtual Microphone Enhancement         ║
    ╚═══════════════════════════════════════════════════════════╝

"@ -ForegroundColor Magenta
}

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Enable-TestSigning {
    Write-Step "Checking test signing mode..."
    
    $bcdedit = bcdedit /enum | Select-String "testsigning"
    if ($bcdedit -match "Yes") {
        Write-Success "Test signing already enabled"
        return $true
    }
    
    Write-Warn "Test signing is not enabled. This is required for the virtual audio driver."
    
    if (-not $Silent) {
        $response = Read-Host "Enable test signing? This requires a reboot. (Y/N)"
        if ($response -ne "Y" -and $response -ne "y") {
            Write-Fail "Cannot install without test signing. Aborting."
            return $false
        }
    }
    
    Write-Step "Enabling test signing..."
    bcdedit /set testsigning on
    
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "Failed to enable test signing. You may need to disable Secure Boot in BIOS."
        return $false
    }
    
    Write-Success "Test signing enabled. A reboot is required."
    $script:NeedsReboot = $true
    return $true
}

function Test-VBCableInstalled {
    # Check if VB-Cable is already installed
    $devices = Get-CimInstance Win32_SoundDevice | Where-Object { $_.Name -like "*VB-Audio*" -or $_.Name -like "*CABLE*" }
    return $devices.Count -gt 0
}

function Install-VirtualDriver {
    Write-Step "Setting up Virtual Audio Device..."
    
    # Check if VB-Cable is already installed
    if (Test-VBCableInstalled) {
        Write-Success "VB-Cable virtual audio device already installed"
        return $true
    }
    
    # Try bundled driver first
    $driverPath = Join-Path $InstallPath "drivers"
    $infFile = Join-Path $driverPath "WindowsAiMicDriver.inf"
    
    if (Test-Path $infFile) {
        Write-Host "  Attempting to install bundled driver..." -ForegroundColor Gray
        $result = pnputil /add-driver $infFile /install 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Success "WindowsAiMic Virtual Audio Driver installed"
            return $true
        } else {
            Write-Warn "Bundled driver installation failed (this is normal without WDK signing)"
        }
    }
    
    # Offer VB-Cable installation
    Write-Host ""
    Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Yellow
    Write-Host "║  Virtual Audio Driver Required                                  ║" -ForegroundColor Yellow
    Write-Host "╠════════════════════════════════════════════════════════════════╣" -ForegroundColor Yellow
    Write-Host "║                                                                 ║" -ForegroundColor Yellow
    Write-Host "║  WindowsAiMic needs a virtual audio device to work.            ║" -ForegroundColor Yellow
    Write-Host "║  The easiest option is VB-Cable (FREE):                        ║" -ForegroundColor Yellow
    Write-Host "║                                                                 ║" -ForegroundColor Yellow
    Write-Host "║  1. Download from: https://vb-audio.com/Cable/                 ║" -ForegroundColor Yellow
    Write-Host "║  2. Run VBCABLE_Setup_x64.exe as Administrator                 ║" -ForegroundColor Yellow
    Write-Host "║  3. Reboot                                                     ║" -ForegroundColor Yellow
    Write-Host "║                                                                 ║" -ForegroundColor Yellow
    Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Yellow
    Write-Host ""
    
    if (-not $Silent) {
        $response = Read-Host "Download VB-Cable now? (Y/N)"
        if ($response -eq "Y" -or $response -eq "y") {
            # Download VB-Cable
            $vbcableUrl = "https://download.vb-audio.com/Download_CABLE/VBCABLE_Driver_Pack43.zip"
            $vbcableZip = "$env:TEMP\VBCable.zip"
            $vbcableDir = "$env:TEMP\VBCable"
            
            Write-Host "  Downloading VB-Cable..." -ForegroundColor Cyan
            try {
                Invoke-WebRequest -Uri $vbcableUrl -OutFile $vbcableZip -UseBasicParsing
                
                # Extract
                if (Test-Path $vbcableDir) { Remove-Item $vbcableDir -Recurse -Force }
                Expand-Archive -Path $vbcableZip -DestinationPath $vbcableDir -Force
                
                # Run installer
                $installer = Get-ChildItem -Path $vbcableDir -Filter "VBCABLE_Setup_x64.exe" -Recurse | Select-Object -First 1
                if ($installer) {
                    Write-Host "  Running VB-Cable installer..." -ForegroundColor Cyan
                    Start-Process -FilePath $installer.FullName -Wait
                    
                    if (Test-VBCableInstalled) {
                        Write-Success "VB-Cable installed successfully"
                        $script:NeedsReboot = $true
                        return $true
                    }
                }
            } catch {
                Write-Warn "Failed to download VB-Cable automatically."
                Write-Host "  Please download manually from: https://vb-audio.com/Cable/" -ForegroundColor Yellow
            }
        }
    }
    
    Write-Warn "Virtual audio driver not installed. Please install VB-Cable manually."
    Write-Host "  Download: https://vb-audio.com/Cable/" -ForegroundColor Yellow
    
    return $true  # Continue with rest of install
}

function Uninstall-VirtualDriver {
    Write-Step "Removing Virtual Audio Driver..."
    
    # Find and remove the driver
    $drivers = pnputil /enum-drivers | Select-String -Pattern "VirtualAudio" -Context 0,5
    
    foreach ($driver in $drivers) {
        $oemInf = ($driver.Context.PostContext | Select-String "oem\d+\.inf").Matches.Value
        if ($oemInf) {
            pnputil /delete-driver $oemInf /uninstall /force 2>&1 | Out-Null
        }
    }
    
    Write-Success "Driver removed"
}

function Install-Application {
    Write-Step "Installing application files..."
    
    # Create install directory
    if (-not (Test-Path $InstallPath)) {
        New-Item -ItemType Directory -Path $InstallPath -Force | Out-Null
    }
    
    # Create subdirectories
    @("drivers", "config") | ForEach-Object {
        $dir = Join-Path $InstallPath $_
        if (-not (Test-Path $dir)) {
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
        }
    }
    
    # Determine source locations
    # Priority 1: Same directory as install script (pre-built package)
    # Priority 2: Build output directory
    
    $foundExe = $false
    
    # Check if we're running from a pre-built package (exe files in same dir as script)
    $packageExes = Get-ChildItem -Path $ScriptDir -Filter "*.exe" -ErrorAction SilentlyContinue
    if ($packageExes) {
        Write-Host "  Found pre-built executables in package" -ForegroundColor Gray
        Copy-Item "$ScriptDir\*.exe" $InstallPath -Force -ErrorAction SilentlyContinue
        $foundExe = $true
    }
    
    # Also check parent directory (if install.ps1 is in a subdirectory)
    if (-not $foundExe) {
        $parentDir = Split-Path -Parent $ScriptDir
        $parentExes = Get-ChildItem -Path $parentDir -Filter "*.exe" -ErrorAction SilentlyContinue
        if ($parentExes) {
            Write-Host "  Found pre-built executables in parent directory" -ForegroundColor Gray
            Copy-Item "$parentDir\*.exe" $InstallPath -Force -ErrorAction SilentlyContinue
            $foundExe = $true
        }
    }
    
    # Fall back to build directory
    if (-not $foundExe) {
        $buildDir = Join-Path $RootDir "build\bin\Release"
        if (-not (Test-Path $buildDir)) {
            $buildDir = Join-Path $RootDir "build\bin"
        }
        
        if (Test-Path $buildDir) {
            Copy-Item "$buildDir\*.exe" $InstallPath -Force -ErrorAction SilentlyContinue
            $foundExe = $true
            Write-Host "  Found executables in build directory" -ForegroundColor Gray
        }
    }
    
    if ($foundExe) {
        Write-Success "Application files copied"
    } else {
        Write-Warn "No executable files found."
        Write-Warn "Please ensure you're running from a pre-built package or build the project first."
    }
    
    # Copy driver files (check multiple locations)
    $driverSources = @(
        (Join-Path $ScriptDir "drivers"),
        (Join-Path (Split-Path -Parent $ScriptDir) "drivers"),
        (Join-Path $RootDir "driver\output"),
        (Join-Path $RootDir "driver\x64\Release")
    )
    
    foreach ($driverSrc in $driverSources) {
        if (Test-Path $driverSrc) {
            Copy-Item "$driverSrc\*" (Join-Path $InstallPath "drivers") -Force -Recurse -ErrorAction SilentlyContinue
            Write-Host "  Copied drivers from $driverSrc" -ForegroundColor Gray
            break
        }
    }
    
    # Create config directory for user
    if (-not (Test-Path $ConfigPath)) {
        New-Item -ItemType Directory -Path $ConfigPath -Force | Out-Null
    }
    
    # Copy default config if user doesn't have one
    $userConfig = Join-Path $ConfigPath "config.json"
    if (-not (Test-Path $userConfig)) {
        $configSources = @(
            (Join-Path $ScriptDir "config\default_config.json"),
            (Join-Path (Split-Path -Parent $ScriptDir) "config\default_config.json"),
            (Join-Path $RootDir "config\default_config.json")
        )
        
        foreach ($configSrc in $configSources) {
            if (Test-Path $configSrc) {
                Copy-Item $configSrc $userConfig
                Write-Success "Default configuration created"
                break
            }
        }
    }
    
    Write-Success "Application installed to $InstallPath"
}

function Uninstall-Application {
    Write-Step "Removing application files..."
    
    # Stop running processes
    Get-Process -Name "WindowsAiMic*" -ErrorAction SilentlyContinue | Stop-Process -Force
    
    Start-Sleep -Seconds 1
    
    # Remove install directory
    if (Test-Path $InstallPath) {
        Remove-Item $InstallPath -Recurse -Force
        Write-Success "Application files removed"
    }
    
    # Ask about config
    if (Test-Path $ConfigPath) {
        if (-not $Silent) {
            $response = Read-Host "Remove configuration files? (Y/N)"
            if ($response -eq "Y" -or $response -eq "y") {
                Remove-Item $ConfigPath -Recurse -Force
                Write-Success "Configuration removed"
            }
        }
    }
}

function Install-ScheduledTask {
    Write-Step "Creating startup task..."
    
    $enginePath = Join-Path $InstallPath "WindowsAiMicEngine.exe"
    $taskName = "WindowsAiMic Engine"
    
    # Remove existing task if present
    Unregister-ScheduledTask -TaskName $taskName -Confirm:$false -ErrorAction SilentlyContinue
    
    # Create new task
    $action = New-ScheduledTaskAction -Execute $enginePath -Argument "--background"
    $trigger = New-ScheduledTaskTrigger -AtLogOn
    $principal = New-ScheduledTaskPrincipal -UserId $env:USERNAME -LogonType Interactive -RunLevel Highest
    $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1)
    
    Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Force | Out-Null
    
    Write-Success "Startup task created"
}

function Uninstall-ScheduledTask {
    Write-Step "Removing startup task..."
    
    Unregister-ScheduledTask -TaskName "WindowsAiMic Engine" -Confirm:$false -ErrorAction SilentlyContinue
    
    Write-Success "Startup task removed"
}

function Add-StartMenuShortcut {
    Write-Step "Creating Start Menu shortcut..."
    
    $startMenu = [Environment]::GetFolderPath("CommonPrograms")
    $shortcutPath = Join-Path $startMenu "WindowsAiMic.lnk"
    $targetPath = Join-Path $InstallPath "WindowsAiMicApp.exe"
    
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = $targetPath
    $shortcut.WorkingDirectory = $InstallPath
    $shortcut.Description = "AI-Powered Virtual Microphone Enhancement"
    $shortcut.Save()
    
    Write-Success "Start Menu shortcut created"
}

function Remove-StartMenuShortcut {
    $startMenu = [Environment]::GetFolderPath("CommonPrograms")
    $shortcutPath = Join-Path $startMenu "WindowsAiMic.lnk"
    
    if (Test-Path $shortcutPath) {
        Remove-Item $shortcutPath -Force
    }
}

function Start-Services {
    Write-Step "Starting WindowsAiMic..."
    
    $enginePath = Join-Path $InstallPath "WindowsAiMicEngine.exe"
    $appPath = Join-Path $InstallPath "WindowsAiMicApp.exe"
    
    if (Test-Path $enginePath) {
        Start-Process $enginePath -ArgumentList "--background" -WindowStyle Hidden
        Write-Success "Audio engine started"
    }
    
    if (Test-Path $appPath) {
        Start-Process $appPath -WindowStyle Hidden
        Write-Success "Tray application started"
    }
}

function Show-PostInstallInstructions {
    Write-Host @"

╔═══════════════════════════════════════════════════════════════════════════╗
║                         Installation Complete!                             ║
╠═══════════════════════════════════════════════════════════════════════════╣
║                                                                            ║
║  WindowsAiMic has been installed successfully.                            ║
║                                                                            ║
║  NEXT STEPS:                                                               ║
║  1. The tray icon should appear in your system tray                       ║
║  2. Right-click the icon to access settings                               ║
║  3. In your apps (Teams, Discord, OBS), select "Virtual Mic Driver"       ║
║     as your microphone input                                              ║
║                                                                            ║
"@ -ForegroundColor Green

    if ($script:NeedsReboot) {
        Write-Host @"
║  ⚠️  REBOOT REQUIRED                                                       ║
║  Test signing was enabled. Please restart your computer for the           ║
║  virtual audio driver to work properly.                                   ║
║                                                                            ║
"@ -ForegroundColor Yellow
    }

    Write-Host @"
║  TROUBLESHOOTING:                                                          ║
║  • If Virtual Mic doesn't appear: Check Device Manager                    ║
║  • Config file: $ConfigPath\config.json
║  • Logs: Check Windows Event Viewer                                       ║
║                                                                            ║
╚═══════════════════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Green
}

function Show-UninstallComplete {
    Write-Host @"

╔═══════════════════════════════════════════════════════════════════════════╗
║                         Uninstall Complete!                                ║
╠═══════════════════════════════════════════════════════════════════════════╣
║                                                                            ║
║  WindowsAiMic has been removed from your system.                          ║
║                                                                            ║
║  NOTE: Test signing mode is still enabled. To disable it:                 ║
║        bcdedit /set testsigning off                                       ║
║        (then restart your computer)                                       ║
║                                                                            ║
╚═══════════════════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Cyan
}

# ============================================================================
# Main
# ============================================================================

$script:NeedsReboot = $false

Show-Banner

if (-not (Test-Administrator)) {
    Write-Fail "This script must be run as Administrator!"
    Write-Host "Right-click the script and select 'Run as Administrator'"
    if (-not $Silent) { Read-Host "Press Enter to exit" }
    exit 1
}

if ($Uninstall) {
    Write-Host "Uninstalling WindowsAiMic..." -ForegroundColor Yellow
    
    Uninstall-ScheduledTask
    Uninstall-VirtualDriver
    Remove-StartMenuShortcut
    Uninstall-Application
    
    Show-UninstallComplete
} else {
    Write-Host "Installing WindowsAiMic..." -ForegroundColor Cyan
    
    # Enable test signing (required for unsigned driver)
    if (-not (Enable-TestSigning)) {
        exit 1
    }
    
    # Install components
    Install-Application
    Install-VirtualDriver
    Install-ScheduledTask
    Add-StartMenuShortcut
    
    # Start if no reboot needed
    if (-not $script:NeedsReboot) {
        Start-Services
    }
    
    Show-PostInstallInstructions
    
    if ($script:NeedsReboot) {
        if (-not $Silent) {
            $response = Read-Host "Reboot now? (Y/N)"
            if ($response -eq "Y" -or $response -eq "y") {
                Restart-Computer -Force
            }
        }
    }
}

if (-not $Silent) {
    Read-Host "`nPress Enter to exit"
}
