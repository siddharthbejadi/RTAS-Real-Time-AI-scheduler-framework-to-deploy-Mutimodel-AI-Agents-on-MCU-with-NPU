param(
    [int]$Jobs = 4,
    [string]$ToolchainBin = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$debugDir = Join-Path $repoRoot "Application\STM32CubeIDE\Debug"

if ([string]::IsNullOrWhiteSpace($ToolchainBin)) {
    $gcc = Get-Command arm-none-eabi-gcc.exe -ErrorAction SilentlyContinue
    if ($gcc) {
        $ToolchainBin = Split-Path $gcc.Source -Parent
    } else {
        $searchRoots = @(
            "C:\ST",
            "C:\Program Files\STMicroelectronics",
            "C:\Program Files"
        ) | Where-Object { Test-Path $_ }

        $gccPath = $searchRoots |
            ForEach-Object {
                Get-ChildItem -Path $_ -Recurse -Filter arm-none-eabi-gcc.exe -ErrorAction SilentlyContinue
            } |
            Sort-Object FullName -Descending |
            Select-Object -First 1

        if (-not $gccPath) {
            throw "arm-none-eabi-gcc.exe was not found. Open STM32CubeIDE once or pass -ToolchainBin <path-to-tools\bin>."
        }

        $ToolchainBin = Split-Path $gccPath.FullName -Parent
    }
}

if (-not (Test-Path (Join-Path $ToolchainBin "arm-none-eabi-gcc.exe"))) {
    throw "ToolchainBin does not contain arm-none-eabi-gcc.exe: $ToolchainBin"
}

$env:PATH = "$ToolchainBin;$env:PATH"

mingw32-make -C $debugDir "-j$Jobs"
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
