# PowerShell Build Script for Windows ARM64 (Compatible with PowerShell 5.1 & PowerShell 7+)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Minecraft Native Client - Windows ARM64 Builder  " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# 1. Refresh PATH from registry
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

# 4. Resolve UNC Path by building on local C: drive to bypass Windows CMD.EXE UNC limitation
$rawScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceDir = $rawScriptDir

# Local Windows build directory on C: drive (fast & prevents UNC error)
$localBuildDir = "C:\Users\$env:USERNAME\mc-native-build"
if (Test-Path $localBuildDir) {
    Remove-Item -Recurse -Force $localBuildDir -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $localBuildDir | Out-Null

Write-Host "[+] Source Directory: $sourceDir" -ForegroundColor White
Write-Host "[+] Local C: Build Directory: $localBuildDir" -ForegroundColor White

# 5. Configure with CMake
$built = $false

if ($clangPath -and $ninjaPath) {
    $cCompiler = $clangPath.Replace("clang++.exe", "clang.exe")
    Write-Host "`n[+] Configuring build with LLVM Clang & Ninja on C: drive..." -ForegroundColor Green
    
    & $cmakePath -S "$sourceDir" -B "$localBuildDir" -G "Ninja" "-DCMAKE_C_COMPILER=$cCompiler" "-DCMAKE_CXX_COMPILER=$clangPath" "-DCMAKE_MAKE_PROGRAM=$ninjaPath" -DCMAKE_BUILD_TYPE=Release
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n[+] Compiling Minecraft Native with Ninja..." -ForegroundColor Cyan
        & $ninjaPath -C "$localBuildDir"
        if ($LASTEXITCODE -eq 0) { $built = $true }
    }
}

if (-not $built) {
    Write-Host "`n[+] Configuring with Visual Studio generator on C: drive..." -ForegroundColor Cyan
    & $cmakePath -S "$sourceDir" -B "$localBuildDir" -A ARM64
    if ($LASTEXITCODE -ne 0) {
        & $cmakePath -S "$sourceDir" -B "$localBuildDir"
    }
    & $cmakePath --build "$localBuildDir" --config Release
    if ($LASTEXITCODE -eq 0) { $built = $true }
}

# 6. Copy output binary back to native-client/build
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
    Write-Host "`n[!] Build failed. Please inspect CMake output above." -ForegroundColor Red
}
