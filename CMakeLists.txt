cmake_minimum_required(VERSION 2.6)

if (POLICY CMP0053)
cmake_policy(SET CMP0053 OLD)
endif ()

if (POLICY CMP0054)
    cmake_policy(SET CMP0054 OLD)
endif ()

project(ucapcowin C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(PkgConfig)

include(PkgConfigVars)
include(Glib2MkEnums)

add_definitions("-std=c99 -Wall -Wno-deprecated-declarations")
add_definitions(-DG_LOG_DOMAIN="Uca-Pco-Win")

pkg_check_modules(GIO gio-2.0>=2.22 REQUIRED)
pkg_check_modules(UCA libuca>=2.1.0 REQUIRED)
pkg_check_variable(libuca plugindir)

set(PCOSDK_INCLUDE_DIR "C:\\pco.sdk\\include" CACHE PATH "Include directory of pco SDK")
set(PCOSDK_LIBRARY_DIR "C:\\pco.sdk\\bin64" CACHE PATH "Library directory of pco SDK")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${UCA_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${PCOSDK_INCLUDE_DIR}
)
	
link_directories(
    ${UCA_LIBRARY_DIRS}
    ${GIO_LIBRARY_DIRS}
    ${PCOSDK_LIBRARY_DIR}
)

glib2_mkenums(uca-pco-enums
    uca-pco-enums
    uca-pco-win-camera.h
)

add_library(ucapcowin SHARED
    uca-pco-win-camera.c
    uca-pco-enums.c
)

target_link_libraries(ucapcowin
    ${UCA_LIBRARIES}
    ${GIO_LIBRARIES}
    SC2_Cam
)

install(TARGETS ucapcowin
        LIBRARY DESTINATION ${LIBUCA_PLUGINDIR}
        RUNTIME DESTINATION ${LIBUCA_PLUGINDIR})
