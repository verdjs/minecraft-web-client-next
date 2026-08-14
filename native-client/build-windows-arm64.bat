@echo off
setlocal
echo ==================================================
echo   Minecraft Native Client - Windows ARM64 Builder
echo ==================================================

powershell -ExecutionPolicy Bypass -File "%~dp0build-windows-arm64.ps1"
pause
