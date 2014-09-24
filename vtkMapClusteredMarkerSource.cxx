/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapClusteredMarkerSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapClusteredMarkerSource.h"
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <algorithm>
#include <set>
#include <vector>

double lat2y(double);

//----------------------------------------------------------------------------
namespace
{
  class MapCluster
  {
  public:
    double Latitude;
    double Longitude;
    // int ZoomLevel TBD
    std::set<MapCluster*> ChildrenClusters;
    int NumberOfMarkers;
    int MarkerId;
  };
  const int NumberOfClusterLevels = 1;
}

//----------------------------------------------------------------------------
class vtkMapClusteredMarkerSource::MapClusteredMarkerSourceInternals
{
public:
  std::vector<std::set<MapCluster*> > ClusterTable;
  int NumberOfMarkers;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMapClusteredMarkerSource)

//----------------------------------------------------------------------------
vtkMapClusteredMarkerSource::vtkMapClusteredMarkerSource()
{
  this->SetNumberOfInputPorts(0);
  this->Internals = new MapClusteredMarkerSourceInternals;
  std::set<MapCluster*> clusterSet;
  std::fill_n(std::back_inserter(this->Internals->ClusterTable),
              NumberOfClusterLevels, clusterSet);
  this->Internals->NumberOfMarkers = 0;
}

//----------------------------------------------------------------------------
void vtkMapClusteredMarkerSource::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkMapClusteredMarkerSource::~vtkMapClusteredMarkerSource()
{
  this->RemoveMapMarkers();
  delete this->Internals;
}

//----------------------------------------------------------------------------
int vtkMapClusteredMarkerSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *info = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData *polyData = vtkPolyData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  // Setup "MarkerType" as point data (vector)
  const char *arrayName = "MarkerType";
  // Check for array in point data
  if (!polyData->GetPointData()->HasArray(arrayName))
    {
    vtkNew<vtkUnsignedCharArray> newArray;
    newArray->SetName(arrayName);
    newArray->SetNumberOfComponents(3);
    polyData->GetPointData()->AddArray(newArray.GetPointer());
    }
  vtkDataArray *data = polyData->GetPointData()->GetArray(arrayName);
  vtkUnsignedCharArray *types = vtkUnsignedCharArray::SafeDownCast(data);
  types->Reset();

  // Setup "Colors" array as point data
  vtkUnsignedCharArray *colors = this->SetupColorsArray(polyData);
  unsigned char kwBlue[] = {0, 83, 155};
  unsigned char kwGreen[] = {0, 169, 179};

  // copy clusters into polyData
  vtkNew<vtkPoints> points;
  double coord[3];
  unsigned char markerTypeVector[3];
  markerTypeVector[0] = markerTypeVector[1] = markerTypeVector[2] = 0;

  std::set<MapCluster*> clusterSet = this->Internals->ClusterTable[0];
  std::set<MapCluster*>::const_iterator iter;
  for (iter = clusterSet.begin(); iter != clusterSet.end(); iter++)
    {
    MapCluster *cluster = *iter;
    coord[0] = cluster->Longitude;
    coord[1] = lat2y(cluster->Latitude);
    coord[2] = 0.0;
    points->InsertNextPoint(coord);
    if (cluster->NumberOfMarkers == 1)  // marker
      {
      markerTypeVector[0] = 0;
      types->InsertNextTupleValue(markerTypeVector);
      colors->InsertNextTupleValue(kwBlue);
      }
    else  // cluster
      {
      markerTypeVector[0] = 1;
      types->InsertNextTupleValue(markerTypeVector);
      colors->InsertNextTupleValue(kwGreen);
      }
    }
  polyData->Reset();
  polyData->SetPoints(points.GetPointer());

  //std::cout << "Marker pts " << markerPoints->GetNumberOfPoints() << std::endl;
  //std::cout << "Cluster pts " << clusterPoints->GetNumberOfPoints() << std::endl;

  return 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkMapClusteredMarkerSource::AddMarker(double latitude,
                                                 double longitude)
{
  // Instantiate MapCluster instance
  MapCluster *cluster = new MapCluster;
  cluster->Latitude = latitude;
  cluster->Longitude = longitude;
  //cluster->NumberOfMarkers = 1;
  cluster->NumberOfMarkers = 1 + (this->Internals->NumberOfMarkers % 2);
  cluster->MarkerId = this->Internals->NumberOfMarkers++;

  // Hard code to level 0 for now
  this->Internals->ClusterTable[0].insert(cluster);

  this->Modified();
  return cluster->MarkerId;
}

//----------------------------------------------------------------------------
void vtkMapClusteredMarkerSource::RemoveMapMarkers()
{
  std::vector<std::set<MapCluster*> >::iterator tableIter =
    this->Internals->ClusterTable.begin();
  for (; tableIter != this->Internals->ClusterTable.end(); tableIter++)
    {
    std::set<MapCluster*> clusterSet = *tableIter;
    std::set<MapCluster*>::iterator setIter = clusterSet.begin();
    for (; setIter != clusterSet.end(); setIter++)
      {
      delete *setIter;
      }
    clusterSet.empty();
    }
  this->Internals->NumberOfMarkers = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray *
vtkMapClusteredMarkerSource::SetupColorsArray(vtkPolyData *polyData)
{
  const char *arrayName = "Color";
  // Check for array in point data
  if (!polyData->GetPointData()->HasArray(arrayName))
    {
    vtkNew<vtkUnsignedCharArray> newArray;
    newArray->SetName(arrayName);
    newArray->SetNumberOfComponents(3);
    polyData->GetPointData()->AddArray(newArray.GetPointer());
    }
  vtkDataArray *data = polyData->GetPointData()->GetArray(arrayName);
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::SafeDownCast(data);
  colors->Reset();
  return colors;
}

