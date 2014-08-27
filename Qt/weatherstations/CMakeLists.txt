add_executable(wsmap wsmap.cxx qtWSMapMainWindow)
target_link_libraries(wsmap ${VTK_LIBRARIES} ${QT_LIBRARIES} -lcurl)
