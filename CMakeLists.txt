cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

PROJECT(Tiling)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


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

set (SOURCES
    main.cpp
    vtkMapMarker.cxx
    vtkMapMarker.h
    vtkMapTile.cxx
    vtkMapTile.h
    vtkMap.cxx
    vtkMap.h
)

add_executable(vtkMap ${SOURCES})

target_link_libraries(vtkMap ${VTK_LIBRARIES} -lcurl)


if (BUILD_QT_APPS)
  add_subdirectory(Qt)
endif()
