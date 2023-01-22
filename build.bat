rem @echo off

@title "Bonjour for Windows - Single build"

@echo "Bonjour for Windows - Single build"
@echo "Use : build.bat BUILD_TARGET=<target> BUILD_PLATFORM=<platform> BUILD_CONFIG=<config>, e.g. build.bat BUILD_TARGET=Rebuild BUILD_PLATFORM=x64 BUILD_CONFIG=Release"
@echo "Build/Rebuild/clean x86/x64/arm64 Debug/Release
@echo "Default for build.bat alone : Rebuild x64 release"
@echo "Visual Studio 2022 Community required"
@echo:


call params.bat

REM Microsoft Visual Studio 2022 Community required

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86


rem Build/Rebuild/clean
if "%BUILD_TARGET%"=="" set BUILD_TARGET=Rebuild

rem Platform x86/x64/arm64
if "%BUILD_PLATFORM%"=="" set BUILD_PLATFORM=x64

rem Config Debug/Release
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Release





echo:
echo "%BUILD_TARGET% %BUILD_PLATFORM% %BUILD_CONFIG%"
echo:





MSBuild /nologo .\mDNSWindows\DLL\dnssd.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%
MSBuild /nologo .\mDNSWindows\DLLDLL\dnssdDLL.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%

MSBuild /nologo .\mDNSWindows\DLLStub\DLLStub.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%


MSBuild /nologo .\mDNSWindows\SystemService\mDNSResponder.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%
MSBuild /nologo .\mDNSWindows\SystemServiceDLL\mDNSResponderDLL.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%


MSBuild /nologo .\mDNSWindows\mDNSNetMonitor\mDNSNetMonitor.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%


MSBuild /nologo .\mDNSWindows\mdnsNSP\mdnsNSP.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%

MSBuild /nologo .\mDNSWindows\NSPTool\NSPTool.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%

MSBuild /nologo .\mDNSWindows\DLLX\DLLX.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%

MSBuild /nologo .\Clients\DNS-SD.VisualStudio\dns-sd.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%

MSBuild /nologo .\mDNSWindows\ControlPanel\ControlPanelLocRes.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%
MSBuild /nologo .\mDNSWindows\ControlPanel\ControlPanelRes.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%
MSBuild /nologo .\mDNSWindows\ControlPanel\ControlPanel.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%



MSBuild /nologo .\mDNSWindows\Java\Java.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%

MSBuild /nologo .\Clients\Java\JavaSamples.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%


MSBuild /nologo .\Clients\PrinterSetupWizard\PrinterSetupWizardLocRes.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%
MSBuild /nologo .\Clients\PrinterSetupWizard\PrinterSetupWizardRes.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%
MSBuild /nologo .\Clients\PrinterSetupWizard\PrinterSetupWizard.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%

MSBuild /nologo .\Clients\BonjourExample\BonjourExample.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%

rem MSBuild /nologo .\Clients\ExplorerPlugin\ExplorerPluginLocRes.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%
rem MSBuild /nologo .\Clients\ExplorerPlugin\ExplorerPluginRes.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%
rem MSBuild /nologo .\Clients\ExplorerPlugin\ExplorerPlugin.vcxproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=%BUILD_PLATFORM%



rem MSBuild /nologo .\Clients\DNSServiceBrowser.VB\DNSServiceBrowser.VB.vbproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=AnyCPU
rem MSBuild /nologo .\Clients\SimpleChat.VB\SimpleChat.VB.vbproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=AnyCPU
rem MSBuild /nologo .\Clients\DNSServiceBrowser.NET\DNSServiceBrowser.NET.csproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=AnyCPU
rem MSBuild /nologo .\Clients\SimpleChat.NET\SimpleChat.NET.csproj /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIG%;Platform=AnyCPU


MSBuild /nologo .\mDNSWindows\BonjourQuickLooksInstaller\BonjourQuickLooksInstaller.wixproj



echo:
echo "Build finished."
echo: