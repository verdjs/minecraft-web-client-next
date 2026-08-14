# PowerShell Build Script for Windows ARM64 (Compatible with PowerShell 5.1 & PowerShell 7+)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Minecraft Native Client - Windows ARM64 Builder  " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# 1. Locate CMake
$cmakePath = $null
$cmd = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmd) {
    $cmakePath = $cmd.Source
}

if (-not $cmakePath) {
    $vsCmakePaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\CMake\bin\cmake.exe"
    )

    foreach ($p in $vsCmakePaths) {
        if (Test-Path $p) {
            $cmakePath = $p
            break
        }
    }
}

if (-not $cmakePath) {
    Write-Host "`n[!] CMake is not installed or not in PATH." -ForegroundColor Yellow
    Write-Host "Installing CMake automatically via winget..." -ForegroundColor Green
    try {
        winget install -e --id Kitware.CMake --accept-source-agreements --accept-package-agreements
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
        $cmd = Get-Command cmake -ErrorAction SilentlyContinue
        if ($cmd) {
            $cmakePath = $cmd.Source
        }
    } catch {
        Write-Host "Automatic install failed. Please run: winget install Kitware.CMake" -ForegroundColor Red
    }
}

if (-not $cmakePath) {
    Write-Host "`nError: CMake could not be found. Please install CMake via 'winget install Kitware.CMake' and restart PowerShell." -ForegroundColor Red
    exit 1
}

Write-Host "`n[+] Found CMake at: $cmakePath" -ForegroundColor Green

# 2. Check for C++ Compiler (MSVC / Clang / GCC)
$hasCompiler = $false
if (Get-Command cl -ErrorAction SilentlyContinue) { $hasCompiler = $true }
if (Get-Command clang++ -ErrorAction SilentlyContinue) { $hasCompiler = $true }
if (Get-Command g++ -ErrorAction SilentlyContinue) { $hasCompiler = $true }

if (-not $hasCompiler) {
    # Check if Visual Studio with C++ is installed
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsInstall) { $hasCompiler = $true }
    }
}

if (-not $hasCompiler) {
    Write-Host "`n[!] No C++ compiler detected on your Windows system." -ForegroundColor Yellow
    Write-Host "Installing lightweight Clang/LLVM C++ compiler and Ninja builder via winget (fast)..." -ForegroundColor Green
    try {
        winget install -e --id LLVM.LLVM --accept-source-agreements --accept-package-agreements
        winget install -e --id Ninja-build.Ninja --accept-source-agreements --accept-package-agreements
        $env:Path = "C:\Program Files\LLVM\bin;" + [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    } catch {
        Write-Host "Automatic compiler install failed." -ForegroundColor Red
    }
}

# 3. Setup build directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $scriptDir "build"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Set-Location $buildDir

# 4. Configure & Compile for Windows ARM64
Write-Host "`n[+] Configuring Windows ARM64 build..." -ForegroundColor Cyan

# Try Ninja + Clang first if available, otherwise Visual Studio ARM64 generator
$built = $false
if (Get-Command ninja -ErrorAction SilentlyContinue) {
    Write-Host "[+] Using Ninja + Clang generator..." -ForegroundColor Green
    & $cmakePath -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
    if ($LASTEXITCODE -eq 0) {
        & ninja
        if ($LASTEXITCODE -eq 0) { $built = $true }
    }
}

if (-not $built) {
    Write-Host "[+] Trying Visual Studio ARM64 generator..." -ForegroundColor Cyan
    & $cmakePath -A ARM64 ..
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[!] Defaulting to standard MSVC generator..." -ForegroundColor Yellow
        & $cmakePath ..
    }
    & $cmakePath --build . --config Release
    if ($LASTEXITCODE -eq 0) { $built = $true }
}

if ($built) {
    Write-Host "`n==================================================" -ForegroundColor Green
    Write-Host "  SUCCESS! Built MinecraftNative.exe (ARM64)      " -ForegroundColor Green
    Write-Host "==================================================" -ForegroundColor Green
    Write-Host "Executable location: $buildDir\Release\MinecraftNative.exe (or $buildDir\MinecraftNative.exe)" -ForegroundColor White
} else {
    Write-Host "`n==================================================" -ForegroundColor Red
    Write-Host "  COMPILER SETUP REQUIRED                         " -ForegroundColor Red
    Write-Host "==================================================" -ForegroundColor Red
    Write-Host "To finish setting up C++ compilation on Windows:" -ForegroundColor Yellow
    Write-Host "Run this command in PowerShell to install the C++ build tools:" -ForegroundColor White
    Write-Host "  winget install -e --id LLVM.LLVM" -ForegroundColor Cyan
    Write-Host "  winget install -e --id Ninja-build.Ninja" -ForegroundColor Cyan
    Write-Host "Or install Visual Studio Community with 'Desktop development with C++'." -ForegroundColor White
}
