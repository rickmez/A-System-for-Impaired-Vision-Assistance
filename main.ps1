
# build.ps1
param (
    [string]$file = "Cpp\main.cpp",
    [string]$output = "app.exe"
)

Write-Output "Compiling $file ..."
g++ $file -lfreeglut -lopengl32 -lglu32  -o $output
#g++ Getdata/main.cpp -lfreeglut -lopengl32 -lglu32 -o app.exe

if ($LASTEXITCODE -eq 0) {
    Write-Output "Build successful! Running $output ..."
    & .\$output
} else {
    Write-Output "Build failed!"
}

## powershell -ExecutionPolicy Bypass -File main.ps1
