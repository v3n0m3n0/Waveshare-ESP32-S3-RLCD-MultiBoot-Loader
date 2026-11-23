param(
    [string]$Port,
    [int]$Baud = 921600,
    [switch]$FullImage
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# 1. Detect COM port if not specified
if (-not $Port) {
    $ports = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object -Unique
    if ($ports.Count -eq 1) {
        $Port = $ports[0]
        Write-Host "[*] Auto-detected COM port: $Port" -ForegroundColor Cyan
    } else {
        Write-Host "Available COM ports: $($ports -join ', ')" -ForegroundColor Yellow
        $Port = Read-Host "Enter COM port (e.g. COM11)"
    }
}

if (-not $Port) {
    Write-Error "No COM port provided."
    exit 1
}

# 2. Locate Python / esptool
$pythonExe = "python"
if (Get-Command "C:\Users\KB\.platformio\penv\Scripts\python.exe" -ErrorAction SilentlyContinue) {
    $pythonExe = "C:\Users\KB\.platformio\penv\Scripts\python.exe"
}

Write-Host "=== Flashing Waveshare ESP32-S3 RLCD MultiBoot Loader to $Port ===" -ForegroundColor Green

if ($FullImage) {
    $fullBin = Get-ChildItem -Path $scriptDir -Filter "waveshare-esp32s3-rlcd-multiboot-loader-full-16MB-*.bin" | Select-Object -First 1
    if (-not $fullBin) {
        Write-Error "Full factory image not found in $scriptDir"
        exit 1
    }
    Write-Host "[*] Writing Full 16MB Factory Image: $($fullBin.Name) at 0x0 ..." -ForegroundColor Yellow
    & $pythonExe -m esptool --chip esp32s3 --port $Port --baud $Baud `
        --before default-reset --after watchdog-reset write_flash `
        --flash_mode qio --flash_size 16MB --flash_freq 80m `
        0x0 "$($fullBin.FullName)"
} else {
    $appBin = Get-ChildItem -Path $scriptDir -Filter "rlcd-launcher-*.bin" | Select-Object -First 1
    $bootBin = Join-Path $scriptDir "bootloader.bin"
    $partBin = Join-Path $scriptDir "partitions.bin"
    $otaBin  = Join-Path $scriptDir "otadata.bin"

    if (-not ($appBin -and (Test-Path $bootBin) -and (Test-Path $partBin) -and (Test-Path $otaBin))) {
        Write-Error "Missing required release binary files in $scriptDir"
        exit 1
    }

    Write-Host "[*] Writing MultiBoot Loader (Preserving NVS at 0x9000) ..." -ForegroundColor Yellow
    & $pythonExe -m esptool --chip esp32s3 --port $Port --baud $Baud `
        --before default-reset --after watchdog-reset write_flash `
        --flash_mode qio --flash_size 16MB --flash_freq 80m `
        0x0 "$bootBin" `
        0x8000 "$partBin" `
        0xe000 "$otaBin" `
        0x10000 "$($appBin.FullName)"
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[SUCCESS] MultiBoot Loader flashed successfully to $Port!" -ForegroundColor Green
} else {
    Write-Error "Flashing failed with exit code $LASTEXITCODE"
}
