# =============================================================================
# STM32N6570-DK Face Recognition Project — Cleanup Script
# =============================================================================
# Removes duplicate / unused folders so only files needed for your board
# (STM32N6570-DK) and camera (IMX335) remain.
#
# Run from the project root:
#   powershell -ExecutionPolicy Bypass -File .\cleanup_project.ps1
#
# After running, re-open the project in STM32CubeIDE and do:
#   Project -> Clean -> Build All
# =============================================================================

$ErrorActionPreference = "Continue"
$projectRoot = $PSScriptRoot
Write-Host "Project root: $projectRoot" -ForegroundColor Cyan

function Remove-IfExists {
    param([string]$Path, [string]$Reason)
    $full = Join-Path $projectRoot $Path
    if (Test-Path $full) {
        $size = (Get-ChildItem $full -Recurse -ErrorAction SilentlyContinue |
                 Measure-Object -Property Length -Sum).Sum / 1MB
        Remove-Item $full -Recurse -Force -ErrorAction SilentlyContinue
        if (Test-Path $full) {
            Write-Host ("[FAIL] {0,-70} ({1:N1} MB) — {2}" -f $Path, $size, $Reason) -ForegroundColor Red
        } else {
            Write-Host ("[OK]   {0,-70} ({1:N1} MB) — {2}" -f $Path, $size, $Reason) -ForegroundColor Green
        }
    } else {
        Write-Host ("[SKIP] {0,-70} — not present" -f $Path) -ForegroundColor DarkGray
    }
}

Write-Host ""
Write-Host "=== Removing build artifacts (will be regenerated) ===" -ForegroundColor Yellow
Remove-IfExists "Application\build" "stale build objects"
Remove-IfExists "Application\STM32CubeIDE\Debug" "stale Debug build; regenerates from fixed .cproject"

Write-Host ""
Write-Host "=== Removing duplicate Cube firmware ===" -ForegroundColor Yellow
Remove-IfExists "STM32Cube_FW_N6" "duplicate of Drivers/ and Middlewares/"

Write-Host ""
Write-Host "=== Removing empty / duplicate folders under Application/STM32CubeIDE/ ===" -ForegroundColor Yellow
Remove-IfExists "Application\STM32CubeIDE\Application"   "empty placeholder"
Remove-IfExists "Application\STM32CubeIDE\Drivers"       "duplicate of top-level Drivers/"
Remove-IfExists "Application\STM32CubeIDE\Middlewares"   "duplicate of top-level Middlewares/"
Remove-IfExists "Application\STM32CubeIDE\Utilities"     "duplicate of top-level Utilities/"
Remove-IfExists "Application\STM32CubeIDE\ll_aton"       "empty placeholder"
Remove-IfExists "ll_aton"                                "empty placeholder (top-level)"

Write-Host ""
Write-Host "=== Removing BSPs for other boards ===" -ForegroundColor Yellow
Remove-IfExists "Drivers\BSP\STM32N6xx_Nucleo"           "not STM32N6570-DK"

Write-Host ""
Write-Host "=== Removing unused flash-chip driver ===" -ForegroundColor Yellow
Remove-IfExists "Drivers\BSP\Components\mx25um51245g"    "you use mx66uw1g45g, not this"

Write-Host ""
Write-Host "=== Removing camera sensor drivers you don't use (keeping IMX335) ===" -ForegroundColor Yellow
Remove-IfExists "Middlewares\stm32-mw-camera\sensors\vd6g"    "not your camera"
Remove-IfExists "Middlewares\stm32-mw-camera\sensors\vd55g1"  "not your camera"
Remove-IfExists "Middlewares\stm32-mw-camera\sensors\vd1943"  "not your camera"
Remove-IfExists "Middlewares\stm32-mw-camera\sensors\ov5640"  "not your camera"

Write-Host ""
Write-Host "=== Removing other unused / empty folders ===" -ForegroundColor Yellow
Remove-IfExists "Middlewares\ISP_Library"                "empty; real one is under stm32-mw-camera"
Remove-IfExists "Middlewares\screenl"                    "UVC/ILI9341 touchscreen lib not used on STM32N6570-DK"

Write-Host ""
Write-Host "=== Removing heavy CMSIS DSP examples (already excluded in .cproject) ===" -ForegroundColor Yellow
Remove-IfExists "Drivers\CMSIS\DSP\Examples"             "examples, not referenced"
Remove-IfExists "Drivers\CMSIS\DSP\Testing"              "testing harness, not referenced"
Remove-IfExists "Drivers\CMSIS\Documentation"            "HTML docs, not referenced"
Remove-IfExists "Drivers\CMSIS\RTOS2"                    "not using RTOS"
Remove-IfExists "Drivers\CMSIS\Core_A"                   "Cortex-A core, we're Cortex-M55"

Write-Host ""
Write-Host "=== DONE ===" -ForegroundColor Cyan
$total = (Get-ChildItem $projectRoot -Recurse -ErrorAction SilentlyContinue |
          Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host ("Project now ~{0:N0} MB" -f $total) -ForegroundColor Cyan
Write-Host ""
Write-Host "Next: open project in STM32CubeIDE -> Project -> Clean -> Build All" -ForegroundColor Yellow
