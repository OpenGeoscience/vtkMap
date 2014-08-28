add_executable(wsmap wsmap.cxx qtWSMapMainWindow)
target_link_libraries(wsmap ${VTK_LIBRARIES} ${QT_LIBRARIES} -lcurl)

add_executable(RenderWindowNoUiFile RenderWindowNoUiFile.cxx)
target_link_libraries(RenderWindowNoUiFile ${VTK_LIBRARIES})
