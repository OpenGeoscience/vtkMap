/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreading.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMap.h"
#include "vtkOsmLayer.h"

#include <vtkInteractorStyle.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtksys/SystemTools.hxx>

#include <cstdlib>  // for system()
#include <iostream>
#include <string>

//----------------------------------------------------------------------------
void SetupCacheDirectory(vtkMap *map, std::string& dirname)
{
  // Setup cache directory for testing
  std::string storageDir = map->GetStorageDirectory();
  std::string testDir = storageDir + "/" + dirname;

  // Create directory if it doesn't already exist
  if(!vtksys::SystemTools::FileIsDirectory(testDir.c_str()))
    {
    std::cerr << "Creating test directory " << testDir << std::endl;
    vtksys::SystemTools::MakeDirectory(testDir.c_str());
    }

  // Make sure directory is empty
  std::string command = "exec rm -rf " + testDir + "/*";
  int retval = system(command.c_str());
}

//----------------------------------------------------------------------------
int TestOsmLayer(int argc, char* argv[])
{
  vtkNew<vtkMap> map;

  vtkNew<vtkRenderer> renderer;
  map->SetRenderer(renderer.GetPointer());
  map->SetCenter(0.0, 0.0);
  map->SetZoom(1);

  vtkNew<vtkOsmLayer> osmLayer;
  //osmLayer->DebugOn();
  map->AddLayer(osmLayer.GetPointer());

  // Argument 1 specifies test directory (optional)
  if (argc > 1)
    {
    map->SetStorageDirectory(argv[1]);
    }
  std::string cacheDir = "test";
  SetupCacheDirectory(map.GetPointer(), cacheDir);
  osmLayer->SetCacheSubDirectory(cacheDir.c_str());

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindow->SetSize(500, 500);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.GetPointer());
  interactor->SetInteractorStyle(map->GetInteractorStyle());
  interactor->Initialize();
  map->Draw();

  interactor->Start();

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  TestOsmLayer(argc, argv);
}
