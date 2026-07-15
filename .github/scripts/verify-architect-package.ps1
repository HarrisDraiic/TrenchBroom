param(
  [Parameter(Mandatory = $false)]
  [string] $BuildDirectory = "cmakebuild"
)

$ErrorActionPreference = "Stop"

$archives = @(
  Get-ChildItem -LiteralPath $BuildDirectory -File -Filter "TrenchBroomArchitect-*.zip"
)

if ($archives.Count -ne 1) {
  throw "Expected one TrenchBroom Architect ZIP in '$BuildDirectory', found $($archives.Count)."
}

$archivePath = $archives[0].FullName
$checksumPath = "$archivePath.md5"
if (-not (Test-Path -LiteralPath $checksumPath -PathType Leaf)) {
  throw "Package checksum is missing: $checksumPath"
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead($archivePath)
try {
  $entryNames = @(
    $archive.Entries | ForEach-Object { $_.FullName.Replace("\", "/") }
  )

  foreach ($requiredEntry in @(
    "TrenchBroomArchitect.exe",
    "TrenchBroomArchitectRuntime.exe",
    "LICENSE.txt"
  )) {
    if ($requiredEntry -notin $entryNames) {
      throw "Package is missing required entry '$requiredEntry'."
    }
  }

  if ("TrenchBroom.exe" -in $entryNames) {
    throw "Package contains the stock executable name and is not side-by-side isolated."
  }

  $forbiddenPatterns = @(
    '(^|/)\.env($|\.)',
    '(^|/)(credentials?|secrets?)(\.|/|$)',
    '(^|/)(endpoint|session)(-descriptor)?\.json$',
    '\.(pem|p12|pfx|key)$'
  )

  foreach ($entryName in $entryNames) {
    foreach ($pattern in $forbiddenPatterns) {
      if ($entryName -match $pattern) {
        throw "Package contains forbidden credential or session material: $entryName"
      }
    }
  }
}
finally {
  $archive.Dispose()
}

Write-Host "Verified Architect package: $archivePath"
Write-Host "Verified checksum: $checksumPath"
