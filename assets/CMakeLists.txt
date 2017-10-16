include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
include("EncodeCStringFunctions")

# Generate header files for the marker's PolyData (in build/assets folder)
set(marker_names teardrop square triangle pentagon hexagon octagon)
foreach (name ${marker_names})
	set(marker_vtk "${CMAKE_SOURCE_DIR}/assets/${name}.vtk")
	set(marker_h "${CMAKE_BINARY_DIR}/assets/${name}.h")
	configureFileAsCVariable("${marker_vtk}" "${marker_h}" "${name}")
endforeach()

# Copy header file for map marker shadow
set(markers_shadow_input "${CMAKE_SOURCE_DIR}/assets/markersShadowImageData.h")
set(markers_shadow_h "${CMAKE_BINARY_DIR}/markersShadowImageData.h")
configure_file("${markers_shadow_input}" "${markers_shadow_h}" COPYONLY)

# Copy header file for tile-not-available image
set(tile_image_input "${CMAKE_SOURCE_DIR}/assets/tileNotAvailable_png.h")
set(tile_image_h "${CMAKE_BINARY_DIR}/tileNotAvailable_png.h")
configure_file("${tile_image_input}" "${tile_image_h}" COPYONLY)

set(assets_HEADERS
  markersShadowImage.h
  tileNotAvailable_png.h
  )
