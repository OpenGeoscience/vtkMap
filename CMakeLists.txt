cmake_minimum_required(VERSION 3.8.2 FATAL_ERROR)
project(vtkMap)

# ==============================================================================
# Flags & Build
# ==============================================================================
# Require C++11
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  set(CMAKE_CXX_EXTENSIONS FALSE)
endif()

# This is needed for compatibility with ES_Superbuild
if (NOT WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  option(USE_CXX11_ABI_0 "Use the legacy GCC stdlib ABI" OFF)
  mark_as_advanced(USE_CXX11_ABI_0)
  if (USE_CXX11_ABI_0)
    add_definitions("-D_GLIBCXX_USE_CXX11_ABI=0")
  endif()
endif ()

# ==============================================================================
# User options
# ==============================================================================

option(BUILD_EXAMPLES "Build example applications." ON)
option(BUILD_QT_APPS "Build Qt applications." OFF)
option(BUILD_SHARED_LIBS "Build vtkMap with shared libraries." ON)
option(BUILD_GDAL_EXTENSION "GDAL Extensions." OFF)
option(BUILD_GeoJSON_EXTENSION "GeoJSON Extensions." OFF)
option(BUILD_TESTING "Build vtkMap tests." OFF)

option(DISABLE_CURL_SIGNALS "Do not install libcurl signal handlers." OFF)

# ==============================================================================
# Dependencies
# ==============================================================================

# Add our Cmake directory to the module search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(OpenGL REQUIRED)

### VTK
set (VTK_REQUIRED_COMPONENTS
    vtkCommonCore
    vtkFiltersTexture
    vtkInteractionStyle
    vtkIOLegacy
    vtkjsoncpp
    vtkRenderingCore
    vtkRenderingLabel
    vtkRenderingOpenGL2

#   For testing
    vtkIOXML
    vtkImagingStatistics
    vtkInteractionImage
)

if (BUILD_QT_APPS)
  list(APPEND VTK_REQUIRED_COMPONENTS vtkGUISupportQt)
endif ()

if (BUILD_GDAL_EXTENSION)
  list(APPEND VTK_REQUIRED_COMPONENTS vtkIOGDAL)
endif ()

if (BUILD_GeoJSON_EXTENSION)
  list(APPEND VTK_REQUIRED_COMPONENTS vtkIOGeoJSON)
endif ()

find_package(VTK NO_MODULE REQUIRED COMPONENTS ${VTK_REQUIRED_COMPONENTS})
include(${VTK_USE_FILE})

### Curl
mark_as_advanced(DISABLE_CURL_SIGNALS)
if(DISABLE_CURL_SIGNALS)
  add_definitions("-DDISABLE_CURL_SIGNALS")
endif ()

find_package(CURL REQUIRED)

if (BUILD_QT_APPS)
  find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
endif ()

# ==============================================================================
# Modules
# ==============================================================================

include(GenerateExportHeader)
include(vtkMapMacros)

add_subdirectory(assets)
add_subdirectory(core)

if (BUILD_GDAL_EXTENSION)
  add_subdirectory(extensions/GDAL)
endif ()

if (BUILD_GeoJSON_EXTENSION)
  add_subdirectory(extensions/GeoJSON)
endif ()

# Testing, examples, and Qt-apps are not exported or installed.
if (BUILD_EXAMPLES)
  add_subdirectory(applications/examples)
endif ()

if (BUILD_TESTING)
  add_subdirectory(testing)
endif ()

if (BUILD_QT_APPS)
  add_subdirectory(applications/weatherstations)
endif()
