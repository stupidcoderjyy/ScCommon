# break_line.ps1
#
# Hard-wrap the Obsidian level notes to a fixed display width.
#
#   - continuation lines use a hanging indent, i.e. the first character of a
#     continuation aligns with the first character of the previous line body
#     (for a list item the body starts after the "1. " / "- " marker)
#   - CJK line-breaking rules: no closing punctuation at line start, no
#     opening punctuation at line end
#   - inline code spans (`...`) and wikilinks ([[...]]) are never split
#   - very long identifiers are not cut; the whole segment is moved to the
#     next line instead
#   - table rows, headings, horizontal rules and blank lines are left as-is
#   - files are written as UTF-8 without BOM and LF only, so running the
#     script twice is a no-op
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File break_line.ps1
#   powershell -ExecutionPolicy Bypass -File break_line.ps1 -Root <dir> -Width 100

param(
    [string]$Root = $PSScriptRoot,
    [int]$Width = 100
)

$ErrorActionPreference = 'Stop'

function Test-WideChar([char]$ch) {
    $c = [int]$ch
    return (
        ($c -ge 0x1100 -and $c -le 0x115F) -or
        ($c -ge 0x2E80 -and $c -le 0x303E) -or
        ($c -ge 0x3041 -and $c -le 0x33FF) -or
        ($c -ge 0x3400 -and $c -le 0x4DBF) -or
        ($c -ge 0x4E00 -and $c -le 0x9FFF) -or
        ($c -ge 0xA000 -and $c -le 0xA4CF) -or
        ($c -ge 0xAC00 -and $c -le 0xD7A3) -or
        ($c -ge 0xF900 -and $c -le 0xFAFF) -or
        ($c -ge 0xFE30 -and $c -le 0xFE6F) -or
        ($c -ge 0xFF00 -and $c -le 0xFF60) -or
        ($c -ge 0xFFE0 -and $c -le 0xFFE6)
    )
}

function Get-DisplayWidth([string]$text) {
    $w = 0
    for ($i = 0; $i -lt $text.Length; $i++) {
        if (Test-WideChar $text[$i]) { $w += 2 } else { $w += 1 }
    }
    return $w
}

# Characters that must not start a wrapped line (CJK closing punctuation, etc.).
$NoLineStart = -join @(
    [char]0xFF0C, [char]0x3002, [char]0x3001, [char]0xFF1B, [char]0xFF1A,
    [char]0xFF1F, [char]0xFF01, [char]0xFF09, [char]0x3011, [char]0x300B,
    [char]0x300D, [char]0x300F, [char]0x201D, [char]0x2019, [char]0x2026,
    [char]0x00B7, [char]0x2030, [char]0x2103,
    [char]0x002C, [char]0x002E, [char]0x003B, [char]0x003A, [char]0x0021,
    [char]0x003F, [char]0x0029, [char]0x005D, [char]0x007D
)

# Characters that must not end a wrapped line (CJK opening punctuation, etc.).
$NoLineEnd = -join @(
    [char]0xFF08, [char]0x3010, [char]0x300A, [char]0x300C, [char]0x300E,
    [char]0x201C, [char]0x2018, [char]0x0028, [char]0x005B, [char]0x007B
)

# ASCII separators after which a break is acceptable inside a long token.
$BreakAfter = -join @(
    [char]0x002F, [char]0x003D, [char]0x002B, [char]0x003E, [char]0x002C,
    [char]0x003B
)

function Get-LineKind([string]$line) {
    if ($line.Trim().Length -eq 0) { return 'blank' }
    if ($line -match '^#{1,6}\s') { return 'heading' }
    if ($line -match '^\s*\|') { return 'table' }
    if ($line -match '^-{3,}\s*$') { return 'hr' }
    if ($line.StartsWith([string][char]0x2190)) { return 'backlink' }
    if ($line -match '^\s*(\d{1,4}\.\s|[-*+]\s)') { return 'item' }
    return 'text'
}

function Format-WrappedLine([string]$line) {
    $kind = Get-LineKind $line
    if ($kind -ne 'text' -and $kind -ne 'item') { return , $line }

    $indent = [regex]::Match($line, '^\s*').Value
    $body = $line.Substring($indent.Length)
    if ($body.Length -eq 0) { return , $line }

    # Hanging indent: continuation aligns with the body start of a list item.
    $contIndent = $indent
    $marker = [regex]::Match($body, '^(\d{1,4}\.\s+|[-*+]\s+)')
    if ($marker.Success) { $contIndent = $indent + (' ' * $marker.Value.Length) }

    $n = $body.Length
    $allow = New-Object 'bool[]' $n
    $ticks = 0
    $linkDepth = 0
    for ($i = 0; $i -lt $n; $i++) {
        $ok = ((($ticks % 2) -eq 0) -and ($linkDepth -le 0))
        if ($ok -and $i -gt 0) {
            $prev = $body[$i - 1]
            $cur = $body[$i]
            if ($NoLineStart.IndexOf($cur) -ge 0) { $ok = $false }
            elseif ($NoLineEnd.IndexOf($prev) -ge 0) { $ok = $false }
            elseif ((Test-WideChar $prev) -or (Test-WideChar $cur) -or
                    $prev -eq ' ' -or ($BreakAfter.IndexOf($prev) -ge 0)) { $ok = $true }
            else { $ok = $false }
        }
        $allow[$i] = $ok

        $ch = $body[$i]
        if ($ch -eq '`') {
            $ticks++
        } elseif ($ch -eq '[' -and ($i + 1) -lt $n -and $body[$i + 1] -eq '[') {
            $linkDepth++
        } elseif ($ch -eq ']' -and ($i + 1) -lt $n -and $body[$i + 1] -eq ']') {
            if ($linkDepth -gt 0) { $linkDepth-- }
        }
    }

    $result = New-Object System.Collections.Generic.List[string]
    $start = 0
    $first = $true
    while ($start -lt $n) {
        $prefix = if ($first) { $indent } else { $contIndent }
        $avail = $Width - $prefix.Length
        if ($avail -lt 12) { $avail = 12 }

        $i = $start
        $w = 0
        $maxFit = $start
        while ($i -lt $n) {
            $cw = if (Test-WideChar $body[$i]) { 2 } else { 1 }
            if (($w + $cw) -gt $avail) { break }
            $w += $cw
            $i++
            $maxFit = $i
        }

        $end = $n
        if ($maxFit -lt $n) {
            $best = -1
            for ($j = $maxFit; $j -gt $start; $j--) {
                if ($allow[$j]) { $best = $j; break }
            }
            if ($best -gt $start) {
                $end = $best
            } else {
                # No safe break within the width: move the whole segment to the
                # next break opportunity instead of cutting an identifier.
                $next = -1
                for ($j = $maxFit + 1; $j -le $n; $j++) {
                    if (($j -ge $n) -or $allow[$j]) { $next = $j; break }
                }
                if ($next -gt $start) { $end = $next } else { $end = $n }
            }
        }

        $segment = $body.Substring($start, $end - $start).TrimEnd()
        $result.Add($prefix + $segment)
        $start = $end
        while ($start -lt $n -and $body[$start] -eq ' ') { $start++ }
        $first = $false
    }
    return $result.ToArray()
}

function Expand-LogicalLines([string[]]$lines) {
    $result = New-Object System.Collections.Generic.List[string]
    $lastWrappable = $false
    foreach ($raw in $lines) {
        $line = $raw
        if ($line.EndsWith("`r")) { $line = $line.Substring(0, $line.Length - 1) }
        $kind = Get-LineKind $line
        if ($kind -eq 'text') {
            if ($lastWrappable -and $result.Count -gt 0) {
                $prev = $result[$result.Count - 1]
                $pch = $prev[$prev.Length - 1]
                $cch = $line.TrimStart()[0]
                $joiner = ' '
                if ((Test-WideChar $pch) -and (Test-WideChar $cch)) { $joiner = '' }
                $result[$result.Count - 1] = $prev + $joiner + $line.TrimStart()
            } else {
                $result.Add($line)
            }
            $lastWrappable = $true
        } elseif ($kind -eq 'item') {
            $result.Add($line)
            $lastWrappable = $true
        } else {
            $result.Add($line)
            $lastWrappable = $false
        }
    }
    return $result.ToArray()
}

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$files = @(Get-ChildItem -Path $Root -Recurse -File -Filter *.md)
foreach ($file in $files) {
    $text = [System.IO.File]::ReadAllText($file.FullName, [System.Text.Encoding]::UTF8)
    $logical = Expand-LogicalLines ($text -split "`n", -1)
    $out = New-Object System.Collections.Generic.List[string]
    foreach ($line in $logical) {
        foreach ($wrapped in (Format-WrappedLine $line)) { $out.Add($wrapped) }
    }
    [System.IO.File]::WriteAllText($file.FullName, ($out -join "`n"), $utf8NoBom)
}
Write-Host ("break_line: wrapped {0} markdown file(s) under {1} (width {2})" -f $files.Count, $Root, $Width)
