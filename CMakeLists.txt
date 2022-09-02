cmake_minimum_required (VERSION 3.8)


PROJECT(fat_tool
        VERSION 1.0.0.1)

ENABLE_TESTING()
FIND_PACKAGE(Python3 REQUIRED COMPONENTS Interpreter)

#----------------------------------------------------------------------------
#
# Configure the environment.
#

INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(strupr "string.h" CFG_HAVE_STRUPR)
message(STATUS "CFG_HAVE_STRUPR: ${CFG_HAVE_STRUPR}")

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/configure/configure.h
               ${CMAKE_CURRENT_BINARY_DIR}/configure/configure.h)


#----------------------------------------------------------------------------
#
# Build the FAT library.
#

set(SOURCES_libfat
	src/fat/cache_dummy.c
	src/fat/directory.c
	src/fat/file_allocation_table.c
	src/fat/file_functions.c
	src/fat/filetime.c
	src/fat/format.c
	src/fat/partition.c
	src/fat/wrapper.c
	src/platform.c
)

add_library(TARGET_libfat STATIC ${SOURCES_libfat})

TARGET_INCLUDE_DIRECTORIES(TARGET_libfat
                           PUBLIC src
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/configure)


#----------------------------------------------------------------------------
#
# Build the RAM disk library.
#

set(SOURCES_libramdisk
	src/ramdisk/interface.c
)

add_library(TARGET_libramdisk STATIC ${SOURCES_libramdisk})

TARGET_INCLUDE_DIRECTORIES(TARGET_libramdisk
                           PUBLIC src)


#----------------------------------------------------------------------------
#
# Build the RAM disk library.
#

set(SOURCES_fattool
	src/fat_tool.cpp
	src/fatfs.cpp
)

add_executable(TARGET_fattool ${SOURCES_fattool})
target_link_libraries(TARGET_fattool TARGET_libfat TARGET_libramdisk)
set_property(TARGET TARGET_fattool PROPERTY OUTPUT_NAME "fat_tool")
IF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
	set_property(TARGET TARGET_fattool PROPERTY LINK_FLAGS "--static -static-libgcc -static-libstdc++")
ENDIF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))

# Add tests for this module.
IF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
        # Here are the MinGW specific tests.
        ADD_TEST(NAME fattool_MinGW_DLL_dependencies
                 COMMAND "${Python3_EXECUTABLE}" ${CMAKE_HOME_DIRECTORY}/cmake/tests/mingw_dll_dependencies.py -u lua5.1 -u lua5.2 -u lua5.3 $<TARGET_FILE:TARGET_fattool>)
ENDIF((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))

#----------------------------------------------------------------------------
#
# Build the distribution.
#

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} POM_ID_OS)
IF(NOT DEFINED TARGET_PROCESSOR)
	SET(TARGET_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
ENDIF(NOT DEFINED TARGET_PROCESSOR)
SET(POM_ID_PROCESSOR ${TARGET_PROCESSOR})
STRING(REPLACE . _ POM_ID_FAT_TOOL_VER ${PROJECT_VERSION})

SET(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/repository/org/muhkuh/tools/fat_tool/${PROJECT_VERSION})
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
SET(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}_${POM_ID_OS}_${POM_ID_PROCESSOR}-${PROJECT_VERSION})

SET(CPACK_GENERATOR "TBZ2")
INCLUDE(CPack)

# Build the POM file.
CONFIGURE_FILE(templates/fat_tool.pom ${CPACK_OUTPUT_FILE_PREFIX}/${PROJECT_NAME}_${POM_ID_OS}_${POM_ID_PROCESSOR}-${PROJECT_VERSION}.pom)

# Build the python file.
CONFIGURE_FILE(templates/fat_tool.py ${PROJECT_NAME}_${POM_ID_FAT_TOOL_VER}.py)

INSTALL(TARGETS TARGET_fattool
        RUNTIME DESTINATION ${PROJECT_NAME}-${PROJECT_VERSION}
)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${POM_ID_FAT_TOOL_VER}.py
        DESTINATION .)

#----------------------------------------------------------------------------

