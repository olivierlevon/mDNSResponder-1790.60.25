#

$scriptpath = $MyInvocation.MyCommand.Path
$scriptdir = Split-Path $scriptpath
$etwman = Join-Path $scriptdir "bonjour-events.man"



# unregister etw provider
wevtutil um `"$etwman`"

if (Get-Service "Bonjour Service" -ErrorAction SilentlyContinue) 
{
   Stop-Service "Bonjour Service"
   sc.exe delete "Bonjour Service" 1>$null
   Write-Host -ForegroundColor Green "bonjour successfully uninstalled"
}
else {
    Write-Host -ForegroundColor Yellow "bonjour service is not installed"
}
