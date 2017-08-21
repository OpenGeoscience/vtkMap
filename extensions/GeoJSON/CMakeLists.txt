# ==============================================================================
# Source
# ==============================================================================
set(vtkMapGeoJSON_SOURCE
  vtkGeoJSONMapFeature.cxx
  )

set(vtkMapGeoJSON_HEADERS
  vtkGeoJSONMapFeature.h
  )

# ==============================================================================
# Target
# ==============================================================================
add_library(vtkMapGeoJSON ${vtkMapGeoJSON_SOURCE})

target_link_libraries(vtkMapGeoJSON
                      LINK_PUBLIC
                         vtkMapCore
                      LINK_PRIVATE
                      )

target_include_directories(vtkMapGeoJSON
                           PUBLIC
                             ${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}
                          )

generate_export_header(vtkMapGeoJSON)

vtkmap_install_target(vtkMapGeoJSON)
install (FILES ${vtkMapGeoJSON_HEADERS} DESTINATION include)
