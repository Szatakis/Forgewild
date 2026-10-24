$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $root
try {
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }
    cmake --build build --config Debug --parallel
    if ($LASTEXITCODE -ne 0) { throw "Forgewild build failed." }
    ctest --test-dir build --build-config Debug --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Forgewild tests failed." }
}
finally {
    Pop-Location
}
