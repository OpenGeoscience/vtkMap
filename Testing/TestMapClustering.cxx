/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMapClustering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMap.h"
#include "vtkMapMarkerSet.h"
#include "vtkOsmLayer.h"
#include <vtkCallbackCommand.h>
#include <vtkInteractorStyle.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPicker.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <iostream>


void scrollCallback(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
                    void* clientData, void* vtkNotUsed(callData) )
{
  vtkMap* map = static_cast<vtkMap*>(clientData);
  map->Draw();
}



int main(int, char*[])
{
  vtkNew<vtkMap> map;
  // Always set map's renderer *before* adding layers
  vtkNew<vtkRenderer> renderer;
  map->SetRenderer(renderer.GetPointer());

  vtkNew<vtkOsmLayer> osmLayer;
  map->AddLayer(osmLayer.GetPointer());

  map->SetCenter(42.849604, -73.758345);  // KHQ
  map->SetZoom(10);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindow->SetSize(640, 640);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.GetPointer());
  interactor->SetInteractorStyle(map->GetInteractorStyle());
  //interactor->SetPicker(map->GetPicker());  // not used
  interactor->Initialize();
  map->Draw();

  //std::cout << "Test" << std::endl;
  double latlonCoords[][2] = {
    42.915081,  -73.805122,  // Country Knolls
    42.902851,  -73.687340,  // Mechanicville
    42.792580,  -73.681229,  // Waterford
    42.774239,  -73.700119   // Cohoes
  };

  vtkMapMarkerSet *markerSet = map->GetMapMarkerSet();
  markerSet->ClusteringOn();
  unsigned numMarkers = sizeof(latlonCoords) / sizeof(double) / 2;
  for (unsigned i=0; i<numMarkers; ++i)
    {
    double lat = latlonCoords[i][0];
    double lon = latlonCoords[i][1];
    markerSet->AddMarker(lat, lon);
    }
  map->Draw();

  // Set callbacks for DO_INTERACTOR
  vtkNew<vtkCallbackCommand> callback;
  callback->SetClientData(map.GetPointer());
  callback->SetCallback(scrollCallback);
  interactor->AddObserver(vtkCommand::MouseWheelForwardEvent,
                          callback.GetPointer());
  interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent,
                          callback.GetPointer());

  interactor->Start();

  return EXIT_SUCCESS;
}
