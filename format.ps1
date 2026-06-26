$excludeDirs = @('external', 'build', 'graphify-out', '.opencode', 'out')

$escapedDirs = ($excludeDirs | ForEach-Object { [regex]::Escape($_) }) -join '|'
$excludePattern = "[\\/]($escapedDirs)[\\/]"

Write-Host "Autoformatting. Exclude: $escapedDirs" -ForegroundColor DarkGray

Get-ChildItem -Path $PSScriptRoot -Recurse -File | Where-Object {
    $_.FullName -notmatch $excludePattern
} | ForEach-Object {
    if ($_.Extension -match '\.(h|hpp|cpp)$') {
        Write-Host "Formatting C++: $($_.FullName)" -ForegroundColor Cyan
        clang-format -i $_.FullName
    }
    elseif ($_.Name -eq 'CMakeLists.txt') {
        Write-Host "Formatting CMake: $($_.FullName)" -ForegroundColor Yellow
        cmake-format -i $_.FullName
    }
}

Write-Host "`nDone." -ForegroundColor Green
