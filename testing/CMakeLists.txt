# ==============================================================================
# Source
# ==============================================================================
include_directories(${CMAKE_SOURCE_DIR})

set (CORE_TESTS
  TestMapClustering
  TestMultiThreadedOsmLayer
  TestOsmLayer
  TestRemoveLayer
)

set (EXT_GDAL_TESTS
  TestGDALRasterConverter
  TestGDALRaster
  TestRasterReprojectionFilter
)

set (EXT_GeoJSON_TESTS
  TestGeoJSON
)


# ==============================================================================
# Target
# ==============================================================================

foreach(name ${CORE_TESTS})
  add_executable(${name} ${name}.cxx)
  target_link_libraries(${name}
    LINK_PRIVATE
      vtkMapCore
  )
endforeach()


if (BUILD_GDAL_EXTENSION)
  foreach(name ${EXT_GDAL_TESTS})
    add_executable(${name} ${name}.cxx)
    target_include_directories(${name}
      PRIVATE
        ${GDAL_INCLUDE_DIR}
    )
    target_link_libraries(${name}
      LINK_PRIVATE
        vtkMapCore
        vtkMapGDAL
        ${GDAL_LIBRARY} 
    )
  endforeach()
endif()


if (BUILD_GeoJSON_EXTENSION)
  foreach(name ${EXT_GeoJSON_TESTS})
    add_executable(${name} ${name}.cxx)

    target_link_libraries(${name}
      LINK_PRIVATE
        vtkMapCore
        vtkMapGeoJSON
    )
  endforeach()
endif()
