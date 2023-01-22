rem @echo off

echo "Building Java Bonjour Debug via makefiles"

rem https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019#developer_command_prompt_shortcuts

rem %comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm uwp -vcvars_ver=14.26

rem On a 64bits OS, 64bits JDK, VS2019
rem %comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

For /F "Tokens=*" %%I in ('vswhere -latest /property installationPath') Do Set VS_INSTALL="%%I"

echo %VS_INSTALL%

call %VS_INSTALL%\VC\Auxiliary\Build\vcvarsall.bat x64


cd %~dp0

rem set JAVA_HOME="C:\Program Files\Java\jdk1.8.0_251"
rem set JDK_HOME="C:\Program Files\Java\jdk1.8.0_251"

set JAVA_HOME=C:\Program Files\Java\jdk-11.0.7
rem set JDK_HOME="C:\Program Files\Java\jdk-11.0.7"

"%JAVA_HOME%\bin\javac" -version

echo "building Java extension"
rem nmake /nologo /f makefile64 DEBUG=1 JDK="C:\Program Files\Java\jdk-11.0.7"
rem nmake /nologo /f makefile DEBUG=1
nmake /nologo /A /f makefile64 DEBUG=1

rem nmake /NOLOGO /A /f makefile DEBUG=1

rem echo "building Java samples"
rem nmake /nologo /f nmakefile DEBUG=1 DNS_SD=..\..\mDNSWindows\Java\build\debug\dns_sd.jar


pause