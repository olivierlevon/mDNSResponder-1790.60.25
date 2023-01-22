# 

$scriptpath = $MyInvocation.MyCommand.Path
$scriptdir = Split-Path $scriptpath

$bonjourpath = Join-Path $scriptdir "mdnsresponder.exe"
$etwman = Join-Path $scriptdir "bonjour-events.man"

if (-not (Test-Path $bonjourpath)) {
    throw "mdnsresponder.exe is not present in script path"
}

if (Get-Service "Bonjour Service" -ErrorAction SilentlyContinue) 
{
   Stop-Service "Bonjour Service"
   sc.exe delete "Bonjour Service" 1>$null
}


# unregister etw provider
wevtutil um `"$etwman`"

# adjust provider resource path in instrumentation manifest
[XML]$xml = Get-Content $etwman
$xml.instrumentationManifest.instrumentation.events.provider.resourceFileName = $bonjourpath.ToString()
$xml.instrumentationManifest.instrumentation.events.provider.messageFileName = $bonjourpath.ToString()

$streamWriter = $null
$xmlWriter = $null
try {
    $streamWriter = new-object System.IO.StreamWriter($etwman)
    $xmlWriter = [System.Xml.XmlWriter]::Create($streamWriter)    
    $xml.Save($xmlWriter)
}
finally {
    if($streamWriter) {
        $streamWriter.Close()
    }
}

#register etw provider
wevtutil im `"$etwman`"


$bonjourDesc = "Enables hardware devices and software services to automatically configure themselves on the network and advertise their presence."
New-Service -Name "Bonjour Service" -DisplayName "Bonjour Service" -BinaryPathName `"$bonjourpath`" -Description $bonjourDesc -StartupType Automatic | Out-Null
sc.exe privs "Bonjour Service" SeAssignPrimaryTokenPrivilege/SeTcbPrivilege/SeBackupPrivilege/SeRestorePrivilege/SeImpersonatePrivilege

Write-Host -ForegroundColor Green "Bonjour service successfully installed"
