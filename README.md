vtkMap is a project to add support for Geovisualization in 2D and 3D in VTK.

As of November 2014, this code depends on vtkDataSetReaders, at
https://github.com/OpenGeoscience/vtkDataSetReaders.
That code is used for reading GeoJSON files into map features.
You can build the minimum files needed by vtkMap by setting these
two options in vtkDataSetReaders:

* BUILD_LAS OFF
* BUILD_POSTGRESS OFF


Huboard: https://huboard.com/OpenGeoscience/vtkMap
