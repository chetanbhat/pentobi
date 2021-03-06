cmake_minimum_required(VERSION 3.1.0)

project(Pentobi)
set(PENTOBI_VERSION 17.x)
set(PENTOBI_RELEASE_DATE 2019-11-05)

cmake_policy(SET CMP0043 NEW)
cmake_policy(SET CMP0071 NEW)

include(GNUInstallDirs)

option(PENTOBI_BUILD_GTP "Build GTP interface" OFF)
option(PENTOBI_BUILD_GUI "Build GUI" ON)
option(PENTOBI_BUILD_THUMBNAILER "Build Gnome thumbnailer" ${UNIX})
option(PENTOBI_BUILD_KDE_THUMBNAILER "Build KDE thumbnailer" OFF)
option(PENTOBI_OPEN_HELP_EXTERNALLY "Force using web browser for displaying help" OFF)
option(BUILD_TESTING "Build tests" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
if(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  add_compile_options(-ffast-math -Wall -Wextra)
endif()

if(BUILD_TESTING)
  if(PENTOBI_BUILD_KDE_THUMBNAILER)
    configure_file(pentobi_kde_thumbnailer/CTestCustom.cmake
        ${CMAKE_BINARY_DIR} COPYONLY)
  endif()
  enable_testing()
endif()

if(UNIX)
  add_custom_target(dist
    COMMAND git archive --prefix=pentobi-${PENTOBI_VERSION}/ HEAD
    | xz -e > ${CMAKE_BINARY_DIR}/pentobi-${PENTOBI_VERSION}.tar.xz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

add_subdirectory(libboardgame_base)
add_subdirectory(libpentobi_base)
if(BUILD_TESTING)
    add_subdirectory(libboardgame_test)
endif()
if(PENTOBI_BUILD_GUI OR PENTOBI_BUILD_GTP)
    add_subdirectory(libboardgame_mcts)
    add_subdirectory(libpentobi_mcts)
endif()
if(PENTOBI_BUILD_GTP)
    add_subdirectory(libboardgame_gtp)
    add_subdirectory(libpentobi_gtp)
    add_subdirectory(pentobi_gtp)
    if(UNIX)
        add_subdirectory(twogtp)
    else()
        message(STATUS "Not building twogtp, needs POSIX")
    endif()
    add_subdirectory(learn_tool)
endif()
if(PENTOBI_BUILD_GUI)
    add_subdirectory(libpentobi_paint)
    add_subdirectory(pentobi)
    if(PENTOBI_BUILD_THUMBNAILER)
        add_subdirectory(libpentobi_thumbnail)
        add_subdirectory(pentobi_thumbnailer)
    endif()
endif()
if(PENTOBI_BUILD_KDE_THUMBNAILER)
    add_subdirectory(libpentobi_kde_thumbnailer)
    add_subdirectory(pentobi_kde_thumbnailer)
endif()

