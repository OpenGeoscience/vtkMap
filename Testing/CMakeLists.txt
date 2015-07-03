include_directories(${CMAKE_SOURCE_DIR})
set (TEST_NAMES
  TestGDALRasterConverter
  TestGDALRaster
  TestGeoJSON
  TestMapClustering
  TestMultiThreadedOsmLayer
  TestOsmLayer
  TestParallelProjection
  TestRasterReprojectionFilter
)

foreach(name ${TEST_NAMES})
  add_executable(${name} ${name}.cxx)
  target_include_directories(${name}
    PUBLIC
      ${CMAKE_BINARY_DIR}
    PRIVATE
      ${CURL_INCLUDE_DIRS}
      ${GDAL_INCLUDE_DIRS}
      ${READERS_INCLUDE_DIRECTORY}
  )
  target_link_libraries(${name}
    LINK_PUBLIC
      vtkMap
      ${VTK_LIBRARIES}
      ${READERS_LIBRARIES}
    LINK_PRIVATE
      ${CURL_LIBRARIES}
      ${GDAL_LIBRARY}
  )
endforeach()
