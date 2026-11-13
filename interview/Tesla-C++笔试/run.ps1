param(
    [ValidateSet('examples','1','2','3')][string]$Task = 'examples',
    [switch]$Reference,
    [ValidateSet('all','fifo','full','close','state','reentry','exception','concurrent','result','workers','stop')]
    [string]$Case = 'all'
)

$ErrorActionPreference = 'Stop'
$compiler = (Get-Command g++ -CommandType Application).Source
$job = @{
    '1' = @('01-限时有界队列', 'queue.hpp', 'fifo', 'full', 'close')
    '2' = @('02-修复设备状态回调', 'device.hpp', 'state', 'reentry', 'exception', 'concurrent')
    '3' = @('03-双线程任务执行器', 'executor.hpp', 'result', 'workers', 'stop')
}

function Run-Source([string]$Source, [string]$TestCase) {
    Write-Host "Building $Source"
    & $compiler -std=c++17 -g -Wall -Wextra -Wpedantic -pthread $Source -o check.exe
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $Source" }
    $process = Start-Process -FilePath (Join-Path (Get-Location).Path 'check.exe') `
        -ArgumentList $TestCase -NoNewWindow -PassThru
    if (!$process.WaitForExit(8000)) {
        $process.Kill()
        $process.WaitForExit()
        throw "TIMEOUT: $TestCase (8s). Check waits, notifications, and joins."
    }
    if ($process.ExitCode -ne 0) { throw "Test failed: $TestCase (exit $($process.ExitCode))" }
}

$temp = New-Item -ItemType Directory -Path (Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString()))
Push-Location -LiteralPath $temp.FullName
try {
    if ($Task -eq 'examples') {
        if ($Reference -or $Case -ne 'all') { throw 'Use -Task examples without other options.' }
        foreach ($file in Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot '代码') -Filter '*.cpp' -File | Sort-Object Name) {
            Copy-Item -LiteralPath $file.FullName -Destination .
            Run-Source $file.Name 'all'
        }
    } else {
        $folder, $header, $cases = $job[$Task]
        if ($Case -ne 'all' -and $Case -notin $cases) { throw "Available cases: $cases" }
        $exercise = Join-Path (Join-Path $PSScriptRoot '模拟') $folder
        $implementation = $exercise
        if ($Reference) {
            $implementation = Join-Path (Join-Path $PSScriptRoot '参考答案') $folder
        }
        Copy-Item -LiteralPath (Join-Path $exercise 'test.cpp') -Destination .
        Copy-Item -LiteralPath (Join-Path $implementation $header) -Destination .
        Run-Source 'test.cpp' $Case
    }
} finally {
    Pop-Location
    Get-ChildItem -LiteralPath $temp.FullName -File | Remove-Item
    Remove-Item -LiteralPath $temp.FullName
}
