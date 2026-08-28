[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$UnityProjectPath
)

$ErrorActionPreference = "Stop"
$source = Split-Path -Parent $PSScriptRoot
$destination = Join-Path (Join-Path (Resolve-Path $UnityProjectPath) "Packages") "UnityVulkanDriverGuard"
if ([IO.Path]::GetFullPath($destination).TrimEnd('\') -eq [IO.Path]::GetFullPath($source).TrimEnd('\')) {
    throw "The Unity project cannot be the driver guard repository itself."
}

New-Item -ItemType Directory -Force $destination | Out-Null
Get-ChildItem $source -Force |
    Where-Object { $_.Name -notin @('.git', 'build', '.github', '.gitignore') } |
    Copy-Item -Destination $destination -Recurse -Force

Write-Host "Installed Unity Vulkan Driver Guard at $destination"

