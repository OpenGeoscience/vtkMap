cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

PROJECT(vtkMap)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(BUILD_SHARED_LIBS "Build vtkMap with shared libraries" ON)


# Specify VTK components
set (VTK_REQUIRED_COMPONENTS
    vtkCommonCore
    vtkFiltersTexture
    vtkInteractionStyle
    vtkjsoncpp
    vtkRenderingCore
    vtkRenderingOpenGL
)

option(BUILD_QT_APPS "Build Qt applications (source files in Qt subdirectory)" OFF)
if (BUILD_QT_APPS)
  list(APPEND VTK_REQUIRED_COMPONENTS vtkGUISupportQt)
endif()

find_package(VTK NO_MODULE REQUIRED COMPONENTS ${VTK_REQUIRED_COMPONENTS})

include(${VTK_USE_FILE})


# Specify vtkDataSetReaders components
# Main repo is https://github.com/OpenGeoscience/vtkDataSetReaders
set(READERS_INCLUDE_DIRECTORY "" CACHE PATH "Location of vtkDataSetReaders header files")
include_directories(${READERS_INCLUDE_DIRECTORY})
set(READERS_LIB_DIRECTORY "" CACHE PATH "Location of vtkDataSetReaders library")
set(READERS_LIBRARIES vtkDataSetReaders)
link_directories(${READERS_LIB_DIRECTORY})


# Specify source files
set (SOURCES
    vtkFeature.cxx
    vtkFeatureLayer.cxx
    vtkGeoJSONMapFeature.cxx
    vtkInteractorStyleMap.cxx
    vtkMapMarkerSet.cxx
    vtkMapPickResult.cxx
    vtkMapTile.cxx
    vtkMap.cxx
    vtkLayer.cxx
    vtkOsmLayer.cxx
    vtkPolydataFeature.cxx
    vtkTeardropSource.cxx
)


# Specify targets
add_library(vtkMap ${SOURCES})
target_link_libraries(vtkMap ${READERS_LIBRARIES} ${VTK_LIBRARIES} curl)

add_executable(example example.cpp)
target_link_libraries(example vtkMap)


# Specify subdirectories
add_subdirectory(Testing)
if (BUILD_QT_APPS)
  add_subdirectory(Qt)
endif()
