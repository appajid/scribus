# For general Scribus (>=1.3.2) copyright and licensing information please refer
# to the COPYING file provided with the program. Following this notice may exist
# a copyright and/or license notice that predates the release of Scribus 1.3.2
# for which a new license (GPL+exception) is in place.

if(NOT DEFINED SCRIBUS_ROOT)
	message(FATAL_ERROR "SCRIBUS_ROOT is required")
endif()

set(WORKFLOW "${SCRIBUS_ROOT}/.github/workflows/windows-native-package.yml")
set(BUILDER "${SCRIBUS_ROOT}/win32/ci/build-package.ps1")
set(VALIDATOR "${SCRIBUS_ROOT}/win32/ci/validate-package.ps1")
set(PORTABLE_ASSEMBLER "${SCRIBUS_ROOT}/win32/deploy/assemble.ps1")
set(INSTALLER_ASSEMBLER "${SCRIBUS_ROOT}/win32/installer/deploy.ps1")

foreach(REQUIRED_FILE IN ITEMS
		"${WORKFLOW}"
		"${BUILDER}"
		"${VALIDATOR}"
		"${PORTABLE_ASSEMBLER}"
		"${INSTALLER_ASSEMBLER}")
	if(NOT EXISTS "${REQUIRED_FILE}")
		message(FATAL_ERROR "Required Windows packaging file is missing: ${REQUIRED_FILE}")
	endif()
endforeach()

function(require_text FILE_PATH REQUIRED_TEXT DESCRIPTION)
	file(READ "${FILE_PATH}" FILE_CONTENTS)
	string(FIND "${FILE_CONTENTS}" "${REQUIRED_TEXT}" TEXT_INDEX)
	if(TEXT_INDEX EQUAL -1)
		message(FATAL_ERROR "${DESCRIPTION} is missing from ${FILE_PATH}")
	endif()
endfunction()

require_text("${WORKFLOW}" "runs-on: windows-2022" "native Windows runner")
require_text("${WORKFLOW}" "arch: win64_msvc2022_64" "MSVC 2022 Qt architecture")
require_text("${WORKFLOW}" "-TestInstaller" "installer smoke test")
require_text("${BUILDER}"
	"a6bd40450a22415d26cc9b0bf4aeaf9f295cfcff2f8bf64e9c62289aa69fdf3b"
	"pinned dependency archive checksum")
require_text("${BUILDER}" "scribus-libs-msvc2022.sln" "Visual Studio 2022 dependency solution")
require_text("${VALIDATOR}" "Assert-X64PE" "x64 PE validation")
require_text("${VALIDATOR}" "python\\python313.dll" "bundled Python validation")
require_text("${VALIDATOR}" "qtplugins\\platforms\\qwindows.dll" "Qt Windows platform validation")
require_text("${PORTABLE_ASSEMBLER}" "'python'" "Python portable staging")
require_text("${INSTALLER_ASSEMBLER}" "'python'" "Python installer staging")

message(STATUS "Native Windows build and packaging contract is complete")
