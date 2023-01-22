rem @echo off

@title "Bonjour for Windows - Rebuild all"

@echo "Bonjour for Windows - Rebuild all"
@echo:
@echo "Visual Studio 2022 Community required"
@echo:


rem MSBuild mDNSResponder.sln /t:Clean 

@echo:

echo:
echo "Debug x86"
echo:

call build.bat

echo:
echo "Release x86"
echo:

MSBuild /nologo .\src\Shairport4w.vcxproj /t:Rebuild /p:Configuration=Release;Platform=x86

echo:
echo "Debug x64"
echo:

MSBuild /nologo .\src\Shairport4w.vcxproj /t:Rebuild /p:Configuration=Debug;Platform=x64

echo:
echo "Release x64"
echo:

MSBuild /nologo .\src\Shairport4w.vcxproj /t:Rebuild /p:Configuration=Release;Platform=x64




echo:
echo "Debug ARM64"
echo:

MSBuild /nologo .\src\Shairport4w.vcxproj /t:Rebuild /p:Configuration=Debug;Platform=x64

echo:
echo "Release ARM64"
echo:

MSBuild /nologo .\src\Shairport4w.vcxproj /t:Rebuild /p:Configuration=Release;Platform=x64




echo:
echo "Build finished."
echo:

pause


