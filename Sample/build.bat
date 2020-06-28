@echo off
setlocal enabledelayedexpansion
cd %~dp0

echo Building solution...
..\DTools\External\premake5 vs2017

echo Build DExporter.exe...
for /f "usebackq tokens=*" %%i in (`..\DTools\External\vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
  "%%i" ..\DTools\DExporter\DExporter.vcxproj -p:PlatformToolset=v141 -p:Platform=x64 -t:Build -p:Configuration=Release
)

echo Copying external libraries...
xcopy /y ..\DTools\External\Assimp\bin\x64\assimp-vc141-mt.dll  ..\DTools\Bin\

echo Building assets...
..\DTools\Bin\DExporterRelease.exe -mode scene -input Assets\Scenes\sampleScene.json
echo Done

pause