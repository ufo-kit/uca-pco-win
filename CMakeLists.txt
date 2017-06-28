cmake_minimum_required(VERSION 2.6)
project(ucapcowin C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(PkgConfig)
include(PkgConfigVars)
include(ConfigurePaths)
include(Glib2MkEnums)
configure_paths(LIBUCA)

add_definitions("-std=c99 -w")
add_definitions(-DG_LOG_DOMAIN="UCA-PCO-WIN")

pkg_check_modules(UCA libuca>=2.1.0 REQUIRED)
pkg_check_modules(GIO gio-2.0>=2.22 REQUIRED)
pkg_check_variable(libuca plugindir)

set(PCOSDK_INCLUDE_DIR "C:\\pco.sdk\\include" CACHE PATH "Include directory of pco SDK")
set(PCOSDK_LIBRARY_DIR "C:\\pco.sdk\\bin64" CACHE PATH "Library directory of pco SDK")

set (${PCO_SDK_HEADERS} 
	${PCOSDK_INCLUDE_DIR}/sc2_SDKStructures.h
	${PCOSDK_INCLUDE_DIR}/sc2_defs.h
	${PCOSDK_INCLUDE_DIR}/PCO_err.h
	${PCOSDK_INCLUDE_DIR}/PCO_errt.h )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${UCA_INCLUDE_DIRS}
	${GIO_INCLUDE_DIRS}
	${PCOSDK_INCLUDE_DIR})
	
link_directories(${UCA_LIBRARY_DIRS} ${GIO_LIBRARY_DIRS} ${PCOSDK_LIBRARY_DIR})

# Reused from ufo-kit/uca-pco plugin
glib2_mkenums(uca-pco-enums
    uca-pco-enums
    uca-pco-win-camera.h)

add_library(ucapcowin SHARED
    uca-pco-win-camera.c uca-pco-enums.c)

target_link_libraries(ucapcowin
    ${UCA_LIBRARIES} ${GIO_LIBRARIES} SC2_Cam)

install(TARGETS ucapcowin
        LIBRARY DESTINATION ${LIBUCA_PLUGINDIR}
        RUNTIME DESTINATION ${LIBUCA_PLUGINDIR})
