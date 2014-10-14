include_directories(${CMAKE_SOURCE_DIR})
set (SOURCES
  TestMapClustering.cxx
)
add_executable(TestMapClustering ${SOURCES})
target_link_libraries(TestMapClustering vtkMap)
