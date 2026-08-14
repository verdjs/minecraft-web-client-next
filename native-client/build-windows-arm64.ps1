# PowerShell Build Script for Windows ARM64 (Compatible with PowerShell 5.1 & PowerShell 7+)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Minecraft Native Client - Windows ARM64 Builder  " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# 1. Refresh PATH from registry so newly installed Winget packages are immediately available
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

# 2. Locate CMake
$cmakePath = $null
$cmd = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmd) {
    $cmakePath = $cmd.Source
}

if (-not $cmakePath) {
    $vsCmakePaths = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )

    foreach ($p in $vsCmakePaths) {
        if (Test-Path $p) {
            $cmakePath = $p
            break
        }
    }
}

if (-not $cmakePath) {
    Write-Host "[+] Installing CMake via winget..." -ForegroundColor Green
    winget install -e --id Kitware.CMake --accept-source-agreements --accept-package-agreements
    $env:Path = "C:\Program Files\CMake\bin;" + [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { $cmakePath = $cmd.Source }
}

if (-not $cmakePath) {
    Write-Host "`nError: CMake could not be found. Please restart PowerShell and re-run." -ForegroundColor Red
    exit 1
}

Write-Host "[+] Found CMake: $cmakePath" -ForegroundColor Green

# 3. Locate Clang / LLVM and Ninja
$clangPath = $null
$clangCmd = Get-Command clang++.exe -ErrorAction SilentlyContinue
if ($clangCmd) { $clangPath = $clangCmd.Source }

if (-not $clangPath) {
    $knownClangPaths = @(
        "C:\Program Files\LLVM\bin\clang++.exe",
        "C:\Program Files (x86)\LLVM\bin\clang++.exe",
        "$env:LOCALAPPDATA\Programs\LLVM\bin\clang++.exe"
    )
    foreach ($p in $knownClangPaths) {
        if (Test-Path $p) {
            $clangPath = $p
            $env:Path = (Split-Path -Parent $p) + ";" + $env:Path
            break
        }
    }
}

$ninjaPath = $null
$ninjaCmd = Get-Command ninja.exe -ErrorAction SilentlyContinue
if ($ninjaCmd) { $ninjaPath = $ninjaCmd.Source }

if (-not $ninjaPath) {
    # Check WinGet package directories for Ninja
    $ninjaSearch = Get-ChildItem -Path "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Filter "ninja.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ninjaSearch) {
        $ninjaPath = $ninjaSearch.FullName
        $env:Path = (Split-Path -Parent $ninjaPath) + ";" + $env:Path
    } else {
        $knownNinjaPaths = @(
            "C:\Program Files\Ninja\ninja.exe",
            "C:\ProgramData\chocolatey\bin\ninja.exe"
        )
        foreach ($p in $knownNinjaPaths) {
            if (Test-Path $p) {
                $ninjaPath = $p
                $env:Path = (Split-Path -Parent $p) + ";" + $env:Path
                break
            }
        }
    }
}

# 4. Setup build directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $scriptDir "build"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Set-Location $buildDir

# Clean previous failed CMakeCache if necessary
if (Test-Path "CMakeCache.txt") {
    Remove-Item -Force "CMakeCache.txt" -ErrorAction SilentlyContinue
}

# 5. Build with Clang + Ninja or Visual Studio
$built = $false

if ($clangPath) {
    $cCompiler = $clangPath.Replace("clang++.exe", "clang.exe")
    Write-Host "`n[+] Configuring build using LLVM Clang ($clangPath)..." -ForegroundColor Green
    
    if ($ninjaPath) {
        Write-Host "[+] Using Ninja generator ($ninjaPath)..." -ForegroundColor Green
        & $cmakePath -G "Ninja" "-DCMAKE_C_COMPILER=$cCompiler" "-DCMAKE_CXX_COMPILER=$clangPath" "-DCMAKE_MAKE_PROGRAM=$ninjaPath" -DCMAKE_BUILD_TYPE=Release ..
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`n[+] Compiling Minecraft Native with Ninja..." -ForegroundColor Cyan
            & $ninjaPath
            if ($LASTEXITCODE -eq 0) { $built = $true }
        }
    } else {
        & $cmakePath "-DCMAKE_C_COMPILER=$cCompiler" "-DCMAKE_CXX_COMPILER=$clangPath" -DCMAKE_BUILD_TYPE=Release ..
        if ($LASTEXITCODE -eq 0) {
            & $cmakePath --build . --config Release
            if ($LASTEXITCODE -eq 0) { $built = $true }
        }
    }
}

if (-not $built) {
    Write-Host "`n[+] Trying Visual Studio generator..." -ForegroundColor Cyan
    & $cmakePath -A ARM64 ..
    if ($LASTEXITCODE -ne 0) {
        & $cmakePath ..
    }
    & $cmakePath --build . --config Release
    if ($LASTEXITCODE -eq 0) { $built = $true }
}

if ($built) {
    Write-Host "`n==================================================" -ForegroundColor Green
    Write-Host "  SUCCESS! Built MinecraftNative.exe (ARM64)      " -ForegroundColor Green
    Write-Host "==================================================" -ForegroundColor Green
    Write-Host "Executable location: $buildDir\MinecraftNative.exe (or $buildDir\Release\MinecraftNative.exe)" -ForegroundColor White
} else {
    Write-Host "`n==================================================" -ForegroundColor Red
    Write-Host "  BUILD CONFIGURATION                             " -ForegroundColor Red
    Write-Host "==================================================" -ForegroundColor Red
    Write-Host "Please restart your PowerShell window so your system PATH updates with the new LLVM and Ninja install, then re-run:" -ForegroundColor Yellow
    Write-Host "  cd \\Mac\Home\Downloads\minecraft-web-client-next\native-client" -ForegroundColor White
    Write-Host "  .\build-windows-arm64.ps1" -ForegroundColor Cyan
}
