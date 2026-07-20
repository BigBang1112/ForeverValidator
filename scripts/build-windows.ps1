#Requires -Version 5.1

[CmdletBinding()]
param(
    [switch]$Clean,
    [ValidateRange(1, 256)]
    [int]$Jobs = [Environment]::ProcessorCount
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

if ($env:OS -ne "Windows_NT" -or
    -not [Environment]::Is64BitOperatingSystem) {
    throw "The Windows build requires 64-bit Windows."
}

[Net.ServicePointManager]::SecurityProtocol =
    [Net.ServicePointManager]::SecurityProtocol -bor
    [Net.SecurityProtocolType]::Tls12

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$BuildRoot = Join-Path $RepositoryRoot "build"
$NativeBuildRoot = Join-Path $BuildRoot "windows-gcc"
$VcpkgInstalledRoot = Join-Path $BuildRoot "windows-vcpkg"
$DistRoot = Join-Path $RepositoryRoot "dist"

$CacheBase = if ($env:LOCALAPPDATA) {
    $env:LOCALAPPDATA
} else {
    $env:TEMP
}
$CacheRoot = Join-Path $CacheBase "FV"
$ArchiveRoot = Join-Path $CacheRoot "a"
$ToolRoot = Join-Path $CacheRoot "t"
$VcpkgDownloadRoot = Join-Path $CacheRoot "d"
$VcpkgBinaryCache = Join-Path $CacheRoot "b"

$WinLibsVersion = "16.1.0posix-14.0.0-ucrt-r2"
$WinLibsArchiveName =
    "winlibs-x86_64-posix-seh-gcc-16.1.0-mingw-w64ucrt-14.0.0-r2.zip"
$WinLibsUri =
    "https://github.com/brechtsanders/winlibs_mingw/releases/download/$WinLibsVersion/$WinLibsArchiveName"
$WinLibsSha256 =
    "78eff1e2e804b6a6320c713f084b8f820c662104a24cea6a3bfcab82032bdd60"

$VcpkgCommit = "cd61e1e26a038e82d6550a3ebbe0fbbfe7da78e3"
$VcpkgArchiveName = "vcpkg-$VcpkgCommit.zip"
$VcpkgUri = "https://github.com/microsoft/vcpkg/archive/$VcpkgCommit.zip"
$VcpkgSha256 =
    "d6419cdfc922a65690e4852d7f0d7219de9ea0e091a17534342693b434971648"

function Invoke-Native {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [string[]]$ArgumentList = @()
    )

    & $FilePath @ArgumentList
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "Command failed with exit code $exitCode`: $FilePath"
    }
}

function Get-Archive {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Uri,
        [Parameter(Mandatory = $true)]
        [string]$Destination,
        [Parameter(Mandatory = $true)]
        [string]$Sha256
    )

    if (Test-Path -LiteralPath $Destination) {
        $existingHash = (Get-FileHash -Algorithm SHA256 `
                -LiteralPath $Destination).Hash.ToLowerInvariant()
        if ($existingHash -eq $Sha256) {
            Write-Host "Using cached $(Split-Path -Leaf $Destination)"
            return
        }
        Remove-Item -Force -LiteralPath $Destination
    }

    $partial = "$Destination.part"
    for ($attempt = 1; $attempt -le 3; ++$attempt) {
        Remove-Item -Force -ErrorAction SilentlyContinue -LiteralPath $partial
        try {
            Write-Host "Downloading $(Split-Path -Leaf $Destination) ($attempt/3)"
            Invoke-WebRequest -UseBasicParsing -Uri $Uri -OutFile $partial
            $actualHash = (Get-FileHash -Algorithm SHA256 `
                    -LiteralPath $partial).Hash.ToLowerInvariant()
            if ($actualHash -ne $Sha256) {
                throw "SHA-256 mismatch for $Uri"
            }
            Move-Item -LiteralPath $partial -Destination $Destination
            return
        } catch {
            Remove-Item -Force -ErrorAction SilentlyContinue `
                -LiteralPath $partial
            if ($attempt -eq 3) {
                throw
            }
            Start-Sleep -Seconds (2 * $attempt)
        }
    }
}

function Expand-ToolArchive {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Archive,
        [Parameter(Mandatory = $true)]
        [string]$Destination,
        [Parameter(Mandatory = $true)]
        [string]$RequiredRelativePath
    )

    $requiredPath = Join-Path $Destination $RequiredRelativePath
    if (Test-Path -LiteralPath $requiredPath) {
        return
    }

    $partial = "$Destination.part"
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
        -LiteralPath $partial
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
        -LiteralPath $Destination

    Write-Host "Extracting $(Split-Path -Leaf $Archive)"
    Expand-Archive -LiteralPath $Archive -DestinationPath $partial
    $partialRequired = Join-Path $partial $RequiredRelativePath
    if (-not (Test-Path -LiteralPath $partialRequired)) {
        throw "Archive did not contain $RequiredRelativePath"
    }

    Get-ChildItem -LiteralPath $partial -Recurse -File |
        ForEach-Object {
            Unblock-File -LiteralPath $_.FullName -ErrorAction SilentlyContinue
        }
    Move-Item -LiteralPath $partial -Destination $Destination
}

function Get-DllImports {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Objdump,
        [Parameter(Mandatory = $true)]
        [string]$Executable
    )

    $fileHeader = & $Objdump -f $Executable
    if ($LASTEXITCODE -ne 0 -or
        (($fileHeader -join "`n") -notmatch "file format pei-x86-64")) {
        throw "Build output is not a 64-bit Windows executable: $Executable"
    }

    $output = & $Objdump -p $Executable
    if ($LASTEXITCODE -ne 0) {
        throw "Could not inspect $Executable"
    }
    $imports = @()
    foreach ($line in $output) {
        if ($line -match "DLL Name:\s*(\S+)") {
            $imports += $Matches[1]
        }
    }
    if ($imports.Count -eq 0) {
        throw "No DLL imports were found in $Executable"
    }
    return $imports
}

function Publish-Artifacts {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Executable,
        [Parameter(Mandatory = $true)]
        [string[]]$Libraries
    )

    $partial = "$DistRoot.part"
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
        -LiteralPath $partial
    New-Item -ItemType Directory -Force -Path $partial | Out-Null
    Copy-Item -LiteralPath $Executable -Destination $partial
    foreach ($library in $Libraries) {
        Copy-Item -LiteralPath $library -Destination $partial
    }
    Copy-Item -LiteralPath (Join-Path $RepositoryRoot "LICENSE") `
        -Destination $partial

    $publishedExecutable = Join-Path $partial "forevervalidator.exe"
    $hash = (Get-FileHash -Algorithm SHA256 `
            -LiteralPath $publishedExecutable).Hash.ToLowerInvariant()
    Set-Content -Encoding ASCII `
        -LiteralPath (Join-Path $partial "forevervalidator.exe.sha256") `
        -Value "$hash  forevervalidator.exe"

    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
        -LiteralPath $DistRoot
    Move-Item -LiteralPath $partial -Destination $DistRoot
}

New-Item -ItemType Directory -Force -Path `
    $BuildRoot, $ArchiveRoot, $ToolRoot, $VcpkgDownloadRoot, `
    $VcpkgBinaryCache | Out-Null

if ($Clean) {
    Write-Host "Cleaning previous Windows build outputs"
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
        -LiteralPath $NativeBuildRoot
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
        -LiteralPath $VcpkgInstalledRoot
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
        -LiteralPath $DistRoot
}

$WinLibsArchive = Join-Path $ArchiveRoot $WinLibsArchiveName
$VcpkgArchive = Join-Path $ArchiveRoot $VcpkgArchiveName
Get-Archive -Uri $WinLibsUri -Destination $WinLibsArchive `
    -Sha256 $WinLibsSha256
Get-Archive -Uri $VcpkgUri -Destination $VcpkgArchive `
    -Sha256 $VcpkgSha256

$WinLibsRoot = Join-Path $ToolRoot "w-78eff1e2"
Expand-ToolArchive -Archive $WinLibsArchive -Destination $WinLibsRoot `
    -RequiredRelativePath "mingw64\bin\x86_64-w64-mingw32-g++.exe"
$WinLibsBin = Join-Path $WinLibsRoot "mingw64\bin"

$VcpkgBundleRoot = Join-Path $ToolRoot "v-d6419cdf"
$VcpkgRelativeRoot = "vcpkg-$VcpkgCommit"
Expand-ToolArchive -Archive $VcpkgArchive -Destination $VcpkgBundleRoot `
    -RequiredRelativePath "$VcpkgRelativeRoot\bootstrap-vcpkg.bat"
$VcpkgRoot = Join-Path $VcpkgBundleRoot $VcpkgRelativeRoot

$env:PATH = "$WinLibsBin;$env:PATH"
$env:VCPKG_DISABLE_METRICS = "1"
$env:VCPKG_DOWNLOADS = $VcpkgDownloadRoot
$env:VCPKG_DEFAULT_BINARY_CACHE = $VcpkgBinaryCache
$env:VCPKG_DEFAULT_TRIPLET = "x64-mingw-static"
$env:VCPKG_DEFAULT_HOST_TRIPLET = "x64-mingw-static"

$Vcpkg = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path -LiteralPath $Vcpkg)) {
    Write-Host "Bootstrapping pinned vcpkg"
    Invoke-Native -FilePath (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") `
        -ArgumentList @("-disableMetrics")
}
if (-not (Test-Path -LiteralPath $Vcpkg)) {
    throw "vcpkg bootstrap did not produce vcpkg.exe"
}

$Cmake = Join-Path $WinLibsBin "cmake.exe"
$Ninja = Join-Path $WinLibsBin "ninja.exe"
$Compiler = Join-Path $WinLibsBin "x86_64-w64-mingw32-g++.exe"
$Objdump = Join-Path $WinLibsBin "objdump.exe"
$Toolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"

Write-Host "Configuring ForeverValidator with MinGW-w64 GCC"
Invoke-Native -FilePath $Cmake -ArgumentList @(
    "--fresh",
    "-S", $RepositoryRoot,
    "-B", $NativeBuildRoot,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_CXX_COMPILER=$Compiler",
    "-DCMAKE_MAKE_PROGRAM=$Ninja",
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain",
    "-DVCPKG_TARGET_TRIPLET=x64-mingw-static",
    "-DVCPKG_HOST_TRIPLET=x64-mingw-static",
    "-DVCPKG_INSTALLED_DIR=$VcpkgInstalledRoot"
)

Write-Host "Building ForeverValidator"
Invoke-Native -FilePath $Cmake -ArgumentList @(
    "--build", $NativeBuildRoot,
    "--parallel", $Jobs.ToString()
)

$Executable = Join-Path $NativeBuildRoot "forevervalidator.exe"
$Libraries = @(
    (Join-Path $NativeBuildRoot "libforevervalidator_core.a"),
    (Join-Path $NativeBuildRoot "libforevervalidator_json.a"),
    (Join-Path $NativeBuildRoot "libforevervalidator_native.a")
)
foreach ($artifact in @($Executable) + $Libraries) {
    if (-not (Test-Path -LiteralPath $artifact)) {
        throw "Expected build artifact is missing: $artifact"
    }
}

$imports = Get-DllImports -Objdump $Objdump -Executable $Executable
$allowedSystemImports = @(
    "ADVAPI32.dll", "BCRYPT.dll", "CRYPT32.dll", "DNSAPI.dll",
    "GDI32.dll", "IPHLPAPI.dll", "KERNEL32.dll", "NCRYPT.dll",
    "NORMALIZ.dll", "NTDLL.dll", "OLE32.dll", "SECUR32.dll",
    "SHELL32.dll", "USER32.dll", "USERENV.dll", "WINMM.dll",
    "WS2_32.dll"
)
$unexpectedImports = @($imports | Where-Object {
    $_ -notmatch "^(api|ext)-ms-win-[a-z0-9-]+\.dll$" -and
    $allowedSystemImports -notcontains $_
})
if ($unexpectedImports.Count -ne 0) {
    throw "Non-system runtime dependency detected: $($unexpectedImports -join ', ')"
}

Publish-Artifacts -Executable $Executable -Libraries $Libraries
$FinalExecutable = Join-Path $DistRoot "forevervalidator.exe"
Write-Host ""
Write-Host "Windows build complete: $FinalExecutable" -ForegroundColor Green
