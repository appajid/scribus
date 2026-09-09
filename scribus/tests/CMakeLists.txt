include_directories(
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/scribus
${FREETYPE_INCLUDE_DIRS}
${SCRIBUS_AUTOGEN_INCLUDE_PATH}
..
)

set(SCRIBUS_TEST_SOURCES
runtests.cpp
#testIndex.cpp
testStoryText.cpp
)

set(SCRIBUS_TESTS_LIB "scribus_tests_lib")
add_library(${SCRIBUS_TESTS_LIB} STATIC ${SCRIBUS_TEST_SOURCES})
target_link_libraries(${SCRIBUS_TESTS_LIB} PRIVATE Qt6::Test)


# This is a convenience library that for linkage purposes is part of Scribus's
# main API.
set_target_properties(${SCRIBUS_TESTS_LIB}
  PROPERTIES
  COMPILE_FLAGS -DCOMPILE_SCRIBUS_MAIN_APP
  )


# Regular unit tests below.
#
# These tests are built as standalone executables and are run using "make test"
#

set(TESTS_LIBRARIES Qt6::Test Qt6::Core)

# Unit tests for anchored-object positioning and wrap geometry
set(ANCHORPOSITIONTESTS_SOURCES anchorpositiontests.cpp ../anchorposition.cpp)
add_executable(anchorpositiontests ${ANCHORPOSITIONTESTS_SOURCES})
target_link_libraries(anchorpositiontests ${TESTS_LIBRARIES} Qt6::Gui)
target_compile_definitions(anchorpositiontests PRIVATE COMPILE_SCRIBUS_MAIN_APP)
add_test(NAME anchorpositiontests COMMAND anchorpositiontests)

# Unit tests for CellArea
set(CELLAREATESTS_SOURCES cellareatests.cpp ../cellarea.cpp)
add_executable(cellareatests ${CELLAREATESTS_SOURCES})
target_link_libraries(cellareatests ${TESTS_LIBRARIES})
add_test(NAME cellareatests COMMAND cellareatests)

# Unit tests for Quick Actions discovery and execution
set(ACTIONSEARCHTESTS_SOURCES actionsearchtests.cpp ../actionsearch.cpp)
add_executable(actionsearchtests ${ACTIONSEARCHTESTS_SOURCES})
target_link_libraries(actionsearchtests Qt6::Test Qt6::Core Qt6::Gui Qt6::Widgets)
target_compile_definitions(actionsearchtests PRIVATE COMPILE_SCRIBUS_MAIN_APP)
add_test(NAME actionsearchtests COMMAND actionsearchtests)
set_tests_properties(actionsearchtests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# End-to-end running-header tests use the installed application because the
# Scripter and file-format plugins are loaded from the installation layout.
if(APPLEBUNDLE)
	set(SCRIBUS_SCRIPT_TEST_EXECUTABLE
		"${APPLE_APP_DIR}/Scribus${VERSION}.app/Contents/MacOS/Scribus")
elseif(WIN32)
	set(SCRIBUS_SCRIPT_TEST_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/scribus.exe")
else()
	set(SCRIBUS_SCRIPT_TEST_EXECUTABLE
		"${CMAKE_INSTALL_PREFIX}/bin/scribus${TAG_VERSION}")
endif()

add_test(NAME runningheader_test_install
	COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>)
set_tests_properties(runningheader_test_install PROPERTIES
	FIXTURES_SETUP runningheader_test_app
	LABELS "integration;running-headers"
	TIMEOUT 180)

function(add_running_header_test test_name script_name pass_marker timeout_seconds)
	set(test_prefs_dir "${CMAKE_CURRENT_BINARY_DIR}/prefs-${test_name}")
	file(MAKE_DIRECTORY "${test_prefs_dir}")
	add_test(NAME ${test_name}
		COMMAND "${SCRIBUS_SCRIPT_TEST_EXECUTABLE}"
			--no-splash
			--prefs "${test_prefs_dir}"
			--no-gui
			--python-script "${CMAKE_CURRENT_SOURCE_DIR}/scripts/${script_name}")
	set_tests_properties(${test_name} PROPERTIES
		ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
		FIXTURES_REQUIRED runningheader_test_app
		LABELS "integration;running-headers"
		PASS_REGULAR_EXPRESSION "${pass_marker}"
		RUN_SERIAL TRUE
		TIMEOUT ${timeout_seconds})
endfunction()

add_running_header_test(runningheader_masters test_running_header_masters.py
	"RUNNING_HEADER_MASTERS_QA_PASSED" 180)
add_running_header_test(runningheader_spreads test_running_header_spreads.py
	"RUNNING_HEADER_SPREADS_QA_PASSED" 120)
add_running_header_test(runningheader_sections test_running_header_sections.py
	"RUNNING_HEADER_SECTIONS_QA_PASSED" 120)
add_running_header_test(runningheader_scale test_running_header_scale.py
	"RUNNING_HEADER_SCALE_QA_PASSED" 180)

set(crossreference_prefs_dir "${CMAKE_CURRENT_BINARY_DIR}/prefs-crossreference_core")
file(MAKE_DIRECTORY "${crossreference_prefs_dir}")
add_test(NAME crossreference_core
	COMMAND "${SCRIBUS_SCRIPT_TEST_EXECUTABLE}"
		--no-splash
		--prefs "${crossreference_prefs_dir}"
		--no-gui
		--python-script "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_cross_references.py")
set_tests_properties(crossreference_core PROPERTIES
	ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
	FIXTURES_REQUIRED runningheader_test_app
	LABELS "integration;cross-references"
	PASS_REGULAR_EXPRESSION "CROSS_REFERENCE_QA_PASSED"
	RUN_SERIAL TRUE
	TIMEOUT 120)

set(anchored_object_prefs_dir "${CMAKE_CURRENT_BINARY_DIR}/prefs-anchored_objects")
file(MAKE_DIRECTORY "${anchored_object_prefs_dir}")
add_test(NAME anchored_objects
	COMMAND "${SCRIBUS_SCRIPT_TEST_EXECUTABLE}"
		--no-splash
		--prefs "${anchored_object_prefs_dir}"
		--no-gui
		--python-script "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_anchored_objects.py")
set_tests_properties(anchored_objects PROPERTIES
	ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
	FIXTURES_REQUIRED runningheader_test_app
	LABELS "integration;anchored-objects"
	PASS_REGULAR_EXPRESSION "ANCHORED_OBJECT_QA_PASSED"
	RUN_SERIAL TRUE
	TIMEOUT 120)

find_program(PDFTOTEXT_EXECUTABLE pdftotext)
if(NOT PDFTOTEXT_EXECUTABLE)
	set_tests_properties(runningheader_masters PROPERTIES DISABLED TRUE)
endif()
