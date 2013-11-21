cmake_minimum_required (VERSION 2.8)


project(fat_tool)


IF(${CMAKE_BUILD_TOOL} MATCHES "(msdev|VCExpress|devenv|nmake)")
	# Do not show msvs security warnings about unsafe functions.
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF(${CMAKE_BUILD_TOOL} MATCHES "(msdev|VCExpress|devenv|nmake)")


#----------------------------------------------------------------------------
#
# Configure the environment.
#

INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(strupr "string.h" CFG_HAVE_STRUPR)
message(STATUS "CFG_HAVE_STRUPR: ${CFG_HAVE_STRUPR}")

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/configure/configure.h
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

#target_include_directories(TARGET_libfat PUBLIC src)
INCLUDE_DIRECTORIES(src ${CMAKE_CURRENT_BINARY_DIR}/configure)


#----------------------------------------------------------------------------
#
# Build the RAM disk library.
#

set(SOURCES_libramdisk
	src/ramdisk/interface.c
)

add_library(TARGET_libramdisk STATIC ${SOURCES_libramdisk})

#target_include_directories(TARGET_libramdisk PUBLIC src)
INCLUDE_DIRECTORIES(src ${CMAKE_CURRENT_BINARY_DIR}/configure)


#----------------------------------------------------------------------------
#
# Build the FAT tool.
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


#----------------------------------------------------------------------------
#
# Build the distribution.
#

SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "1")

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} POM_ID_OS)
IF(NOT DEFINED TARGET_PROCESSOR)
	SET(TARGET_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
ENDIF(NOT DEFINED TARGET_PROCESSOR)
SET(POM_ID_PROCESSOR ${TARGET_PROCESSOR})
SET(POM_FAT_TOOL_VER ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
STRING(REPLACE . _ POM_ID_FAT_TOOL_VER ${POM_FAT_TOOL_VER})
SET(POM_MBS_RELEASE_VERSION 1)

SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
SET(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}_${POM_ID_OS}_${POM_ID_PROCESSOR}-${POM_FAT_TOOL_VER}_${POM_MBS_RELEASE_VERSION})

SET(CPACK_GENERATOR "TBZ2")
INCLUDE(CPack)

# Build the POM file.
CONFIGURE_FILE(templates/fat_tool.pom ${PROJECT_NAME}_${POM_ID_OS}_${POM_ID_PROCESSOR}-${POM_FAT_TOOL_VER}_${POM_MBS_RELEASE_VERSION}.pom)

# Build the python file.
CONFIGURE_FILE(templates/fat_tool.py ${PROJECT_NAME}_${POM_ID_FAT_TOOL_VER}_${POM_MBS_RELEASE_VERSION}.py)

# Build the version.h file
CONFIGURE_FILE(templates/version.h configure/version.h )

INSTALL(TARGETS TARGET_fattool
        RUNTIME DESTINATION ${PROJECT_NAME}-${POM_FAT_TOOL_VER}_${POM_MBS_RELEASE_VERSION}
)

INSTALL(FILES 
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${POM_ID_FAT_TOOL_VER}_${POM_MBS_RELEASE_VERSION}.py
        DESTINATION .
)
        
INSTALL(FILES 
        doc/readme.txt
        doc/changelog.txt
        DESTINATION ${PROJECT_NAME}-${POM_FAT_TOOL_VER}_${POM_MBS_RELEASE_VERSION}/doc
)

#----------------------------------------------------------------------------

