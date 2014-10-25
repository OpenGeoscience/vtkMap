include_directories(${CMAKE_SOURCE_DIR})
set (TESTS
  TestMapClustering
  TestOsmLayer
)

foreach(name ${TESTS})
  add_executable(${name} ${name}.cxx)
  target_link_libraries(${name} vtkMap)
endforeach()
