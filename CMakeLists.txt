cmake_minimum_required(VERSION 3.22)
project(neo)
set(CMAKE_C_STANDARD 99)

option(NEO_USE_WCHAR "Use wide charactrers for strings" FALSE)

# Get version number
file(READ "${PROJECT_SOURCE_DIR}/src/VERSION" ver)
string(REGEX MATCH "VERSION_MAJOR ([0-9]*)" _ ${ver})
set(ver_major ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_MINOR ([0-9]*)" _ ${ver})
set(ver_minor ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_PATCH ([0-9]*)" _ ${ver})
set(ver_patch ${CMAKE_MATCH_1})

# Build application
set(APP_SOURCE
	"${PROJECT_SOURCE_DIR}/src/main.c"
	"${PROJECT_SOURCE_DIR}/src/neo_common.c"
	"${PROJECT_SOURCE_DIR}/src/neo_editor.c"
	"${PROJECT_SOURCE_DIR}/src/neo_menu.c"
	"${PROJECT_SOURCE_DIR}/src/neo_settings.c"
	"${PROJECT_SOURCE_DIR}/src/neo_string.c"
	"${PROJECT_SOURCE_DIR}/src/neo_actions.c"
)
set(APP_HEADER
	"${PROJECT_SOURCE_DIR}/src/neo_common.h"
	"${PROJECT_SOURCE_DIR}/src/neo_editor.h"
	"${PROJECT_SOURCE_DIR}/src/neo_menu.h"
	"${PROJECT_SOURCE_DIR}/src/neo_settings.h"
	"${PROJECT_SOURCE_DIR}/src/neo_string.h"
	"${PROJECT_SOURCE_DIR}/src/neo_actions.h"
	"${PROJECT_SOURCE_DIR}/src/neo.h"
	"${PROJECT_SOURCE_DIR}/vendor/vex/include/vex/vex.h"
	"${PROJECT_SOURCE_DIR}/vendor/komihash/komihash.h"
)
add_executable(neo ${APP_SOURCE} ${APP_HEADER})
target_link_libraries(neo PRIVATE -lc -lmenu -lpanel)
target_compile_definitions(neo PRIVATE
	NEO_VERSION_MAJOR=${ver_major}
	NEO_VERSION_MINOR=${ver_minor}
	NEO_VERSION_PATCH=${ver_patch}
)
set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/${BUILD_ARCH}")
set_target_properties(neo PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
	LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
)
message("neodymium version: ${ver_major}.${ver_minor}.${ver_patch}")

# Configure for wide strings
if (NEO_USE_WCHAR)
	target_link_libraries(neo PRIVATE -lncursesw)
	target_compile_definitions(neo PRIVATE NEO_USE_WCHAR)
else()
	target_link_libraries(neo PRIVATE -lncurses)
endif()

# Add include directories
target_include_directories(neo PRIVATE
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_SOURCE_DIR}/vendor"
	"${PROJECT_SOURCE_DIR}/vendor/vex/include"
)