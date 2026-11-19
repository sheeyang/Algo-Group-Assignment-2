<#
.SYNOPSIS
Build the C3 evidence driver and render DP-versus-EK report figures.
.DESCRIPTION
Runs c3_evidence.cpp against the existing MSVC Release seam library, then
labels the PNG panels and writes the illustrative network SVG. Existing
figure and CSV evidence is overwritten. The source photos are not modified.
PowerShell uses native comment-based help; C++ APIs use Doxygen blocks.
.PARAMETER BuildDir
Repository-relative directory containing an already built MSVC Release
seamlib.lib and CMakeCache.txt. Defaults to build/release.
.EXAMPLE
powershell -NoProfile -ExecutionPolicy Bypass -File tools/generate-c3-evidence.ps1
.NOTES
Requires Windows, Visual Studio C++ tools, and System.Drawing. Errors stop
the script; the caller's original working directory is restored on exit.
#>
param([string]$BuildDir = 'build/release')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $build = [IO.Path]::GetFullPath((Join-Path $root $BuildDir))
    if (!(Test-Path -LiteralPath "$build/src/seamlib.lib")) {
        throw 'Build seamlib in Release mode with MSVC first; see docs/report/figures/c3/README.md.'
    }
    if (!(Select-String -LiteralPath "$build/CMakeCache.txt" -Pattern '^CMAKE_BUILD_TYPE:STRING=Release$' -Quiet)) {
        throw 'Evidence timings require a Release build.'
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$installation) { throw 'MSVC C++ tools were not found.' }
    # Keep compiler artifacts out of the source and figure directories.
    $commands = @"
@echo off
call "$installation\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /O2 /MD /DNDEBUG /I "$root/src" "$root/tools/c3_evidence.cpp" "$build/src/seamlib.lib" /Fo:"$build/c3-evidence.obj" /Fe:"$build/c3-evidence.exe"
if errorlevel 1 exit /b 1
"$build/c3-evidence.exe"
"@
    $commands | Set-Content -LiteralPath "$build/c3-evidence.cmd" -Encoding Ascii
    & "$build/c3-evidence.cmd"
    if ($LASTEXITCODE -ne 0) { throw 'Evidence generation failed.' }

    Add-Type -AssemblyName System.Drawing
    foreach ($name in @('portrait', 'building')) {
        $bitmap = New-Object Drawing.Bitmap 1400, 300
        $g = [Drawing.Graphics]::FromImage($bitmap)
        $title = New-Object Drawing.Font 'Arial', 18
        $label = New-Object Drawing.Font 'Arial', 12
        try {
            $g.Clear([Drawing.Color]::White)
            $g.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
            $g.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
            $g.DrawString("$name : DP versus Edmonds-Karp", $title, [Drawing.Brushes]::Black, 15, 10)
            $suffixes = @('input', 'dp-first', 'ek-first', 'dp-result', 'ek-result')
            $labels = @('Input (64 x 43)', 'DP first seam (red)', 'EK first seam (red)', 'DP result (54 x 43)', 'EK result (54 x 43)')
            for ($i = 0; $i -lt 5; $i++) {
                $g.DrawString($labels[$i], $label, [Drawing.Brushes]::Black, [single](15 + $i*280), 53)
                $panel = [Drawing.Image]::FromFile((Join-Path $root "docs/report/figures/c3/$name-$($suffixes[$i]).png"))
                try {
                    $rect = New-Object Drawing.Rectangle ([int]($i*280 + (280-$panel.Width*4)/2)), 85, ($panel.Width*4), ($panel.Height*4)
                    $g.DrawImage($panel, $rect, 0, 0, $panel.Width, $panel.Height, [Drawing.GraphicsUnit]::Pixel)
                } finally { $panel.Dispose() }
            }
            $g.DrawString('Ten vertical seams removed. All panels enlarged 4x with nearest-neighbour display scaling.', $label, [Drawing.Brushes]::Black, 15, 273)
            $bitmap.Save((Join-Path $root "docs/report/figures/c3/dp-vs-ek-$name.png"), [Drawing.Imaging.ImageFormat]::Png)
        } finally { $g.Dispose(); $bitmap.Dispose(); $title.Dispose(); $label.Dispose() }
    }

    # Draw the actual four-row network, including the virtual column. The
    # example's unique optimum is the connected seam [1,1,2,1].
    $svg = [Text.StringBuilder]::new()
    [void]$svg.AppendLine('<svg xmlns="http://www.w3.org/2000/svg" width="1160" height="735" viewBox="0 0 1160 735"><rect width="1160" height="735" fill="white"/><defs><marker id="arrow" markerWidth="7" markerHeight="7" refX="6" refY="3" orient="auto"><path d="M0 0 L6 3 L0 6" fill="context-stroke"/></marker></defs><g font-family="Arial,sans-serif">')
    [void]$svg.AppendLine('<text x="25" y="35" font-size="24">Minimum-cut network: four rows, four real columns and one virtual column</text><text x="25" y="65" font-size="17">Blue nodes = source side S; grey nodes = sink side T. Red forward edges cross the minimum cut.</text>')
    $seam = @(1,1,2,1)
    # Constraints behind the finite edges and nodes.
    for ($y=0; $y -lt 4; $y++) {
        $py=160+$y*130
        [void]$svg.AppendLine("<path d='M55 355 L120 $py M960 $py L1100 355' stroke='#777' fill='none' marker-end='url(#arrow)'/>")
        for ($x=1; $x -le 4; $x++) {
            $px=140+$x*200; $left=$px-200
            [void]$svg.AppendLine("<path d='M$($px-18) $($py+20) Q$($px-100) $($py+57) $($left+18) $($py+20)' stroke='#39759d' fill='none' stroke-dasharray='5 3' marker-end='url(#arrow)'/>")
            foreach ($other in @(($y-1),($y+1))) {
                if ($other -ge 0 -and $other -lt 4) {
                    $oy=160+$other*130
                    [void]$svg.AppendLine("<path d='M$($px-15) $($py+5) L$($left+15) $($oy-5)' stroke='#39759d' opacity='.36' fill='none' stroke-dasharray='5 3' marker-end='url(#arrow)'/>")
                }
            }
        }
    }
    for ($y=0; $y -lt 4; $y++) {
        $py=160+$y*130
        for ($x=0; $x -lt 4; $x++) {
            $px=140+$x*200; $colour=if($x -eq $seam[$y]){'#d21c1c'}else{'#222'}
            $capacity=if($x -eq $seam[$y]){1000}else{9000}
            [void]$svg.AppendLine("<path d='M$($px+23) $py L$($px+177) $py' stroke='$colour' stroke-width='2' marker-end='url(#arrow)'/><text x='$($px+75)' y='$($py-12)' font-size='16' fill='$colour'>$capacity</text>")
        }
        for ($x=0; $x -le 4; $x++) {
            $px=140+$x*200; $fill=if($x -le $seam[$y]){'#c9e8ff'}else{'#eeeeee'}
            $text=if($x -eq 4){'v'}else{"$x"}
            [void]$svg.AppendLine("<circle cx='$px' cy='$py' r='22' fill='$fill' stroke='#333'/><text x='$($px-5)' y='$($py+6)' font-size='18'>$text</text>")
        }
        [void]$svg.AppendLine("<text x='80' y='$($py-35)' font-size='16'>row $y</text>")
    }
    [void]$svg.AppendLine('<path d="M440 105 L440 355 L640 355 L640 485 L440 485 L440 605" fill="none" stroke="#d21c1c" stroke-width="3" stroke-dasharray="9 6"/><circle cx="35" cy="355" r="21" fill="#c9e8ff" stroke="#333"/><text x="30" y="361" font-size="20">s</text><circle cx="1120" cy="355" r="21" fill="#eee" stroke="#333"/><text x="1115" y="361" font-size="20">t</text>')
    [void]$svg.AppendLine('<text x="25" y="640" font-size="17">Solid horizontal edges: c = 1000e. Dashed blue backward/diagonal edges: INF.</text><text x="25" y="667" font-size="17">All source and sink edges: INF. Here sum(c) = 112000, so INF = 112001.</text><text x="25" y="694" font-size="17">Seam columns [1,1,2,1]; cut capacity = 4000; original seam cost = 4. The virtual nodes v are not pixels.</text></g></svg>')
    $svg.ToString() | Set-Content -LiteralPath docs/report/figures/c3/network-example.svg -Encoding UTF8
} finally { Pop-Location }
