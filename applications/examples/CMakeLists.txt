include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Examples are not installed
add_executable(example
  example.cxx)
target_link_libraries(example
  vtkMapCore
  ${CURL_LIBRARIES})

add_executable(exampleLayers
  exampleLayers.cxx)
target_link_libraries(exampleLayers
  vtkMapCore
  ${CURL_LIBRARIES})

add_executable(markerSize
  markerSize.cxx)
target_link_libraries(markerSize
  vtkMapCore
  ${CURL_LIBRARIES})
