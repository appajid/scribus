# For general Scribus (>=1.3.2) copyright and licensing information please refer
# to the COPYING file provided with the program. Following this notice may exist
# a copyright and/or license notice that predates the release of Scribus 1.3.2
# for which a new license (GPL+exception) is in place.

if(NOT DEFINED SCRIBUS_ROOT)
	message(FATAL_ERROR "SCRIBUS_ROOT is required")
endif()

set(FORK_APPLICATION_FILES
	scribus/anchorposition.cpp
	scribus/anchorposition.h
	scribus/dynamicvariable.cpp
	scribus/dynamicvariable.h
	scribus/styles/objectstyle.cpp
	scribus/styles/objectstyle.h
	scribus/stylequickapplymodel.cpp
	scribus/stylequickapplymodel.h
	scribus/ui/dynamicvariableinsert.cpp
	scribus/ui/dynamicvariableinsert.h
	scribus/ui/dynamicvariablemanager.cpp
	scribus/ui/dynamicvariablemanager.h
	scribus/ui/modernui.h
	scribus/ui/toolpalette.cpp
	scribus/ui/toolpalette.h
	scribus/ui/widgets/inspector_header.cpp
	scribus/ui/widgets/inspector_header.h)

foreach(MSVC_VERSION IN ITEMS 2019 2022 2026)
	set(PROJECT_DIR "${SCRIBUS_ROOT}/win32/msvc${MSVC_VERSION}/scribus-main")
	file(READ "${PROJECT_DIR}/Scribus.vcxproj" PROJECT_CONTENTS)
	file(READ "${PROJECT_DIR}/Scribus.vcxproj.filters" FILTER_CONTENTS)

	foreach(SOURCE_FILE IN LISTS FORK_APPLICATION_FILES)
		string(REPLACE "/" "\\" WINDOWS_SOURCE_FILE "${SOURCE_FILE}")
		set(PROJECT_REFERENCE "..\\..\\..\\${WINDOWS_SOURCE_FILE}")
		string(FIND "${PROJECT_CONTENTS}" "${PROJECT_REFERENCE}" PROJECT_INDEX)
		if(PROJECT_INDEX EQUAL -1)
			message(FATAL_ERROR
				"${SOURCE_FILE} is missing from the MSVC ${MSVC_VERSION} project")
		endif()

		string(FIND "${FILTER_CONTENTS}" "${PROJECT_REFERENCE}" FILTER_INDEX)
		if(FILTER_INDEX EQUAL -1)
			message(FATAL_ERROR
				"${SOURCE_FILE} is missing from the MSVC ${MSVC_VERSION} filters")
		endif()
	endforeach()
endforeach()

message(STATUS "All fork application files are registered in every Windows project")
