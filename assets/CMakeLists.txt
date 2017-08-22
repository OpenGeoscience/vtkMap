include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Generate header file for map marker geometry
include("EncodeCStringFunctions")
set(marker_vtk "${CMAKE_SOURCE_DIR}/assets/pointMarkerPolyData.vtk")
set(marker_h "${CMAKE_BINARY_DIR}/pointMarkerPolyData.h")
configureFileAsCVariable("${marker_vtk}" "${marker_h}" "pointMarkerPolyData")

# Copy header file for map marker shadow
set(markers_shadow_input "${CMAKE_SOURCE_DIR}/assets/markersShadowImageData.h")
set(markers_shadow_h "${CMAKE_BINARY_DIR}/markersShadowImageData.h")
configure_file("${markers_shadow_input}" "${markers_shadow_h}" COPYONLY)

set(assets_HEADERS
  markersShadowImage.h
  pointMarkerPolyData.h
  )
