[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$Engine,
    [Parameter(Position = 1)]
    [ValidateRange(0, 20)][int]$MinLog2 = 2,
    [Parameter(Position = 2)]
    [ValidateRange(0, 20)][int]$MaxLog2 = 20,
    [Parameter(Position = 3)]
    [ValidateRange(1, 100)][int]$Runs = 5,
    [Parameter(Position = 4)]
    [ValidateRange(1, 64)][int]$Depth = 8,
    [string]$OutputDirectory,
    [ValidateRange(1, 86400)][int]$TimeoutSeconds = 3600
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Use a new, hidden child process for each invocation. Only this child is killed
# on timeout/cancellation; other running copies of the engine are left alone.
function Invoke-FreshEngine {
    param([string[]]$Commands, [string]$LogPath)

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo.FileName = $script:enginePath
    $process.StartInfo.WorkingDirectory = Split-Path -Parent $script:enginePath
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardInput = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    $started = $false
    $stdoutTask = $null
    $stderrTask = $null
    $watch = [System.Diagnostics.Stopwatch]::StartNew()
    try {
        $started = $process.Start()
        if (-not $started) { throw 'Could not start the engine.' }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        foreach ($command in $Commands) { $process.StandardInput.WriteLine($command) }
        $process.StandardInput.Close()

        while (-not $process.WaitForExit(1000)) {
            if ($watch.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
                throw "Engine PID $($process.Id) timed out after $TimeoutSeconds seconds. See $LogPath"
            }
        }
        $watch.Stop()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        if ($process.ExitCode -ne 0) {
            throw "Engine exited with code $($process.ExitCode). See $LogPath"
        }
        return [pscustomobject]@{
            Stdout = $stdout
            ProcessId = $process.Id
            ProcessTimeMs = [math]::Round($watch.Elapsed.TotalMilliseconds, 2)
        }
    }
    finally {
        try {
            if ($started -and -not $process.HasExited) {
                $process.Kill()
                $null = $process.WaitForExit(5000)
            }
            if ($null -ne $stdoutTask) {
                $stdoutTask.GetAwaiter().GetResult() |
                    Set-Content -LiteralPath $LogPath -Encoding UTF8
            }
            if ($null -ne $stderrTask) {
                $stderrText = $stderrTask.GetAwaiter().GetResult()
                if ($stderrText.Length -gt 0) {
                    $stderrText | Set-Content -LiteralPath "$LogPath.stderr" -Encoding UTF8
                }
            }
        }
        finally { $process.Dispose() }
    }
}

try {
    if ($MinLog2 -gt $MaxLog2) { throw 'MinLog2 must not exceed MaxLog2.' }
    if (-not $Engine) {
        $Engine = Join-Path (Split-Path -Parent $PSScriptRoot) 'out\build\x64-Release\Chess-Bot_Engine.exe'
    }
    $script:enginePath = (Resolve-Path -LiteralPath $Engine).Path
    if (-not (Test-Path -LiteralPath $script:enginePath -PathType Leaf)) {
        throw "Engine is not a file: $script:enginePath"
    }
    if (-not $OutputDirectory) {
        $suffix = [guid]::NewGuid().ToString('N').Substring(0, 8)
        $OutputDirectory = Join-Path (Split-Path -Parent $PSScriptRoot) (
            'out\bench-pawn-table\{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), $suffix)
    }
    if (Test-Path -LiteralPath $OutputDirectory) {
        throw 'OutputDirectory must be new, to avoid overwriting previous results.'
    }
    $outputPath = (New-Item -ItemType Directory -Path $OutputDirectory).FullName
    $runCsv = Join-Path $outputPath 'runs.csv'
    $summaryCsv = Join-Path $outputPath 'summary.csv'

    # Check before any timed benchmark, rather than silently testing a clamped value.
    $probe = Invoke-FreshEngine -Commands @('uci', 'quit') -LogPath (Join-Path $outputPath 'uci.log')
    $optionPattern = '(?m)^option name PAWN_TABLE_SIZE_LOG2 type spin default (-?\d+) min (\d+) max (\d+)\s*$'
    $option = [regex]::Match($probe.Stdout, $optionPattern)
    if (-not $option.Success -or $probe.Stdout -notmatch '(?m)^uciok\s*$') {
        throw 'Engine does not advertise PAWN_TABLE_SIZE_LOG2. Use a rebuilt normal UCI executable.'
    }
    $supportedMin = [int]$option.Groups[2].Value
    $supportedMax = [int]$option.Groups[3].Value
    if ($MinLog2 -lt $supportedMin -or $MaxLog2 -gt $supportedMax) {
        throw "Engine supports exponents $supportedMin..$supportedMax, but you requested $MinLog2..$MaxLog2. Rebuild or narrow the range."
    }
    $hashStream = [System.IO.File]::OpenRead($script:enginePath)
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    try { $engineHash = [BitConverter]::ToString($hasher.ComputeHash($hashStream)).Replace('-', '') }
    finally { $hasher.Dispose(); $hashStream.Dispose() }
    [pscustomobject]@{
        Engine = $script:enginePath
        EngineSHA256 = $engineHash
        StartedAt = (Get-Date).ToString('o')
        MinLog2 = $MinLog2; MaxLog2 = $MaxLog2; Runs = $Runs; Depth = $Depth
        SupportedMin = $supportedMin; SupportedMax = $supportedMax
        CsvCulture = [System.Globalization.CultureInfo]::CurrentCulture.Name
        Order = 'ascending on odd rounds, descending on even rounds'
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outputPath 'settings.json') -Encoding UTF8

    Write-Host "Engine: $script:enginePath"
    Write-Host "bench_full depth $Depth; $Runs fresh-process runs per exponent; range $MinLog2..$MaxLog2"
    Write-Host "Results: $outputPath"
    $results = New-Object 'System.Collections.Generic.List[object]'
    $referenceSignature = $null
    $warnedDiagnostics = $false
    $completed = 0
    $totalRuns = ($MaxLog2 - $MinLog2 + 1) * $Runs
    for ($round = 1; $round -le $Runs; ++$round) {
        $exponents = @($MinLog2..$MaxLog2)
        if ($round % 2 -eq 0) { [array]::Reverse($exponents) }
        foreach ($exponent in $exponents) {
            Write-Host ('[{0}/{1}] log2={2}, run {3}/{4}...' -f (
                $completed + 1), $totalRuns, $exponent, $round, $Runs)
            $logPath = Join-Path $outputPath ('log2_{0:D2}_run_{1:D2}.log' -f $exponent, $round)
            # bench_full is synchronous, so quit is processed only after it finishes.
            $run = Invoke-FreshEngine -Commands @(
                'setoption name Threads value 1'
                "setoption name PAWN_TABLE_SIZE_LOG2 value $exponent"
                'uci'
                "go bench_full depth $Depth"
                'quit'
            ) -LogPath $logPath

            $effective = [regex]::Match($run.Stdout, $optionPattern)
            if (-not $effective.Success -or [int]$effective.Groups[1].Value -ne $exponent) {
                throw "Engine did not accept exponent $exponent. See $logPath"
            }
            if (-not $warnedDiagnostics -and $run.Stdout -match ' pawnprobes ') {
                Write-Warning 'Diagnostics are enabled. Use a diagnostics-disabled Release build for final speed comparisons.'
                $warnedDiagnostics = $true
            }
            $totals = [regex]::Matches($run.Stdout,
                '(?m)^info string bench total positions (\d+) depth (\d+) time (\d+) nodes (\d+) nps (\d+)\s*$')
            if ($totals.Count -ne 1) { throw "Missing/ambiguous benchmark summary. See $logPath" }
            $total = $totals[0]
            $timeMs = [long]$total.Groups[3].Value
            if ([int]$total.Groups[2].Value -ne $Depth -or $timeMs -le 0) {
                throw "Wrong depth or a run too short to time; increase Depth. See $logPath"
            }
            $positions = [regex]::Matches($run.Stdout,
                '(?m)^info string bench pos (.*?) bestmove (\S+).*? eval (-?\d+).*? nodes (\d+)(?:\s|$)')
            if ($positions.Count -ne [int]$total.Groups[1].Value) {
                throw "Incomplete per-position results. See $logPath"
            }
            $signature = ($positions | ForEach-Object {
                '{0}|{1}|{2}|{3}' -f $_.Groups[1].Value, $_.Groups[2].Value,
                    $_.Groups[3].Value, $_.Groups[4].Value
            }) -join "`n"
            if ($null -eq $referenceSignature) { $referenceSignature = $signature }
            $matchesReference = $signature -ceq $referenceSignature
            $entryCount = [long]1 -shl $exponent
            foreach ($capacity in [regex]::Matches($run.Stdout, ' pawncapacity (\d+)')) {
                $actual = [long]$capacity.Groups[1].Value
                if ($actual -ne 0 -and $actual -ne $entryCount) {
                    throw "Unexpected pawn capacity $actual (expected $entryCount). See $logPath"
                }
            }
            $record = [pscustomobject]@{
                Log2 = $exponent; Entries = $entryCount; Run = $round; Depth = $Depth
                TimeMs = $timeMs; Nodes = [long]$total.Groups[4].Value
                NPS = [long]$total.Groups[5].Value
                ProcessId = $run.ProcessId; ProcessTimeMs = $run.ProcessTimeMs
                MatchesReference = $matchesReference; LogFile = $logPath
            }
            $results.Add($record)
            $results | Export-Csv -LiteralPath $runCsv -NoTypeInformation -Encoding UTF8
            if (-not $matchesReference) {
                throw "Moves, scores, or node counts changed; do not compare these timings. See $logPath"
            }
            ++$completed
            Write-Host ('  {0:N0} NPS, {1:N0} ms, PID {2} (exited)' -f $record.NPS, $timeMs, $run.ProcessId)
        }
    }

    $summary = foreach ($exponent in $MinLog2..$MaxLog2) {
        $samples = @($results | Where-Object { $_.Log2 -eq $exponent })
        $npsStats = $samples | Measure-Object -Property NPS -Average -Minimum -Maximum
        $mean = $npsStats.Average
        $sumSquared = 0.0
        foreach ($sample in $samples) { $sumSquared += [math]::Pow($sample.NPS - $mean, 2) }
        $stdDev = 0.0
        if ($samples.Count -gt 1) { $stdDev = [math]::Sqrt($sumSquared / ($samples.Count - 1)) }
        $timeTotal = ($samples | Measure-Object -Property TimeMs -Sum).Sum
        $nodeTotal = ($samples | Measure-Object -Property Nodes -Sum).Sum
        [pscustomobject]@{
            Log2 = $exponent; Entries = [long]1 -shl $exponent; Runs = $samples.Count
            AvgTimeMs = [math]::Round($timeTotal / $samples.Count, 2)
            AvgNPS = [math]::Round($mean, 2)
            PooledNPS = [math]::Round(1000.0 * $nodeTotal / $timeTotal, 2)
            MinNPS = $npsStats.Minimum; MaxNPS = $npsStats.Maximum
            NpsStdDevPct = [math]::Round(100.0 * $stdDev / $mean, 2)
        }
    }
    $summary | Export-Csv -LiteralPath $summaryCsv -NoTypeInformation -Encoding UTF8
    Write-Host ''
    $summary | Format-Table Log2, Entries, Runs, AvgTimeMs, AvgNPS, PooledNPS, NpsStdDevPct -AutoSize | Out-Host
    Write-Host "Averages: $summaryCsv"
    Write-Host "Individual runs: $runCsv"
    Write-Host 'AvgNPS is the arithmetic mean. PooledNPS is total nodes / total benchmark time.'
    exit 0
}
catch {
    Write-Error -Message $_.Exception.Message -ErrorAction Continue
    exit 1
}
