# PowerShell Build Script for SPF Framework

# --- Configuration ---
$ErrorActionPreference = "Stop" # Exit script on error

# Define project paths
$ProjectRoot = Get-Location
$InstallerScript = Join-Path $ProjectRoot "installer.iss"
$DistDir = Join-Path $ProjectRoot "dist"
$StagingDir = Join-Path $ProjectRoot "staging"
$InstallerOutputDir = Join-Path $ProjectRoot "Output"
$ReadmeFile = Join-Path $ProjectRoot "readme.txt"
$ApiSourceDir = Join-Path $ProjectRoot "..\include\SPF\SPF_API"
$PluginTemplateSourceDir = Join-Path $DistDir "MyPlugin"

# Inno Setup Compiler path (update if it's not in your system PATH)
$ISCC = "E:\ProgramsFile\Inno Setup 6\ISCC.exe"

# --- Script ---

# 1. Get Version from Installer Script
Write-Host "Reading version from installer script..."
$AppVersion = (Get-Content $InstallerScript | Select-String -Pattern '#define MyAppVersion "(.*)"').Matches.Groups[1].Value
if (-not $AppVersion) {
    Write-Error "Could not determine app version from '$InstallerScript'. Make sure '#define MyAppVersion ""x.y.z""' is present."
    exit 1
}
Write-Host "Version found: $AppVersion"
$FinalZipName = "SPF-Framework_v$($AppVersion).zip"
$FinalZipPath = Join-Path $InstallerOutputDir $FinalZipName
$ApiZipName = "SPF_API_v$($AppVersion).zip"
$ApiZipPath = Join-Path $InstallerOutputDir $ApiZipName
$PluginTemplateZipName = "MyPlugin_Template_v$($AppVersion).zip"
$PluginTemplateZipPath = Join-Path $InstallerOutputDir $PluginTemplateZipName

# 2. Compile with Inno Setup
Write-Host "Running Inno Setup Compiler..."
if (-not (Test-Path $ISCC)) {
    Write-Error "Inno Setup Compiler not found at '$ISCC'. Please update the path in the script or add it to your system's PATH."
    exit 1
}
& $ISCC $InstallerScript

# 3. Prepare Staging Area
Write-Host "Preparing staging area for packaging..."
if (Test-Path $StagingDir) {
    Remove-Item -Recurse -Force $StagingDir
}
New-Item -ItemType Directory -Path $StagingDir | Out-Null

$InstallerExe = Join-Path $InstallerOutputDir "spf-framework.exe"
if (-not (Test-Path $InstallerExe)) {
    Write-Error "Installer executable not found at '$InstallerExe'. Compilation may have failed."
    exit 1
}

# 4. Copy Files to Staging
Write-Host "Copying files to staging directory..."
Copy-Item -Path $InstallerExe -Destination $StagingDir
Copy-Item -Path $ReadmeFile -Destination $StagingDir

$ManualInstallDir = Join-Path $StagingDir "manualInstall"
New-Item -ItemType Directory -Path $ManualInstallDir | Out-Null
Copy-Item -Path (Join-Path $DistDir "*") -Destination $ManualInstallDir -Recurse

# 5. Create Main ZIP Archive
Write-Host "Creating main ZIP archive: $FinalZipName"
if (Test-Path $FinalZipPath) {
    Remove-Item $FinalZipPath
}
Compress-Archive -Path (Join-Path $StagingDir "*") -DestinationPath $FinalZipPath -CompressionLevel Optimal

# 6. Create API ZIP Archive
Write-Host "Creating API ZIP archive: $ApiZipName"
if (-not (Test-Path $ApiSourceDir)) {
    Write-Error "API source directory not found at '$ApiSourceDir'."
    exit 1
}
if (Test-Path $ApiZipPath) {
    Remove-Item $ApiZipPath
}
Compress-Archive -Path (Join-Path $ApiSourceDir "*.h") -DestinationPath $ApiZipPath -CompressionLevel Optimal

# 7. Create Plugin Template ZIP Archive
Write-Host "Creating Plugin Template ZIP archive: $PluginTemplateZipName"
if (-not (Test-Path $PluginTemplateSourceDir)) {
    Write-Error "Plugin Template source directory not found at '$PluginTemplateSourceDir'."
    exit 1
}
if (Test-Path $PluginTemplateZipPath) {
    Remove-Item $PluginTemplateZipPath
}
Compress-Archive -Path $PluginTemplateSourceDir -DestinationPath $PluginTemplateZipPath -CompressionLevel Optimal

# 8. Cleanup
Write-Host "Cleaning up staging directory..."
Remove-Item -Recurse -Force $StagingDir

Write-Host "Build process complete!"
Write-Host "Output available at: $InstallerOutputDir"
Write-Host "- $FinalZipName"
Write-Host "- $ApiZipName"
Write-Host "- $PluginTemplateZipName"
