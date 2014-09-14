set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/ThirdParty)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${THIRD_PARTY_DIR}
)

set(UI_FILES qtMapCoordinatesWidget.ui qtWeatherStations.ui)
set(QT_WRAP qtMapCoordinatesWidget.h qtWeatherStations.h)
QT4_WRAP_UI(UISrcs ${UI_FILES})
QT4_WRAP_CPP(MOCSrcs ${QT_WRAP})

add_executable(wsmap
  wsmap.cxx
  qtWeatherStations.cxx
  qtMapCoordinatesWidget.cxx
  ${CMAKE_SOURCE_DIR}/vtkMapMarker.cxx
  ${CMAKE_SOURCE_DIR}/vtkMapTile.cxx
  ${CMAKE_SOURCE_DIR}/vtkMap.cxx
  ${THIRD_PARTY_DIR}/cJSON/cJSON.c
  ${UISrcs} ${MOCSrcs}
)
target_link_libraries(wsmap ${VTK_LIBRARIES} -lcurl)

#add_executable(RenderWindowNoUiFile
#  RenderWindowNoUiFile.cxx
#  ${CMAKE_SOURCE_DIR}/vtkMapMarker.cxx
#  ${CMAKE_SOURCE_DIR}/vtkMapTile.cxx
#  ${CMAKE_SOURCE_DIR}/vtkMap.cxx
#)
#target_link_libraries(RenderWindowNoUiFile ${VTK_LIBRARIES} -lcurl)
