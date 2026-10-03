param()
$ErrorActionPreference = 'Stop'
$vendor = Join-Path (Split-Path -Parent $PSScriptRoot) 'src\vendor\ags'
$currentPath = Join-Path $vendor 'amd_ags.h'
$referencePath = Join-Path $vendor 'amd_ags_6_3_0_reference.h'
if ((Get-FileHash -LiteralPath $currentPath -Algorithm SHA256).Hash -ne '9CADB847D7828DC42555DAEA7FDB2E4B773339C81B738441A1F35E0AF1F45F70') {
    throw 'Pinned AMD AGS 6.3.1 header hash changed.'
}
if ((Get-FileHash -LiteralPath $referencePath -Algorithm SHA256).Hash -ne '6826D07FBA96819467F29ACBA1022346228410B29A0C3FA92F1C5C356723672B') {
    throw 'Pinned AMD AGS 6.3.0 reference header hash changed.'
}
$current = Get-Content -LiteralPath $currentPath -Raw
$reference = Get-Content -LiteralPath $referencePath -Raw
function Normalize([string]$declaration) {
    [regex]::Replace($declaration, '(?m)//.*$|\s+', '')
}
foreach ($type in @('AGSDeviceInfo', 'AGSGPUInfo', 'AGSConfiguration', 'AGSReturnCode')) {
    $pattern = 'typedef (?:struct|enum) ' + $type + '\s*\{[\s\S]*?\} ' + $type + ';'
    $a = [regex]::Match($current, $pattern).Value
    $b = [regex]::Match($reference, $pattern).Value
    if (-not $a -or -not $b -or (Normalize $a) -cne (Normalize $b)) { throw "AMD ABI differs: $type" }
}
foreach ($api in @('agsInitialize', 'agsGetGPUInfo', 'agsGetVersionNumber')) {
    $pattern = '(?m)^AMD_AGS_API [^\r\n]* ' + $api + '\([^\r\n]*;'
    $a = [regex]::Match($current, $pattern).Value
    $b = [regex]::Match($reference, $pattern).Value
    if (-not $a -or -not $b -or (Normalize $a) -cne (Normalize $b)) { throw "AMD API differs: $api" }
}
Write-Output 'PASS: pinned AMD AGS 6.3.0 and 6.3.1 hashes, GPU structure layouts and intercepted API declarations.'
