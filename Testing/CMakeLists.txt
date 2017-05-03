include_directories(${CMAKE_SOURCE_DIR})
set (TEST_NAMES
  TestMapClustering
  TestMultiThreadedOsmLayer
  TestOsmLayer
)
if(NOT TINY_BUILD)
  list(APPEND TEST_NAMES
    TestGDALRasterConverter
    TestGDALRaster
    TestGeoJSON
    TestRasterReprojectionFilter
  )
endif()

foreach(name ${TEST_NAMES})
  add_executable(${name} ${name}.cxx)
  target_include_directories(${name}
    PUBLIC
      ${CMAKE_BINARY_DIR}
    PRIVATE
      ${CURL_INCLUDE_DIRS}
  )
  target_link_libraries(${name}
    LINK_PUBLIC
      vtkMap
      ${VTK_LIBRARIES}
    LINK_PRIVATE
      ${CURL_LIBRARIES}
  )
  if(NOT TINY_BUILD)
    target_include_directories(${name}
      PRIVATE
        ${GDAL_INCLUDE_DIRS}
        ${READERS_INCLUDE_DIRECTORY}
    )
    target_link_libraries(${name}
      LINK_PUBLIC
        ${READERS_LIBRARIES}
      LINK_PRIVATE
        ${GDAL_LIBRARY}
    )
  endif()
endforeach()
