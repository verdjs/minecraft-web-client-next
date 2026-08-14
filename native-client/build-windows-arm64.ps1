# PowerShell Build Script for Windows ARM64

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Minecraft Native Client - Windows ARM64 Builder  " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# 1. Locate CMake
$cmakePath = (Get-Command cmake -ErrorAction SilentlyContinue)?.Source

if (-not $cmakePath) {
    # Check standard Visual Studio 2022 CMake locations
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
        # Refresh PATH in current session
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
        $cmakePath = (Get-Command cmake -ErrorAction SilentlyContinue)?.Source
    } catch {
        Write-Host "Automatic install failed. Please run: winget install Kitware.CMake" -ForegroundColor Red
    }
}

if (-not $cmakePath) {
    Write-Host "`nError: CMake could not be found. Please install CMake from https://cmake.org/download/ or run 'winget install Kitware.CMake' and restart PowerShell." -ForegroundColor Red
    exit 1
}

Write-Host "`n[+] Found CMake at: $cmakePath" -ForegroundColor Green

# 2. Setup build directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $scriptDir "build"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Set-Location $buildDir

# 3. Configure & Compile for Windows ARM64
Write-Host "`n[+] Configuring Windows ARM64 Visual Studio build..." -ForegroundColor Cyan
& $cmakePath -A ARM64 ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "[!] Defaulting to standard generator..." -ForegroundColor Yellow
    & $cmakePath ..
}

Write-Host "`n[+] Compiling Minecraft Native Release Binary..." -ForegroundColor Cyan
& $cmakePath --build . --config Release

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n==================================================" -ForegroundColor Green
    Write-Host "  SUCCESS! Built MinecraftNative.exe (ARM64)      " -ForegroundColor Green
    Write-Host "==================================================" -ForegroundColor Green
    Write-Host "Executable location: $buildDir\Release\MinecraftNative.exe" -ForegroundColor White
} else {
    Write-Host "`n[!] Build failed. Ensure Visual Studio with C++ desktop tools is installed." -ForegroundColor Red
}
