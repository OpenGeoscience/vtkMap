# ==============================================================================
# Dependencies
# ==============================================================================
# include export header modules so that we can easily control symbol exporting
# VTK Map is setup by default not to export symbols unless explicitly stated.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

## Specify GDAL components (used by vtkGDALRasterReprojection)
find_package(GDAL REQUIRED)
mark_as_advanced(GDAL_INCLUDE_DIR GDAL_LIBRARY)

# Unset flags for executables that are built downstream
set(CMAKE_CXX_VISIBILITY_PRESET)
set(CMAKE_VISIBILITY_INLINES_HIDDEN)

# ==============================================================================
# Source
# ==============================================================================
set(vtkMapGDAL_SOURCE
  vtkGDALRasterConverter.cxx
  vtkGDALRasterReprojection.cxx
  vtkGDALRasterFeature.cxx
  vtkRasterReprojectionFilter.cxx
  )

set(vtkMapGDAL_HEADERS
  vtkGDALRasterConverter.h
  vtkGDALRasterReprojection.h
  vtkGDALRasterFeature.h
  vtkRasterReprojectionFilter.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkmapgdal_export.h
  )

# ==============================================================================
# Target
# ==============================================================================
add_library(vtkMapGDAL ${vtkMapGDAL_SOURCE})

target_link_libraries(vtkMapGDAL
                      LINK_PUBLIC
                         vtkMapCore
                      LINK_PRIVATE
                        ${GDAL_LIBRARY}
                      )

target_include_directories(vtkMapGDAL
                           PUBLIC
                             ${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}
                           PRIVATE
                             ${GDAL_INCLUDE_DIR}
                          )

generate_export_header(vtkMapGDAL)

vtkmap_install_target(vtkMapGDAL)
install (FILES ${vtkMapGDAL_HEADERS} DESTINATION include)
