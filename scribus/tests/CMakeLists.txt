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
