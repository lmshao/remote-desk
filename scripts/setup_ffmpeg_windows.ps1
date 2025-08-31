# FFmpeg Windows Dependencies Download Script
# Downloads and extracts FFmpeg development libraries for Windows

param(
    [string]$Version = "8.0",
    [string]$Architecture = "x64", # x64 or x86
    [string]$OutputDir = "$PSScriptRoot\..\ffmpeg",
    [switch]$Force = $false
)

$ErrorActionPreference = "Stop"

# Color output functions
function Write-Info { param($Message) Write-Host "[INFO] $Message" -ForegroundColor Blue }
function Write-Success { param($Message) Write-Host "[SUCCESS] $Message" -ForegroundColor Green }
function Write-Warning { param($Message) Write-Host "[WARNING] $Message" -ForegroundColor Yellow }
function Write-Error { param($Message) Write-Host "[ERROR] $Message" -ForegroundColor Red }

function Download-FFmpeg {
    Write-Info "FFmpeg Windows Dependencies Downloader"
    Write-Info "======================================"
    Write-Info "Version: $Version"
    Write-Info "Architecture: $Architecture"
    Write-Info "Output Directory: $OutputDir"
    
    # Create output directory
    if (Test-Path $OutputDir) {
        if ($Force) {
            Write-Warning "Removing existing FFmpeg directory..."
            Remove-Item $OutputDir -Recurse -Force
        } else {
            Write-Warning "FFmpeg directory already exists. Use -Force to overwrite."
            return
        }
    }
    
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    
    # Download URLs - using gyan.dev builds (popular Windows FFmpeg distribution)
    $BaseUrl = "https://www.gyan.dev/ffmpeg/builds/packages"
    $PackageName = "ffmpeg-$Version-full_build-shared"
    $ArchiveFile = "$PackageName.7z"
    $DownloadUrl = "$BaseUrl/$ArchiveFile"
    $TempDir = "$env:TEMP\ffmpeg-download"
    $ArchivePath = "$TempDir\$ArchiveFile"
    
    Write-Info "Download URL: $DownloadUrl"
    
    # Create temp directory
    if (Test-Path $TempDir) {
        Remove-Item $TempDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $TempDir -Force | Out-Null
    
    try {
        # Check if 7-Zip is available
        $7zipPath = $null
        $7zipPaths = @(
            "${env:ProgramFiles}\7-Zip\7z.exe",
            "${env:ProgramFiles(x86)}\7-Zip\7z.exe",
            "7z.exe"  # Try PATH
        )
        
        foreach ($path in $7zipPaths) {
            if (Get-Command $path -ErrorAction SilentlyContinue) {
                $7zipPath = $path
                break
            }
        }
        
        if (-not $7zipPath) {
            Write-Warning "7-Zip not found. Attempting to use PowerShell's Expand-Archive (may not work with .7z files)"
            Write-Info "To install 7-Zip, download from: https://www.7-zip.org/download.html"
        }
        
        # Download FFmpeg
        Write-Info "Downloading FFmpeg $Version..."
        Invoke-WebRequest -Uri $DownloadUrl -OutFile $ArchivePath -UseBasicParsing
        Write-Success "Download completed"
        
        # Extract archive
        Write-Info "Extracting FFmpeg archive..."
        if ($7zipPath) {
            # Use 7-Zip for extraction
            Write-Info "Using 7-Zip for extraction"
            & $7zipPath x "$ArchivePath" "-o$TempDir" -y
            if ($LASTEXITCODE -ne 0) {
                throw "7-Zip extraction failed with exit code $LASTEXITCODE"
            }
        } else {
            # Fallback to PowerShell (may fail with .7z)
            Write-Warning "Attempting extraction with PowerShell (may fail with .7z format)"
            try {
                Expand-Archive -Path $ArchivePath -DestinationPath $TempDir -Force
            } catch {
                Write-Error "PowerShell cannot extract .7z files. Please install 7-Zip from https://www.7-zip.org/download.html"
                throw "Extraction failed: $($_.Exception.Message)"
            }
        }
        
        # Move files to target location
        $ExtractedDir = "$TempDir\$PackageName"
        if (Test-Path $ExtractedDir) {
            # Copy include files
            Write-Info "Copying include files..."
            Copy-Item "$ExtractedDir\include" -Destination "$OutputDir\include" -Recurse -Force
            
            # Copy library files
            Write-Info "Copying library files..."
            Copy-Item "$ExtractedDir\lib" -Destination "$OutputDir\lib" -Recurse -Force
            
            # Copy DLL files
            Write-Info "Copying DLL files..."
            Copy-Item "$ExtractedDir\bin" -Destination "$OutputDir\dll" -Recurse -Force
            
            # Create version file
            Write-Info "Creating version file..."
            "FFmpeg $Version ($Architecture) - Downloaded $(Get-Date)" | Out-File "$OutputDir\version$Version.txt"
            
            Write-Success "FFmpeg $Version extracted successfully to $OutputDir"
        } else {
            Write-Error "Extracted directory not found: $ExtractedDir"
            exit 1
        }
        
    } catch {
        Write-Error "Failed to download or extract FFmpeg: $($_.Exception.Message)"
        exit 1
    } finally {
        # Cleanup
        if (Test-Path $TempDir) {
            Remove-Item $TempDir -Recurse -Force
        }
    }
    
    Write-Info ""
    Write-Info "Next steps:"
    Write-Info "1. Configure your project: cmake .."
    Write-Info "2. Build your project: cmake --build ."
    Write-Info ""
    Write-Success "FFmpeg Windows setup completed!"
}

# Show usage if help requested
if ($args -contains "-h" -or $args -contains "--help") {
    Write-Host "Usage: .\download_ffmpeg_windows.ps1 [OPTIONS]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Version <string>      FFmpeg version to download (default: 8.0)"
    Write-Host "  -Architecture <string> Target architecture: x64 or x86 (default: x64)"
    Write-Host "  -OutputDir <string>    Output directory (default: ..\ffmpeg)"
    Write-Host "  -Force                 Overwrite existing FFmpeg directory"
    Write-Host "  -h, --help            Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\download_ffmpeg_windows.ps1"
    Write-Host "  .\download_ffmpeg_windows.ps1 -Version 7.1 -Force"
    Write-Host "  .\download_ffmpeg_windows.ps1 -Architecture x86"
    exit 0
}

# Run main function
Download-FFmpeg