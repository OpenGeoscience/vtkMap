cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

PROJECT(Tiling)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(BUILD_SHARED_LIBS "Build vtkMap with shared libraries" ON)

set (VTK_REQUIRED_COMPONENTS
    # Core components
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkCommonCore
    vtkFiltersTexture
    vtkInteractionStyle)


option(BUILD_QT_APPS "Build Qt applications (source files in Qt subdirectory)" OFF)
if (BUILD_QT_APPS)
  list(APPEND VTK_REQUIRED_COMPONENTS vtkGUISupportQt)
endif()

find_package(VTK NO_MODULE REQUIRED COMPONENTS ${VTK_REQUIRED_COMPONENTS})

include(${VTK_USE_FILE})

# Temporary logic for specifying marker image file, used in vtkMap.cxx
add_definitions("-DMARKER_IMAGE_FILE=\"${CMAKE_SOURCE_DIR}/markerteal.png\"")

set (SOURCES
    vtkMapMarkerSet.cxx
    vtkMapPickResult.cxx
    vtkMapTile.cxx
    vtkMap.cxx
    vtkLayer.cxx
    vtkOsmLayer.cxx
    vtkTeardropSource.cxx
)

add_library(vtkMap ${SOURCES})
target_link_libraries(vtkMap ${VTK_LIBRARIES} curl)

add_executable(example example.cpp)
target_link_libraries(example vtkMap)

add_subdirectory(Testing)
if (BUILD_QT_APPS)
  add_subdirectory(Qt)
endif()
