include_directories(${CMAKE_SOURCE_DIR})
add_executable(wsmap
  wsmap.cxx
  qtWSMapMainWindow
  ${CMAKE_SOURCE_DIR}/vtkMapTile.cxx
  ${CMAKE_SOURCE_DIR}/vtkMap.cxx
)
target_link_libraries(wsmap ${VTK_LIBRARIES} ${QT_LIBRARIES} -lcurl)

add_executable(RenderWindowNoUiFile
  RenderWindowNoUiFile.cxx
  ${CMAKE_SOURCE_DIR}/vtkMapTile.cxx
  ${CMAKE_SOURCE_DIR}/vtkMap.cxx
)
target_link_libraries(RenderWindowNoUiFile ${VTK_LIBRARIES} -lcurl)
