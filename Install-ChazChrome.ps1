#Requires -RunAsAdministrator
# Install Chaz Chrome.vst3 into the system VST3 folder from OneDrive\Chaz Chrome zip.
# Paste (elevated):
#   Start-Process powershell -Verb RunAs -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File', "$env:USERPROFILE\OneDrive\Chaz Chrome\Install-ChazChrome.ps1"
$ErrorActionPreference = 'Stop'

$TargetBundle = 'C:\Program Files\Common Files\VST3\Chaz Chrome.vst3'
$TargetBinary = Join-Path $TargetBundle 'Contents\x86_64-win\Chaz Chrome.vst3'
$ExpectedSha  = 'PLACEHOLDER_SHA256_REPLACE_AFTER_CI'
$ZipName      = 'ChazChrome-VST3-Windows-x64.zip'

function Find-Zip {
  $roots = New-Object System.Collections.Generic.List[string]
  if ($PSScriptRoot) { [void]$roots.Add($PSScriptRoot) }
  if ($env:OneDrive) { [void]$roots.Add((Join-Path $env:OneDrive 'Chaz Chrome')) }
  if ($env:OneDriveConsumer) { [void]$roots.Add((Join-Path $env:OneDriveConsumer 'Chaz Chrome')) }
  if ($env:OneDriveCommercial) { [void]$roots.Add((Join-Path $env:OneDriveCommercial 'Chaz Chrome')) }
  Get-ChildItem -LiteralPath $env:USERPROFILE -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -like 'OneDrive*' } |
    ForEach-Object { [void]$roots.Add((Join-Path $_.FullName 'Chaz Chrome')) }

  foreach ($r in ($roots | Select-Object -Unique)) {
    $p = Join-Path $r $ZipName
    if (Test-Path -LiteralPath $p) { return (Get-Item -LiteralPath $p).FullName }
  }
  return $null
}

$Zip = Find-Zip
if (-not $Zip) {
  throw "Missing $ZipName. Expected next to this script or under OneDrive\Chaz Chrome."
}

Write-Host "Zip: $Zip"
Unblock-File -LiteralPath $Zip -ErrorAction SilentlyContinue
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Zip).Hash.ToLowerInvariant()
if ($hash -ne $ExpectedSha) {
  throw "SHA256 mismatch.`n  got:      $hash`n  expected: $ExpectedSha"
}
Write-Host "SHA256 OK"

$Staging = Join-Path $env:TEMP ('ChazChrome-install-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $Staging | Out-Null
try {
  Expand-Archive -LiteralPath $Zip -DestinationPath $Staging -Force

  $Bundle = Get-ChildItem -LiteralPath $Staging -Recurse -Directory -Filter 'Chaz Chrome.vst3' -ErrorAction SilentlyContinue |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'Contents\x86_64-win\Chaz Chrome.vst3') } |
    Select-Object -First 1
  if (-not $Bundle) {
    throw "Chaz Chrome.vst3 bundle (with Contents\x86_64-win\Chaz Chrome.vst3) not found inside zip."
  }

  $DestParent = Split-Path $TargetBundle -Parent
  if (-not (Test-Path -LiteralPath $DestParent)) {
    New-Item -ItemType Directory -Path $DestParent -Force | Out-Null
  }
  if (Test-Path -LiteralPath $TargetBundle) {
    Remove-Item -LiteralPath $TargetBundle -Recurse -Force
  }
  Copy-Item -LiteralPath $Bundle.FullName -Destination $TargetBundle -Recurse -Force

  Get-ChildItem -LiteralPath $TargetBundle -Recurse -File | ForEach-Object {
    Unblock-File -LiteralPath $_.FullName -ErrorAction SilentlyContinue
  }

  if (-not (Test-Path -LiteralPath $TargetBinary)) {
    throw "Install incomplete — binary missing: $TargetBinary"
  }

  Write-Host "Installed bundle: $TargetBundle"
  Write-Host "Binary OK:        $TargetBinary"
  Write-Host "Test-Path result: $((Test-Path -LiteralPath $TargetBinary))"
  Write-Host "Next: FL Studio → Options → Manage plugins → Find plugins / Rescan"
  Write-Host "Load as Generator / Instrument (not Mixer FX)."
}
finally {
  Remove-Item -LiteralPath $Staging -Recurse -Force -ErrorAction SilentlyContinue
}
