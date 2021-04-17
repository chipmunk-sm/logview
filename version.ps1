

param(
    [string] $platformId = "x64"
)

echo ""
echo "******************************************************"
echo "*** Start 'create version info' ***"
echo "******************************************************"
echo ""

if(!$platformId)
{
    Throw "platformId not set. Abort"
}

echo "`$platformId: $platformId"


$directory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($PSScriptRoot)
echo "`$directory: $directory"

echo ""
echo "***"
echo "*** Parse GIT attributes..."
echo "***"
echo ""

$revision = "HEAD" 
$buildName = "Custom build"
$Build=0

if($env:appveyor_build_number)
{
    $Build = $env:appveyor_build_number
    $buildName = "Auto-build"
}
else
{
    $versionFile = $(Join-Path $directory "ver.h")
    echo "`$versionFile: $versionFile"
    $fileContent = Get-Content -Path "$versionFile" -Raw -Encoding UTF8
    if($fileContent)
    {
        $searchTempl = "#define VER_BUILD "
        $posStart = $fileContent.IndexOf($searchTempl) 
        if(-1 -ne $posStart)
        {
            $posStart += $searchTempl.Length
            $posEnd = $fileContent.IndexOf("`n", $posStart) 
            if(-1 -eq $posEnd)
            {
                $posEnd = $fileContent.IndexOf("`r", $posStart) 
            }
            if(-1 -ne $posEnd)
            {
                $posEnd -= 1
                $newBuildNum = $fileContent.Substring($posStart, $posEnd - $posStart)
                $Build = 1 + $newBuildNum
                echo ""
                echo "***"
                echo "*** New Build Num = $Build"
                echo "***"
                echo ""
            }
        }
    }
}

$FullCommit = git -C $directory rev-parse $revision 2>$null
if (!$FullCommit) {
	Throw "Failed on get commit for $revision revision"
}

$LastTag = git -C $directory describe --tags --first-parent --match "*" $revision 2>$null


if (!$LastTag) 
{
	$LastTag = "0.0-test_release"
	Write-Host "Failed on get tag for revision $revision - defaulting to $LastTag"
}

$delim = ".",";","-"
$srrayStr = @($LastTag -Split {$delim -contains $_})
$Major = $srrayStr[0] -replace '\D+(\d+)','$1'
$Minor = $srrayStr[1]
$buildName = $srrayStr[2]

echo "TAG [$LastTag]"
echo "Message: [$buildName]" 
echo "[$LastTag] => [$Major.$Minor.$Build]"

$ShortCommit = $FullCommit.SubString(0, 7)

echo ""
echo "***"
echo "*** Create release note..."
echo "***"
echo ""

$releaseNoteFile = (Join-Path $directory releaseNote.txt)

$gitTagList = git -C $directory tag --sort=-version:refname
if(1 -ge $gitTagList.Count)
{
    echo "Use all entries for a release note:"
    git -C $directory log --pretty=format:"%d %s %N" | Out-File -FilePath "$releaseNoteFile"
}
else
{
    $gitTagRange = $gitTagList[1] + "..$revision"
    echo "Release note range [$gitTagRange]"
    git -C $directory log "$gitTagRange" --pretty=format:"%d %s %N" | Out-File -FilePath "$releaseNoteFile"
}

echo ""
Get-Content -Path "$releaseNoteFile"

echo ""
echo "***"
echo "*** Start update ver.h..."
echo "***"
echo ""

$output = "ver.h"

$new_output_contents = @"

#define FVER_NAME "$buildName"
#define FVER1 $Major
#define FVER2 $Minor
#define FVER3 $Build
#define FVER4 0

"@

$current_output_contents = Get-Content (Join-Path $directory $output) -Raw -Encoding Ascii -ErrorAction Ignore
if ([string]::Compare($new_output_contents, $current_output_contents, $False) -ne 0) {
	Write-Host "Updating $(Join-Path $directory $output)"
	[System.IO.File]::WriteAllText((Join-Path $directory $output), $new_output_contents)
}

echo ""
echo "***"
echo "*** Start update include.wxi..."
echo "***"
echo ""

# Upgrade code HAS to be the same for all updates!!!
$UpgradeCode = "ED1DA208-1973-445F-BADD-80B2E7CFE22A"
$ProductCode = [guid]::NewGuid().ToString().ToUpper()
$PackageCode = [guid]::NewGuid().ToString().ToUpper()

$directoryInstaller = $(Join-Path $directory "\Installer\")
echo "`$directoryInstaller: $directoryInstaller"

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

echo "Update $outputIncludeFile"
[System.IO.File]::WriteAllText( $outputIncludeFile, $new_output_contents_wxi_include, (New-Object System.Text.UTF8Encoding $True))


echo ""
echo "***"
echo "*** Start update Appveyor Build Version..."
echo "***"
echo ""

if (Get-Command Update-AppveyorBuild -errorAction SilentlyContinue)
{
    Update-AppveyorBuild -Version "$Major.$Minor.$Build"
}

$env:APPVEYOR_BUILD_VERSION="$Major.$Minor.$Build"

$tmpVer = $(Join-Path $directory "tmpver.txt")

[System.IO.File]::WriteAllText($tmpVer, "$Major.$Minor.$Build", [Text.Encoding]::ASCII)
  
echo "Write version $Major.$Minor.$Build to $tmpVer"

echo ""
echo "******************************************************"
echo "***  End 'create version info' "
echo "******************************************************"

