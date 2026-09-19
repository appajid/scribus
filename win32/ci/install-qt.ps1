# For general Scribus (>=1.3.2) copyright and licensing information please refer
# to the COPYING file provided with the program. Following this notice may exist
# a copyright and/or license notice that predates the release of Scribus 1.3.2
# for which a new license (GPL+exception) is in place.

[CmdletBinding()]
param(
	[string]$InstallRoot = 'C:\Qt\6.11.2\msvc2022_64'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$qtRepository = 'https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6112/qt6_6112_msvc2022_64'
$packageVersion = '6.11.2-0-202608131017'
$basePackage = 'qt.qt6.6112.win64_msvc2022_64'
$compatPackage = 'qt.qt6.6112.addons.qt5compat.win64_msvc2022_64'

$archives = @(
	@{ Package = $basePackage; File = 'qtbase-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z' },
	@{ Package = $basePackage; File = 'qtsvg-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z' },
	@{ Package = $basePackage; File = 'qtdeclarative-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z' },
	@{ Package = $basePackage; File = 'qttools-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z' },
	@{ Package = $basePackage; File = 'qttranslations-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z' },
	@{ Package = $basePackage; File = 'd3dcompiler_47-x64.7z' },
	@{ Package = $basePackage; File = 'opengl32sw-64-mesa_11_2_2-signed_sha256.7z' },
	@{ Package = $compatPackage; File = 'qt5compat-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z' }
)

$requiredFiles = @(
	'bin\qmake.exe',
	'bin\windeployqt.exe',
	'bin\Qt6Core.dll',
	'bin\Qt6Gui.dll',
	'bin\Qt6Widgets.dll',
	'bin\Qt6Svg.dll',
	'bin\Qt6Core5Compat.dll',
	'lib\Qt6Core.lib',
	'lib\Qt6Svg.lib',
	'lib\Qt6Core5Compat.lib',
	'lib\cmake\Qt6\Qt6Config.cmake'
)

function Test-QtInstallation
{
	foreach ($relativePath in $requiredFiles)
	{
		if (-not (Test-Path -LiteralPath (Join-Path $InstallRoot $relativePath) -PathType Leaf))
		{
			return $false
		}
	}
	return $true
}

function Export-QtEnvironment
{
	$qtBin = Join-Path $InstallRoot 'bin'
	$qtCMake = Join-Path $InstallRoot 'lib\cmake'
	$env:QT_ROOT_DIR = $InstallRoot
	$env:Qt6_DIR = $qtCMake
	$env:Path = "$qtBin;$env:Path"

	if ($env:GITHUB_ENV)
	{
		Add-Content -LiteralPath $env:GITHUB_ENV -Value "QT_ROOT_DIR=$InstallRoot"
		Add-Content -LiteralPath $env:GITHUB_ENV -Value "Qt6_DIR=$qtCMake"
	}
	if ($env:GITHUB_PATH)
	{
		Add-Content -LiteralPath $env:GITHUB_PATH -Value $qtBin
	}
}

if (Test-QtInstallation)
{
	Write-Host "Using cached Qt 6.11.2 installation at $InstallRoot"
	Export-QtEnvironment
	exit 0
}

$sevenZip = (Get-Command '7z.exe' -ErrorAction Stop).Source
$downloadRoot = if ($env:RUNNER_TEMP) { Join-Path $env:RUNNER_TEMP 'scribus-qt-archives' } else { Join-Path $env:TEMP 'scribus-qt-archives' }
New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
New-Item -ItemType Directory -Path $InstallRoot -Force | Out-Null

foreach ($archive in $archives)
{
	$fileName = [string]$archive.File
	$packageName = [string]$archive.Package
	$archiveUrl = "$qtRepository/$packageName/$packageVersion$fileName"
	$archivePath = Join-Path $downloadRoot $fileName
	$checksumPath = "$archivePath.sha1"

	Write-Host "Downloading $fileName"
	& curl.exe --fail --location --retry 5 --retry-delay 5 --output $archivePath $archiveUrl
	if ($LASTEXITCODE -ne 0) { throw "Failed to download $archiveUrl" }
	& curl.exe --fail --location --retry 5 --retry-delay 5 --output $checksumPath "$archiveUrl.sha1"
	if ($LASTEXITCODE -ne 0) { throw "Failed to download $archiveUrl.sha1" }

	$expectedSha1 = ((Get-Content -LiteralPath $checksumPath -Raw).Trim() -split '\s+')[0].ToLowerInvariant()
	$actualSha1 = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA1).Hash.ToLowerInvariant()
	if ($actualSha1 -ne $expectedSha1)
	{
		Remove-Item -LiteralPath $archivePath -Force
		throw "SHA-1 mismatch for $fileName (expected $expectedSha1, got $actualSha1)"
	}

	Write-Host "Extracting verified $fileName"
	& $sevenZip x $archivePath "-o$InstallRoot" -y | Out-Host
	if ($LASTEXITCODE -ne 0) { throw "7-Zip failed to extract $fileName" }
}

if (-not (Test-QtInstallation))
{
	$missingFiles = $requiredFiles | Where-Object { -not (Test-Path -LiteralPath (Join-Path $InstallRoot $_) -PathType Leaf) }
	throw "Qt installation is incomplete. Missing: $($missingFiles -join ', ')"
}

Export-QtEnvironment
Write-Host "Qt 6.11.2 is ready at $InstallRoot"
