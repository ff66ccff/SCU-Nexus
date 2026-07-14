[CmdletBinding()]
param(
  [string]$QtDir = $env:QT_ROOT_DIR,
  [string]$OpenSSLRoot = $env:OPENSSL_ROOT_DIR,
  [string]$BuildDir,
  [string]$PackageDir,
  [switch]$SkipTests,
  [switch]$SkipInstaller
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$global:LASTEXITCODE = 0

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SourceDir = Join-Path $RepoRoot "SCU_Nexus"
if (-not $BuildDir) {
  $BuildDir = Join-Path $RepoRoot "out/build/windows-release"
}
if (-not $PackageDir) {
  $PackageDir = Join-Path $RepoRoot "out/packages"
}

function Resolve-Executable {
  param(
    [Parameter(Mandatory)][string]$Name,
    [string[]]$Candidates = @()
  )

  $command = Get-Command $Name -ErrorAction SilentlyContinue
  if ($command) {
    return $command.Source
  }
  foreach ($candidate in $Candidates) {
    if ($candidate -and (Test-Path $candidate -PathType Leaf)) {
      return (Resolve-Path $candidate).Path
    }
  }
  throw "Cannot find $Name. Install it or add it to PATH."
}

function Resolve-QtDirectory {
  param([string]$Requested)

  if ($Requested) {
    $resolved = Resolve-Path $Requested -ErrorAction Stop
    if (Test-Path (Join-Path $resolved.Path "bin/windeployqt.exe")) {
      return $resolved.Path
    }
    throw "QtDir does not contain bin/windeployqt.exe: $Requested"
  }

  $candidates = @()
  foreach ($root in @("C:/Qt", "D:/Qt", "C:/QT", "D:/QT")) {
    if (Test-Path $root) {
      $candidates += Get-ChildItem $root -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^6\.' } |
        ForEach-Object { Get-ChildItem $_.FullName -Directory -Filter "mingw_64" -ErrorAction SilentlyContinue }
    }
  }
  $match = $candidates |
    Where-Object { Test-Path (Join-Path $_.FullName "bin/windeployqt.exe") } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
  if (-not $match) {
    throw "Cannot find Qt 6 MinGW 64-bit. Specify -QtDir, for example D:/Qt/6.11.1/mingw_64."
  }
  return $match.FullName
}

$QtDir = Resolve-QtDirectory $QtDir
$QtBase = Split-Path (Split-Path $QtDir -Parent) -Parent
$ToolsDir = Join-Path $QtBase "Tools"

$mingw = Get-ChildItem $ToolsDir -Directory -Filter "mingw*_64" -ErrorAction SilentlyContinue |
  Where-Object { Test-Path (Join-Path $_.FullName "bin/g++.exe") } |
  Sort-Object LastWriteTime -Descending |
  Select-Object -First 1
if (-not $mingw) {
  throw "Cannot find a Qt MinGW toolchain under $ToolsDir."
}

if (-not $OpenSSLRoot) {
  $candidate = Join-Path $mingw.FullName "opt"
  if (Test-Path (Join-Path $candidate "include/openssl/evp.h")) {
    $OpenSSLRoot = $candidate
  }
}
if (-not $OpenSSLRoot -or -not (Test-Path (Join-Path $OpenSSLRoot "include/openssl/evp.h"))) {
  throw "Cannot find a compatible OpenSSL SDK. Specify its root with -OpenSSLRoot."
}
$OpenSSLRoot = (Resolve-Path $OpenSSLRoot).Path

$cmakeCandidates = @(
  (Join-Path $ToolsDir "CMake_64/bin/cmake.exe"),
  (Join-Path $ToolsDir "CMake/bin/cmake.exe")
)
$ninjaCandidates = @(
  (Join-Path $ToolsDir "Ninja/ninja.exe")
)
$CMake = Resolve-Executable "cmake.exe" $cmakeCandidates
$Ninja = Resolve-Executable "ninja.exe" $ninjaCandidates
$CTest = Join-Path (Split-Path $CMake -Parent) "ctest.exe"
$CPack = Join-Path (Split-Path $CMake -Parent) "cpack.exe"

$runtimeNames = @("libcrypto-3-x64.dll", "libcrypto-1_1-x64.dll", "libcrypto-1_1.dll")
$cryptoRuntime = $runtimeNames |
  ForEach-Object { Join-Path $OpenSSLRoot "bin/$_" } |
  Where-Object { Test-Path $_ -PathType Leaf } |
  Select-Object -First 1
if (-not $cryptoRuntime) {
  throw "Cannot find a libcrypto runtime DLL under $OpenSSLRoot/bin."
}

$env:PATH = @(
  (Join-Path $QtDir "bin"),
  (Join-Path $mingw.FullName "bin"),
  (Split-Path $CMake -Parent),
  (Split-Path $Ninja -Parent),
  (Join-Path $OpenSSLRoot "bin"),
  $env:PATH
) -join [IO.Path]::PathSeparator

New-Item -ItemType Directory -Force -Path $BuildDir, $PackageDir | Out-Null

Write-Host "Qt:       $QtDir"
Write-Host "Compiler: $($mingw.FullName)"
Write-Host "OpenSSL:  $OpenSSLRoot"
Write-Host "Build:    $BuildDir"

& $CMake -S $SourceDir -B $BuildDir -G Ninja `
  "-DCMAKE_MAKE_PROGRAM=$Ninja" `
  "-DCMAKE_BUILD_TYPE=Release" `
  "-DBUILD_TESTING=ON" `
  "-DQt6_ROOT=$QtDir" `
  "-DCMAKE_PREFIX_PATH=$QtDir" `
  "-DOPENSSL_ROOT_DIR=$OpenSSLRoot" `
  "-DSCU_NEXUS_OPENSSL_RUNTIME=$cryptoRuntime"
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }

& $CMake --build $BuildDir --parallel
if ($LASTEXITCODE -ne 0) { throw "Release build failed." }

if (-not $SkipTests) {
  & $CTest --test-dir $BuildDir --output-on-failure -C Release
  if ($LASTEXITCODE -ne 0) { throw "Tests failed; packaging stopped." }
}

& $CPack --config (Join-Path $BuildDir "CPackConfig.cmake") -G ZIP -B $PackageDir
if ($LASTEXITCODE -ne 0) { throw "Portable ZIP packaging failed." }

if (-not $SkipInstaller) {
  $makensisCandidates = @(
    (Join-Path $RepoRoot "out/tools/nsis/makensis.exe"),
    "$env:ProgramFiles/NSIS/makensis.exe",
    "${env:ProgramFiles(x86)}/NSIS/makensis.exe",
    "$env:LOCALAPPDATA/Programs/NSIS/makensis.exe"
  )
  $makensis = Get-Command "makensis.exe" -ErrorAction SilentlyContinue
  if (-not $makensis) {
    $makensisPath = $makensisCandidates |
      Where-Object { $_ -and (Test-Path $_ -PathType Leaf) } |
      Select-Object -First 1
    if ($makensisPath) {
      $env:PATH = "$(Split-Path $makensisPath -Parent)$([IO.Path]::PathSeparator)$env:PATH"
      $makensis = Get-Item $makensisPath
    }
  }

  if ($makensis) {
    & $CPack --config (Join-Path $BuildDir "CPackConfig.cmake") -G NSIS -B $PackageDir
    if ($LASTEXITCODE -ne 0) { throw "NSIS installer packaging failed." }
  } else {
    Write-Warning "NSIS was not found; installer skipped. Run 'winget install --id NSIS.NSIS --exact' and retry."
  }
}

Write-Host "Release artifacts:"
Get-ChildItem $PackageDir -File |
  Where-Object { $_.Name -match '\.(exe|zip|sha256)$' } |
  Sort-Object Name |
  ForEach-Object { Write-Host "  $($_.FullName)" }
