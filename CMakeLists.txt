cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

PROJECT(Tiling)

set (VTK_REQUIRED_COMPONENTS
    # Core components
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkCommonCore
    vtkFiltersTexture)

find_package(VTK NO_MODULE REQUIRED COMPONENTS ${VTK_REQUIRED_COMPONENTS})

include(${VTK_USE_FILE})

set (SOURCES
    main.cpp
    vtkMapTile.cxx
    vtkMapTile.h
    vtkMap.cxx
    vtkMap.h
)

add_executable(vtkMap ${SOURCES})

target_link_libraries(vtkMap ${VTK_LIBRARIES} -lcurl)
