cmake_minimum_required (VERSION 2.8)

project(fat_tool)

if(${CMAKE_BUILD_TOOL} MATCHES "(msdev|VCExpress|devenv|nmake)")
	# Do not show msvs security warnings about unsafe functions.
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF(${CMAKE_BUILD_TOOL} MATCHES "(msdev|VCExpress|devenv|nmake)")

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
INCLUDE_DIRECTORIES(src)


#----------------------------------------------------------------------------
#
# Build the RAM disk library.
#

set(SOURCES_libramdisk
	src/ramdisk/interface.c
)

add_library(TARGET_libramdisk STATIC ${SOURCES_libramdisk})

#target_include_directories(TARGET_libramdisk PUBLIC src)
INCLUDE_DIRECTORIES(src)


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

