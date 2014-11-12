include_directories(${CMAKE_SOURCE_DIR})
set (TEST_NAMES
  TestGeoJSON
  TestMapClustering
  TestMultiThreadedOsmLayer
  TestOsmLayer
)

foreach(name ${TEST_NAMES})
  add_executable(${name} ${name}.cxx)
  target_link_libraries(${name} vtkMap)
endforeach()
