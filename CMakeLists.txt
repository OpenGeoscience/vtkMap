cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

PROJECT(Tiling)

set(VTK_REQUIRED_COMPONENTS
	# Core components
	vtkRenderingCore
	vtkRenderingOpenGL
	vtkjsoncpp
	vtkCommonCore
	vtkInteractionStyle
	vtkIOPostgreSQL
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

add_executable(LabelContours ${SOURCES})

target_link_libraries(LabelContours ${VTK_LIBRARIES} -lcurl)
