rem @echo off

cls

title "Bonjour1"

rem title "Bonjour mdns/dnssd service/standalone daemon"


rem run with admin rights !!


for /F "TOKENS=1,2,*" %%a in ('tasklist /FI "WindowTitle eq Bonjour1"') do set MyPID=%%b
echo %MyPID%


rem cd C:\Users\Olivier\Desktop\notes-master-ffi\Bonjour\Debug

cd %~dp0% 


set CUR_YYYY=%date:~6,4%
set CUR_MM=%date:~3,2%
set CUR_DD=%date:~0,2%

set CUR_HH=%time:~0,2%
if %CUR_HH% lss 10 (set CUR_HH=0%time:~1,1%)

set CUR_NN=%time:~3,2%
set CUR_SS=%time:~6,2%
set CUR_MS=%time:~9,2%


rem log to ramdisk r:
rem set LOGFILENAME=r:\mdnsresponder_marianne_%CUR_YYYY%%CUR_MM%%CUR_DD%-%CUR_HH%%CUR_NN%%CUR_SS%.txt
set LOGFILENAME=C:\Users\Olivier\Downloads\notes-master-ffi\Bonjour\Debug\mdnsresponder_marianne_%CUR_YYYY%%CUR_MM%%CUR_DD%-%CUR_HH%%CUR_NN%%CUR_SS%.txt

set path=c:\msys64\usr\bin\;%path%

rem chcp 65001

rem mDNSResponder_svc.exe  -t -d -v  -p  -server  | tee %LOGFILENAME% 2>&1
mDNSResponder_svc.exe  -t -d -v  -p  -server  2>&1 | wtee -i %LOGFILENAME%

rem mdnsresponder -server

pause