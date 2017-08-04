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
#include "vtkMultiThreadedOsmLayer.h"

#include <vtkCommand.h>
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
// For tracking map's async state changes
class TimerCallback : public vtkCommand
{
public:
  static TimerCallback *New()
    { return new TimerCallback; }

  // Output all changes in map AsyncState
  virtual void Execute(vtkObject *vtkNotUsed(caller),
                       unsigned long vtkNotUsed(eventId),
                       void *vtkNotUsed(clientData))
    {
      vtkMap::AsyncState state = this->Map->GetAsyncState();
      if (state != this->MapAsyncState)
        {
        std::string text;
        switch (state)
          {
          case vtkMap::AsyncOff: text = "OFF"; break;
          case vtkMap::AsyncIdle: text = "IDLE"; break;
          case vtkMap::AsyncPending: text = "PENDING"; break;
          case vtkMap::AsyncPartialUpdate: text = "PARTIAL_UPDATE"; break;
          case vtkMap::AsyncFullUpdate: text = "FULL_UPDATE"; break;
          default: text = "UNKNOWN"; break;
          }
        std::cout << text << std::endl;
        }
      this->MapAsyncState = state;
    }

  void SetMap(vtkMap *map)
  {
    this->Map = map;
  }

protected:
  TimerCallback()
  {
    this->Map = NULL;
    this->MapAsyncState = vtkMap::AsyncOff;
  }

  vtkMap *Map;
  vtkMap::AsyncState MapAsyncState;
};

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
int TestMultiThreadedOsmLayer(int argc, char* argv[])
{
  vtkNew<vtkMap> map;

  vtkNew<vtkRenderer> renderer;
  map->SetRenderer(renderer.GetPointer());
  map->SetCenter(0.0, 0.0);
  map->SetZoom(1);

  vtkNew<vtkMultiThreadedOsmLayer> osmLayer;
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
  map->SetInteractor(interactor.GetPointer());
  interactor->Initialize();

  // Setup observer to track async state changes
  interactor->CreateRepeatingTimer(101);
  vtkNew<TimerCallback> timerCallback;
  timerCallback->SetMap(map.GetPointer());
  interactor->AddObserver(vtkCommand::TimerEvent,
                          timerCallback.GetPointer());

  map->Draw();
  interactor->Start();

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  TestMultiThreadedOsmLayer(argc, argv);
}
