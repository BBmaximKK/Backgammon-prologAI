Write-Host "===== BUILD BACKGAMMON PORTABLE ====="

# Percorsi (modifica solo se necessario)
$SWIPL_BIN = "C:\Program Files\swipl\bin"
$MINGW_BIN = "C:\msys64\mingw64\bin"
$RELEASE = "release"

# 1️⃣ Pulisce vecchia release
if (Test-Path $RELEASE) {
    Remove-Item $RELEASE -Recurse -Force
}
New-Item -ItemType Directory -Path $RELEASE | Out-Null

Write-Host "Compilazione..."

# 2️⃣ Compilazione
gcc main.c engine/*.c graphics/*.c game.c interface_prolog.c setup.c `
-Iengine -Igraphics -Iai `
-I"$SWIPL_BIN\..\include" `
-L"$SWIPL_BIN" `
-static -static-libgcc -static-libstdc++ `
-lraylib -lws2_32 -lopengl32 -lgdi32 -lwinmm `
"-Wl,-Bdynamic" -lswipl `
-o backgammon.exe

if (!(Test-Path "backgammon.exe")) {
    Write-Host "ERRORE: compilazione fallita!"
    exit
}

Write-Host "Compilazione completata!"

# Copia exe nella release
Copy-Item backgammon.exe $RELEASE

Write-Host "Copia DLL necessarie..."

# Copia DLL SWI-Prolog
Copy-Item "$SWIPL_BIN\libswipl.dll" $RELEASE -ErrorAction SilentlyContinue

# Copia DLL MinGW se esistono
$dlls = @("libgcc_s_seh-1.dll","libwinpthread-1.dll","libstdc++-6.dll")

foreach ($dll in $dlls) {
    $path = Join-Path $MINGW_BIN $dll
    if (Test-Path $path) {
        Copy-Item $path $RELEASE
    }
}

Write-Host "Build portable completata!"
Write-Host "Cartella pronta: /release"