param (
    [string]$InstallerPath
)

if (-Not (Test-Path $InstallerPath)) {
    Write-Error "Installer not found at $InstallerPath"
    exit 1
}

$file = Get-Item $InstallerPath
$sizeMB = $file.Length / 1MB

Write-Output "Installer size: $([math]::Round($sizeMB, 2)) MB"

if ($sizeMB -gt 120) {
    Write-Host "ERROR: PDF Elite installer is $([math]::Round($sizeMB, 2)) MB."
    Write-Host "Maximum allowed size: 120 MB."
    Write-Host "Difference: $([math]::Round($sizeMB - 120, 2)) MB"
    Write-Error "Build rejected."
    exit 1
} else {
    Write-Output "Size check passed: $([math]::Round($sizeMB, 2)) MB"
    exit 0
}
