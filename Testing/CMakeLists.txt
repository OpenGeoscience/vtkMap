include_directories(${CMAKE_SOURCE_DIR})
set (SOURCES
  TestMapClustering.cxx
  ${CMAKE_SOURCE_DIR}/vtkMap.cxx
  ${CMAKE_SOURCE_DIR}/vtkMapMarkerSet.cxx
  ${CMAKE_SOURCE_DIR}/vtkMapTile.cxx
  ${CMAKE_SOURCE_DIR}/vtkTeardropSource.cxx
)
add_executable(TestMapClustering ${SOURCES})
target_link_libraries(TestMapClustering ${VTK_LIBRARIES} -lcurl)
