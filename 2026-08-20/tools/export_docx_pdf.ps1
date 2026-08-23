param(
    [Parameter(Mandatory = $true)][string]$InputPath,
    [Parameter(Mandatory = $true)][string]$OutputPath
)

$word = $null
$doc = $null
try {
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $word.DisplayAlerts = 0
    $doc = $word.Documents.Open($InputPath, $false, $true)
    $doc.ExportAsFixedFormat($OutputPath, 17)
}
finally {
    if ($null -ne $doc) { $doc.Close($false) }
    if ($null -ne $word) { $word.Quit() }
    [System.GC]::Collect()
    [System.GC]::WaitForPendingFinalizers()
}
