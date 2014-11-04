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

# include export header modules so that we can easily control symbol exporting
# VTK Map is setup by default not to export symbols unless explicitly stated.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
include(GenerateExportHeader)

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

#headers that we are going to install
set (HEADERS
    vtkFeature.h
    vtkFeatureLayer.h
    vtkInteractorStyleMap.h
    vtkMapMarkerSet.h
    vtkMapPickResult.h
    vtkMapTile.h
    vtkMap.h
    vtkLayer.h
    vtkOsmLayer.h
    vtkPolydataFeature.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtkmap_export.h
    )


# Specify targets
add_library(vtkMap ${SOURCES})

target_link_libraries(vtkMap ${VTK_LIBRARIES} curl)
target_include_directories(vtkMap PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

#setup export header
generate_export_header(vtkMap)

# On Mac OS X, set the directory included as part of the
# installed library's path. We only do this to libraries that we plan
# on installing
if (BUILD_SHARED_LIBS)
  set_target_properties(vtkMap PROPERTIES
                        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
else( )
  set_target_properties(vtkMap PROPERTIES
                        POSITION_INDEPENDENT_CODE True
                        )
endif()

#now generate very basic install rules
install(TARGETS vtkMap
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install (FILES ${HEADERS} DESTINATION include)

#example does not need to installed
add_executable(example example.cpp)
target_link_libraries(example vtkMap)

#both testing and Qt do need to exported or installed as they are for testing
#and examples
add_subdirectory(Testing)
if (BUILD_QT_APPS)
  add_subdirectory(Qt)
endif()
