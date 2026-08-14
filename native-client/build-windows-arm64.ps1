# PowerShell Build Script for Windows ARM64 (Compatible with PowerShell 5.1 & PowerShell 7+)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Minecraft Native Client - Windows ARM64 Builder  " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# 1. Refresh PATH from registry
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

# 2. Locate Windows SDK & MSVC CRT libraries if available
$sdkLibDir = $null
$sdkIncludeDir = $null
$windowsKitsLib = "C:\Program Files (x86)\Windows Kits\10\Lib"
if (Test-Path $windowsKitsLib) {
    $latestVersion = Get-ChildItem -Path $windowsKitsLib -Directory | Sort-Object Name -Descending | Select-Object -First 1
    if ($latestVersion) {
        $sdkLibDir = "$windowsKitsLib\$($latestVersion.Name)\um\arm64;$windowsKitsLib\$($latestVersion.Name)\ucrt\arm64;$windowsKitsLib\$($latestVersion.Name)\um\x64;$windowsKitsLib\$($latestVersion.Name)\ucrt\x64"
        $env:LIB = "$sdkLibDir;" + $env:LIB
    }
}

# 3. Locate CMake
$cmakePath = $null
$cmd = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmd) { $cmakePath = $cmd.Source }

if (-not $cmakePath) {
    $vsCmakePaths = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe"
    )
    foreach ($p in $vsCmakePaths) {
        if (Test-Path $p) { $cmakePath = $p; break }
    }
}

if (-not $cmakePath) {
    Write-Host "[+] Installing CMake via winget..." -ForegroundColor Green
    winget install -e --id Kitware.CMake --accept-source-agreements --accept-package-agreements
    $env:Path = "C:\Program Files\CMake\bin;" + $env:Path
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { $cmakePath = $cmd.Source }
}

# 4. Locate Clang & Ninja
$clangPath = $null
$clangCmd = Get-Command clang++.exe -ErrorAction SilentlyContinue
if ($clangCmd) { $clangPath = $clangCmd.Source }

if (-not $clangPath) {
    $knownClangPaths = @(
        "C:\Program Files\LLVM\bin\clang++.exe",
        "C:\Program Files (x86)\LLVM\bin\clang++.exe"
    )
    foreach ($p in $knownClangPaths) {
        if (Test-Path $p) { $clangPath = $p; break }
    }
}

$ninjaPath = $null
$ninjaCmd = Get-Command ninja.exe -ErrorAction SilentlyContinue
if ($ninjaCmd) { $ninjaPath = $ninjaCmd.Source }

if (-not $ninjaPath) {
    $ninjaSearch = Get-ChildItem -Path "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Filter "ninja.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ninjaSearch) { $ninjaPath = $ninjaSearch.FullName }
}

# 5. Check if Windows SDK / CRT is missing
if (-not (Test-Path $windowsKitsLib) -and -not (Test-Path "C:\Program Files\Microsoft Visual Studio\2022")) {
    Write-Host "`n[!] Windows C++ SDK libraries (kernel32.lib / CRT) are missing." -ForegroundColor Yellow
    Write-Host "Installing Windows 10/11 SDK via winget..." -ForegroundColor Green
    winget install -e --id Microsoft.WindowsSDK.10.0.22621 --accept-source-agreements --accept-package-agreements
}

# 6. Local Windows build directory on C:
$rawScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceDir = $rawScriptDir
$localBuildDir = "C:\Users\$env:USERNAME\mc-native-build"

if (Test-Path $localBuildDir) {
    Remove-Item -Recurse -Force $localBuildDir -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $localBuildDir | Out-Null

Write-Host "[+] Source Directory: $sourceDir" -ForegroundColor White
Write-Host "[+] Local Build Directory: $localBuildDir" -ForegroundColor White

# 7. Configure with CMake
$built = $false

if ($clangPath -and $ninjaPath) {
    $cCompiler = $clangPath.Replace("clang++.exe", "clang.exe")
    Write-Host "`n[+] Configuring build with LLVM Clang & Ninja..." -ForegroundColor Green
    
    & $cmakePath -S "$sourceDir" -B "$localBuildDir" -G "Ninja" "-DCMAKE_C_COMPILER=$cCompiler" "-DCMAKE_CXX_COMPILER=$clangPath" "-DCMAKE_MAKE_PROGRAM=$ninjaPath" -DCMAKE_BUILD_TYPE=Release
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n[+] Compiling Minecraft Native..." -ForegroundColor Cyan
        & $ninjaPath -C "$localBuildDir"
        if ($LASTEXITCODE -eq 0) { $built = $true }
    }
}

if (-not $built) {
    Write-Host "`n[+] Trying Visual Studio build..." -ForegroundColor Cyan
    & $cmakePath -S "$sourceDir" -B "$localBuildDir" -A ARM64
    if ($LASTEXITCODE -ne 0) {
        & $cmakePath -S "$sourceDir" -B "$localBuildDir"
    }
    & $cmakePath --build "$localBuildDir" --config Release
    if ($LASTEXITCODE -eq 0) { $built = $true }
}

# 8. Copy binary back
if ($built) {
    $outputExe = $null
    $possibleExes = @(
        "$localBuildDir\MinecraftNative.exe",
        "$localBuildDir\Release\MinecraftNative.exe"
    )
    foreach ($e in $possibleExes) {
        if (Test-Path $e) { $outputExe = $e; break }
    }

    $destBuildDir = Join-Path $sourceDir "build"
    if (-not (Test-Path $destBuildDir)) { New-Item -ItemType Directory -Path $destBuildDir | Out-Null }

    if ($outputExe) {
        Copy-Item -Force $outputExe (Join-Path $destBuildDir "MinecraftNative.exe")
        Write-Host "`n==================================================" -ForegroundColor Green
        Write-Host "  SUCCESS! Built MinecraftNative.exe (ARM64)      " -ForegroundColor Green
        Write-Host "==================================================" -ForegroundColor Green
        Write-Host "Executable location: $destBuildDir\MinecraftNative.exe" -ForegroundColor White
        Write-Host "`nLaunching Minecraft Native..." -ForegroundColor Cyan
        Start-Process (Join-Path $destBuildDir "MinecraftNative.exe")
    }
} else {
    Write-Host "`n==================================================" -ForegroundColor Red
    Write-Host "  WINDOWS SDK REQUIRED                            " -ForegroundColor Red
    Write-Host "==================================================" -ForegroundColor Red
    Write-Host "Clang requires Windows system libraries (kernel32.lib)." -ForegroundColor Yellow
    Write-Host "Run this command in PowerShell to install the Windows C++ SDK:" -ForegroundColor White
    Write-Host "  winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override `"--passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended`"" -ForegroundColor Cyan
}
