@title "Shairport4w - Coverity build"

@echo "Shairport4w - Coverity build"
@echo "Forced to x64 debug"
@echo "Coverity and VS 2022 Community required"
@echo:

@if "%COVERITY_SCAN%"=="" goto exit

@call "%SystemDrive%\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"


echo:
echo "Building %BUILD_PLATFORM% %BUILD_CONFIG%"
echo:

set BUILD_CMD=buildDebugX64.bat

%COVERITY_SCAN%\bin\cov-build --dir cov-int "%BUILD_CMD%"

echo:
echo "Build finished."
echo:

pause
goto end


:exit
echo:
echo "***** ERROR : COVERITY NOT INSTALLED !! *******"
echo "Please download Coverity scan and set COVERITY_SCAN env var to the place of the tool, e.g. something like : set COVERITY_SCAN=C:\cov-analysis-win64-2021.12.1"
echo:


:end
