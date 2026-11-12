param(
  [Parameter(Mandatory=$true)][string]$AvrBin,
  [string]$Node = 'node',
  [string]$EmulatorModule = 'avr8js',
  [string]$BuildDirectory = (Join-Path $PSScriptRoot 'build')
)
$ErrorActionPreference='Stop'
$build=$BuildDirectory
New-Item -ItemType Directory -Force $build | Out-Null
foreach($name in @('firmware_tests','locked_tests')) {
  & (Join-Path $AvrBin 'avr-g++.exe') -mmcu=atmega328p -Os -std=gnu++11 "-I$PSScriptRoot/mocks" (Join-Path $PSScriptRoot "$name.cpp") -o "$build/$name.elf" -lm
  if($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
  & (Join-Path $AvrBin 'avr-objcopy.exe') -O binary "$build/$name.elf" "$build/$name.bin"
  if($LASTEXITCODE -ne 0) { throw 'Binary conversion failed' }
  & (Join-Path $AvrBin 'avr-nm.exe') "$build/$name.elf" | Set-Content "$build/$name-symbols.txt"
  if($LASTEXITCODE -ne 0) { throw 'Symbol extraction failed' }
  & $Node (Join-Path $PSScriptRoot 'run-emulator.cjs') "$build/$name.bin" "$build/$name-symbols.txt" $EmulatorModule
  if($LASTEXITCODE -ne 0) { throw 'Emulated assertions failed' }
}
