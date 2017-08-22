# ==============================================================================
# Source
# ==============================================================================
set(APP_SOURCE
  wsmap.cxx
  qtWeatherStations.cxx
  qtMapCoordinatesWidget.cxx
  )

set(UI_FILES
  qtMapCoordinatesWidget.ui
  qtWeatherStations.ui
  )

set(QT_WRAP
  qtMapCoordinatesWidget.h
  qtWeatherStations.h
  )

# ==============================================================================
# Target
# ==============================================================================
add_executable(wsmap
  ${APP_SOURCE}
  ${UISrcs}
  ${MOCSrcs}
  )

set_target_properties(wsmap
  PROPERTIES
    AUTOMOC TRUE
    AUTORCC TRUE
    AUTOUIC TRUE
  )

target_link_libraries(wsmap
  vtkMapCore
  ${CURL_LIBRARIES}
  )

target_include_directories(wsmap
                           PRIVATE
                             ${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}
                             ${CMAKE_SOURCE_DIR}
                          )
