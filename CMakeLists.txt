cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

PROJECT(vtkMap)

# If the user/superbuild hasn't explicitly stated what c++ standard to use,
# require C++11, since VTK now builds with it
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  set(CMAKE_CXX_EXTENSIONS FALSE)
endif()


# Add our Cmake directory to the module search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(BUILD_SHARED_LIBS "Build vtkMap with shared libraries" ON)

# Specify VTK components
set (VTK_RENDERING_BACKEND "OpenGL2"
     CACHE STRING "Specify OpenGL2 (default) or OpenGL (legacy)")
set (VTK_REQUIRED_COMPONENTS
    vtkCommonCore
    vtkFiltersTexture
    vtkInteractionStyle
    vtkIOGDAL
    vtkIOGeoJSON
    vtkIOLegacy
    vtkjsoncpp
    vtkRenderingCore
    vtkRendering${VTK_RENDERING_BACKEND}

    # For testing:
    vtkIOXML
    vtkImagingStatistics
    vtkInteractionImage
)

option(BUILD_QT_APPS "Build Qt applications (source files in Qt subdirectory)" OFF)
if (BUILD_QT_APPS)
  list(APPEND VTK_REQUIRED_COMPONENTS vtkGUISupportQt)
endif()

find_package(VTK NO_MODULE REQUIRED COMPONENTS ${VTK_REQUIRED_COMPONENTS})
find_package(CURL REQUIRED)

include(${VTK_USE_FILE})

# include export header modules so that we can easily control symbol exporting
# VTK Map is setup by default not to export symbols unless explicitly stated.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
include(GenerateExportHeader)

# Specify GDAL components (used by vtkGDALRasterReprojection)
find_package(GDAL REQUIRED)
mark_as_advanced(GDAL_INCLUDE_DIR GDAL_LIBRARY)

# Generate header file for map marker geometry
include("EncodeCStringFunctions")
set(marker_vtk "${CMAKE_SOURCE_DIR}/assets/pointMarkerPolyData.vtk")
set(marker_h "${CMAKE_BINARY_DIR}/pointMarkerPolyData.h")
configureFileAsCVariable("${marker_vtk}" "${marker_h}" "pointMarkerPolyData")

# Copy header file for map marker shadow
set(markers_shadow_input "${CMAKE_SOURCE_DIR}/assets/markersShadowImageData.h")
set(markers_shadow_h "${CMAKE_BINARY_DIR}/markersShadowImageData.h")
configure_file("${markers_shadow_input}" "${markers_shadow_h}" COPYONLY)


# Specify source files
set (SOURCES
    vtkFeature.cxx
    vtkFeatureLayer.cxx
    vtkGDALRasterConverter.cxx
    vtkGDALRasterReprojection.cxx
    vtkGeoJSONMapFeature.cxx
    vtkGeoMapFeatureSelector.cxx
    vtkGeoMapSelection.cxx
    vtkInteractorStyleGeoMap.cxx
    vtkMapMarkerSet.cxx
    vtkMapTile.cxx
    vtkMap.cxx
    vtkMultiThreadedOsmLayer.cxx
    vtkLayer.cxx
    vtkOsmLayer.cxx
    vtkPolydataFeature.cxx
    vtkRasterFeature.cxx
    vtkRasterReprojectionFilter.cxx
    vtkTeardropSource.cxx
    )

#headers that we are going to install
set (HEADERS
    vtkFeature.h
    vtkFeatureLayer.h
    vtkGDALRasterConverter.h
    vtkGDALRasterReprojection.h
    vtkGeoMapFeatureSelector.h
    vtkGeoMapSelection.h
    vtkInteractorStyleGeoMap.h
    vtkMapMarkerSet.h
    vtkMapTile.h
    vtkMapTileSpecInternal.h
    vtkMap.h
    vtkMercator.h
    vtkLayer.h
    vtkMultiThreadedOsmLayer.h
    vtkOsmLayer.h
    vtkPolydataFeature.h
    vtkRasterFeature.h
    vtkRasterReprojectionFilter.h
    vtkTeardropSource.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtkmap_export.h
    )


# Specify targets
add_library(vtkMap ${SOURCES})

target_link_libraries(vtkMap
                      LINK_PUBLIC
                        ${VTK_LIBRARIES}
                      LINK_PRIVATE
                        ${CURL_LIBRARIES}
                        ${GDAL_LIBRARY}
                      )

target_include_directories(vtkMap
                           PUBLIC
                             ${CMAKE_CURRENT_BINARY_DIR}
                           PRIVATE
                             ${CURL_INCLUDE_DIRS}
                             ${GDAL_INCLUDE_DIRS}
                          )

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

# Unset flags for executables that are built downstream
set(CMAKE_CXX_VISIBILITY_PRESET)
set(CMAKE_VISIBILITY_INLINES_HIDDEN)

#example does not need to installed
add_executable(example example.cpp)
target_link_libraries(example vtkMap)

#both testing and Qt do need to exported or installed as they are for testing
#and examples
add_subdirectory(Testing)
if (BUILD_QT_APPS)
  add_subdirectory(Qt)
endif()
