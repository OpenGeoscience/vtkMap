macro(vtkmap_install_target target)
  # On Mac OS X, set the directory included as part of the
  # installed library's path. We only do this to libraries that we plan
  # on installing
  if (BUILD_SHARED_LIBS)
    set_target_properties(${target} PROPERTIES
                          INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
                          )
  else()
    set_target_properties(${target} PROPERTIES
                          POSITION_INDEPENDENT_CODE True
                          )
  endif()

  #now generate very basic install rules
  install(TARGETS ${target}
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
          )
endmacro()
