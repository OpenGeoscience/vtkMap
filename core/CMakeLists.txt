# ==============================================================================
# Source
# ==============================================================================
set(vtkMapCore_SOURCE
    Timer.h
    vtkFeature.cxx
    vtkFeatureLayer.cxx
    vtkGeoMapFeatureSelector.cxx
    vtkGeoMapLayerPass.cxx
    vtkGeoMapSelection.cxx
    vtkInteractorStyleGeoMap.cxx
    vtkMapMarkerSet.cxx
    vtkMapPointSelection.cxx
    vtkMapTile.cxx
    vtkMap.cxx
    vtkMultiThreadedOsmLayer.cxx
    vtkLayer.cxx
    vtkOsmLayer.cxx
    vtkPolydataFeature.cxx
    vtkRasterFeature.cxx
    vtkTeardropSource.cxx
    )

set (vtkMapCore_HEADERS
    vtkFeature.h
    vtkFeatureLayer.h
    vtkGeoMapFeatureSelector.h
    vtkGeoMapLayerPass.h
    vtkGeoMapSelection.h
    vtkInteractorStyleGeoMap.h
    vtkMapMarkerSet.h
    vtkMapPointSelection.cxx
    vtkMapTile.h
    vtkMapTileSpecInternal.h
    vtkMap.h
    vtkMap_typedef.h
    vtkMercator.h
    vtkLayer.h
    vtkMultiThreadedOsmLayer.h
    vtkOsmLayer.h
    vtkPolydataFeature.h
    vtkRasterFeature.h
    vtkTeardropSource.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtkmapcore_export.h
    )

# ==============================================================================
# Target
# ==============================================================================
add_library(vtkMapCore ${vtkMapCore_SOURCE})

set(CMAKE_CXX_VISIBILITY_PRESET)
set(CMAKE_VISIBILITY_INLINES_HIDDEN)

target_link_libraries(vtkMapCore
                      LINK_PUBLIC
                        ${VTK_LIBRARIES}
                      LINK_PRIVATE
                        ${CURL_LIBRARIES}
                        ${OPENGL_LIBRARIES}
                      )

target_include_directories(vtkMapCore
                           PUBLIC
                             ${CMAKE_BINARY_DIR}
                             ${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}
                             ${CURL_INCLUDE_DIRS}
                           PRIVATE
                             ${OPENGL_INCLUDE_DIRS}
                          )

generate_export_header(vtkMapCore)

vtkmap_install_target(vtkMapCore)
install (FILES ${vtkMapCore_HEADERS} DESTINATION include)
