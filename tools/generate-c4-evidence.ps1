param(
    [string]$BuildDir = 'build-task-audit',
    [switch]$Large
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $build = [IO.Path]::GetFullPath((Join-Path $root $BuildDir))
    & cmake --build $build --config Release --target c4_evidence bench_maxflow
    if ($LASTEXITCODE -ne 0) { throw 'Build failed. Configure a Release build first.' }
    function Find-Binary([string]$name) {
        foreach ($relative in @("bench/$name.exe", "bench/Release/$name.exe", "bench/$name")) {
            $candidate = Join-Path $build $relative
            if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
        }
        throw "Cannot find $name in $build."
    }
    & (Find-Binary 'c4_evidence')
    if ($LASTEXITCODE -ne 0) { throw 'Seam validation or evidence generation failed.' }

    # Compose actual seamlib PNGs; no algorithm output is retouched.
    Add-Type -AssemblyName System.Drawing
    foreach ($name in @('portrait', 'building')) {
        $bitmap = New-Object Drawing.Bitmap 1400, 300
        $graphics = [Drawing.Graphics]::FromImage($bitmap)
        $title = New-Object Drawing.Font 'Arial', 18
        $label = New-Object Drawing.Font 'Arial', 12
        try {
            $graphics.Clear([Drawing.Color]::White)
            $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
            $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
            $graphics.DrawString("$name : DP versus Dinic", $title, [Drawing.Brushes]::Black, 15, 10)
            $suffixes = @('input', 'dp-first', 'dinic-first', 'dp-result', 'dinic-result')
            $labels = @('Input (64 x 43)', 'DP first seam (red)', 'Dinic first seam (red)', 'DP result (54 x 43)', 'Dinic result (54 x 43)')
            for ($i = 0; $i -lt $suffixes.Count; $i++) {
                $graphics.DrawString($labels[$i], $label, [Drawing.Brushes]::Black, [single](15 + $i * 280), 53)
                $path = Join-Path $root "docs/report/figures/c4/$name-$($suffixes[$i]).png"
                $panel = [Drawing.Image]::FromFile($path)
                try {
                    $rectangle = New-Object Drawing.Rectangle ([int]($i * 280 + (280 - $panel.Width * 4) / 2)), 85, ($panel.Width * 4), ($panel.Height * 4)
                    $graphics.DrawImage($panel, $rectangle, 0, 0, $panel.Width, $panel.Height, [Drawing.GraphicsUnit]::Pixel)
                } finally { $panel.Dispose() }
            }
            $graphics.DrawString('Ten vertical seams removed. Panels enlarged 4x using nearest-neighbour display scaling.', $label, [Drawing.Brushes]::Black, 15, 273)
            $bitmap.Save((Join-Path $root "docs/report/figures/c4/dp-vs-dinic-$name.png"), [Drawing.Imaging.ImageFormat]::Png)
        } finally {
            $graphics.Dispose()
            $bitmap.Dispose()
            $title.Dispose()
            $label.Dispose()
        }
    }
    $benchmark = Find-Binary 'bench_maxflow'
    if ($Large) { & $benchmark results --large }
    else { & $benchmark results }
    if ($LASTEXITCODE -ne 0) { throw 'Max-flow measurements failed.' }
} finally { Pop-Location }
