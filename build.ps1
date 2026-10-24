$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $root
try {
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }
    cmake --build build --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw "Forgewild build failed." }
}
finally {
    Pop-Location
}
