# One-time public image download. No login, admin privileges, or Python needed.
# Run with Windows PowerShell 5.1 from the project root; restart the game afterward.
[CmdletBinding()]
param(
    [string]$Manifest = (Join-Path $PSScriptRoot '..\data\sets\unlimited-images.json'),
    [string]$CacheRoot = (Join-Path ([Environment]::GetFolderPath('ApplicationData')) 'FemboyLover\Clubhouse\cards'),
    [int]$Limit = 0,
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$userAgent = 'FemboyLover-CardImporter/0.2.2 (private tabletop; local cache)'
$manifestData = Get-Content -LiteralPath $Manifest -Raw -Encoding UTF8 | ConvertFrom-Json
if ($manifestData.set_id -ne '2ed') { throw 'Expected the Unlimited image manifest.' }
$destination = Join-Path $CacheRoot '2ed'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
$cards = @($manifestData.cards)
if ($Limit -gt 0) { $cards = @($cards | Select-Object -First $Limit) }
$downloaded = 0; $skipped = 0; $failures = @()
function Test-ImageFile([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return $false }
    if ((Get-Item -LiteralPath $Path).Length -lt 512) { return $false }
    $stream = [IO.File]::OpenRead($Path)
    try {
        $bytes = New-Object byte[] 8
        $null = $stream.Read($bytes, 0, 8)
        return (($bytes[0] -eq 255 -and $bytes[1] -eq 216 -and $bytes[2] -eq 255) -or
                ($bytes[0] -eq 137 -and $bytes[1] -eq 80 -and $bytes[2] -eq 78 -and $bytes[3] -eq 71))
    } finally { $stream.Dispose() }
}
function Invoke-Paced([scriptblock]$Action) {
    for ($attempt = 0; $attempt -lt 3; $attempt++) {
        Start-Sleep -Milliseconds 250
        try { return (& $Action) }
        catch {
            $code = 0
            if ($_.Exception.Response) { $code = [int]$_.Exception.Response.StatusCode }
            if ($code -eq 401 -or $code -eq 403 -or $code -eq 404 -or $attempt -eq 2) { throw }
            if ($code -eq 429) { Start-Sleep -Seconds 15 }
            else { Start-Sleep -Seconds (2 + $attempt * 2) }
        }
    }
}
for ($i = 0; $i -lt $cards.Count; $i++) {
    $entry = $cards[$i]
    $id = [string]$entry.scryfall_id
    if ($id -notmatch '^[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}$') { throw 'Invalid printing ID in manifest.' }
    $target = Join-Path $destination ($id + '.jpg')
    $partial = $target + '.part'
    Write-Progress -Activity 'Caching Unlimited card images' -Status $entry.name -PercentComplete (100 * $i / [Math]::Max(1, $cards.Count))
    if (-not $Force -and (Test-ImageFile $target)) { $skipped++; continue }
    try {
        $url = 'https://api.scryfall.com/cards/' + $id
        $card = Invoke-Paced { Invoke-RestMethod -Uri $url -UserAgent $userAgent -Headers @{ Accept = 'application/json' } -TimeoutSec 30 }
        if ($card.id -ne $id -or $card.set -ne '2ed') { throw 'Printing identity/set mismatch.' }
        if ($card.image_status -eq 'missing' -or $card.image_status -eq 'placeholder') { throw 'Original image is unavailable for this printing.' }
        $imageUrl = $card.image_uris.large
        if (-not $imageUrl) { $imageUrl = $card.image_uris.normal }
        if (-not $imageUrl) { $imageUrl = $card.image_uris.png }
        if (-not $imageUrl) { throw 'No full-card image URL was returned.' }
        $imageUri = [Uri]$imageUrl
        if ($imageUri.Scheme -ne 'https' -or $imageUri.Host -notmatch '(^|\.)scryfall\.(com|io)$') { throw 'Unexpected image host.' }
        Invoke-Paced { Invoke-WebRequest -Uri $imageUrl -UserAgent $userAgent -Headers @{ Accept = 'image/*' } -UseBasicParsing -TimeoutSec 60 -OutFile $partial } | Out-Null
        if (-not (Test-ImageFile $partial)) { throw 'Downloaded response is not a JPEG/PNG image.' }
        Move-Item -LiteralPath $partial -Destination $target -Force
        $downloaded++
        Write-Host ('[{0}/{1}] {2}' -f ($i + 1), $cards.Count, $entry.name)
    } catch {
        $failures += [PSCustomObject]@{ name = $entry.name; id = $id; error = $_.Exception.Message }
        Write-Warning ($entry.name + ': ' + $_.Exception.Message)
        $code = 0
        if ($_.Exception.Response) { $code = [int]$_.Exception.Response.StatusCode }
        if ($code -eq 401 -or $code -eq 403) { Write-Warning 'Access denied; stopping requests. Existing cached images remain intact.'; break }
    } finally {
        if (Test-Path -LiteralPath $partial) { Remove-Item -LiteralPath $partial -Force }
    }
}
Write-Progress -Activity 'Caching Unlimited card images' -Completed
$report = [PSCustomObject]@{ set = '2ed'; downloaded = $downloaded; already_cached = $skipped; failed = $failures.Count; failures = $failures }
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $destination 'download-report.json') -Encoding UTF8
Write-Host ('Downloaded: {0}; cached: {1}; failed: {2}' -f $downloaded, $skipped, $failures.Count)
Write-Host ('Image folder: ' + $destination)
Write-Host 'Restart the game to load new images. Rerun to resume; use -Force to replace cached files.'
if ($failures.Count -gt 0) { exit 1 }
