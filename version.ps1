

param(
    [string] $platformId = "x64"
)

echo ""
echo "******************************************************"
echo "*** Start 'create version info' ***"
echo "******************************************************"
echo ""

$Build = $env:APPVEYOR_BUILD_NUMBER
$distrver = $env:APPVEYOR_JOB_NAME
$appBranch= $env:APPVEYOR_REPO_BRANCH
#$appBranch= '"{0}"' -f $env:APPVEYOR_REPO_BRANCH
$buildFolder = $env:APPVEYOR_BUILD_FOLDER
$sourceDir = $env:APPVEYOR_BUILD_FOLDER

if(!$platformId)
{
    $platformId = $env:BUILD_PLATFORMID
}

if(!$platformId)
{
    Throw "platformId not set. Abort"
}

if (!$Build)
{
    $Build = 0
}

if (!$distrver)
{
    $distrver = (get-itemproperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion" -Name ProductName).ProductName 2>$null
    if (!$distrver)
    {
        $distribution = Invoke-Expression "lsb_release -cs"
        $version = Invoke-Expression "lsb_release -rs"
        $distrver = "$distribution.$version"
    }
}

if (!$appBranch)
{
    $appBranch = $(git rev-parse --abbrev-ref HEAD)
}

# $curDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($PSScriptRoot)
$curDir = Get-Location

if (!$buildFolder)
{
    $buildFolder = $curDir
}

if (!$sourceDir)
{
    $sourceDir = $curDir
}

Write-Host "-Build [$Build]`n"
Write-Host "-distrver [$distrver]`n"
Write-Host "-appBranch [$appBranch]`n"
Write-Host "-buildFolder [$buildFolder]`n"
Write-Host "-sourceDir [$sourceDir]`n"
Write-Host "-platformId [$platformId]`n"

Write-Host ""
Write-Host "*** Parse GIT attributes..."
Write-Host ""

$revision = "HEAD" 

$LastTag = git -C $sourceDir describe --tags --first-parent --match "*" $revision 2>$null
if (!$LastTag) 
{
	$LastTag = "0.0-test_build"
	Write-Host "Failed on get tag for revision $revision - defaulting to $LastTag"
}

$delim = ".",";","-","_"
$srrayStr = @($LastTag -Split {$delim -contains $_})
$Major = $srrayStr[0] -replace '\D+(\d+)','$1'
$Minor = $srrayStr[1]
$buildName = $srrayStr[2]

echo "[$LastTag] => [$Major.$Minor.$Build.$buildName]"
if (Get-Command Update-AppveyorBuild -errorAction SilentlyContinue)
{
    Update-AppveyorBuild -Version "$Major.$Minor.$Build.$appBranch"
    $env:APPVEYOR_BUILD_VERSION="$Major.$Minor.$Build.$appBranch"
}

echo ""
echo "*** Create release note..."
$releaseNoteFile = (Join-Path $buildFolder releaseNote.txt)
$gitTagList = git -C $sourceDir tag --sort=-version:refname
if(1 -ge $gitTagList.Count)
{
    Write-Host "Use all entries for a release note:"
    git -C $sourceDir log  --date=short --pretty=format:"  * %ad [%aN] %s" | Out-File -FilePath "$releaseNoteFile"
}
else
{
    $gitTagRange = $gitTagList[1] + "..$revision"
    Write-Host "Release note range [$gitTagRange]"
    git -C $sourceDir log "$gitTagRange" --date=short --pretty=format:"  * %ad [%aN] %s" | Out-File -FilePath "$releaseNoteFile"
}

Write-Host ""
Write-Host "*** Start update ver.h..."

$releasenote=$(Get-Content -Path "$releaseNoteFile")

$verContents = @"

#define FVER_NAME "$buildName"
#define FVER1 $Major
#define FVER2 $Minor
#define FVER3 $Build
#define FVER4 0
#define FBRANCH "$appBranch"
#define FDISTR "$distrver"
#define RELEASENOTE "$releasenote"

"@

$verFile = $(Join-Path $sourceDir ver.h)
Write-Host "Update [$verFile]"
[System.IO.File]::WriteAllText($verFile, $verContents)

Write-Host ""
Write-Host "*** Start update include.wxi..."
Write-Host ""

# Upgrade code HAS to be the same for all updates!!!
# logview reserved  ED1DA208-1973-445F-BADD-81B2E7CFE22B
$UpgradeCode = "ED1DA208-1973-445F-BADD-81B2E7CFE22B"
$ProductCode = [guid]::NewGuid().ToString().ToUpper()
$PackageCode = [guid]::NewGuid().ToString().ToUpper()

$directoryInstaller = $(Join-Path $sourceDir "\Installer\")
Write-Host "`$directoryInstaller: $directoryInstaller"

$outputIncludeFile = $(Join-Path $directoryInstaller "include.wxi")
If (Test-Path $outputIncludeFile)
{
	Remove-Item $outputIncludeFile
}

$new_output_contents_wxi_include = @"
<?xml version="1.0" encoding="utf-8"?>
<!-- Automatically-generated file. Do not edit! -->
<Include>
  <?define MajorVersion="$Major" ?>
  <?define MinorVersion="$Minor" ?>
  <?define BuildVersion="$Build" ?>
  <?define Version="$Major.$Minor.$Build" ?>
  <?define UpgradeCode="{$UpgradeCode}" ?>
  <?define ProductCode="{$ProductCode}" ?>
  <?define PackageCode="{$PackageCode}" ?>
  <?define ExeProcessName="logview.exe" ?>
  <?define Name="logview" ?>
  <?define Comments="https://github.com/chipmunk-sm/logview" ?>
  <?define Manufacturer="chipmunk-sm" ?>
  <?define CopyrightWarning="License GPL v3.0 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details" ?>
  <?define ARPTITLE="logview" ?>
  <?define ARPCONTACT="chipmunk-sm" ?> 
  <?define ARPHELPLINK="https://github.com/chipmunk-sm/logview" ?>
  <?define ARPURLINFOABOUT="https://github.com/chipmunk-sm/logview" ?>
  <?define Platform="$platformId" ?>
</Include>
"@

Write-Host "Update [$outputIncludeFile]"
[System.IO.File]::WriteAllText( $outputIncludeFile, $new_output_contents_wxi_include, (New-Object System.Text.UTF8Encoding $True))

Write-Host ""
Write-Host "******************************************************"
Write-Host "***  End 'create version info' "
Write-Host "******************************************************"

