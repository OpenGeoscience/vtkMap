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
#include <vtkIdTypeArray.h>
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
    int ClusterId;  // position in ClusterList vector
    double Latitude;
    double Longitude;
    // int ZoomLevel TBD
    std::set<MapCluster*> ChildrenClusters;
    int NumberOfMarkers;
    int MarkerId;  // only relevant for single-marker clusters
  };
  const int NumberOfClusterLevels = 1;
}

//----------------------------------------------------------------------------
class vtkMapClusteredMarkerSource::MapClusteredMarkerSourceInternals
{
public:
  std::vector<MapCluster*> ClusterList;
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
  int numberOfClusters = this->Internals->ClusterList.size() -
    this->Internals->NumberOfMarkers;
  os << indent << "vtkMapClusteredMarkerSource" << "\n"
     << indent << "NumberOfClusterLevels: " << NumberOfClusterLevels << "\n"
     << indent << "NumberOfMarkers: " << this->Internals->NumberOfMarkers << "\n"
     << indent << "Number of clusters: " << numberOfClusters << "\n"
     << std::endl;
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
  // Get output polydata object
  vtkInformation *info = outputVector->GetInformationObject(0);
  vtkPolyData *polyData = vtkPolyData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  // Setup ClusterId array
  const char* clusterIdName = "ClusterId";
  if (!polyData->GetPointData()->HasArray(clusterIdName))
    {
    vtkNew<vtkIdTypeArray> newArray;
    newArray->SetName(clusterIdName);
    polyData->GetPointData()->AddArray(newArray.GetPointer());
    }
  vtkDataArray *data = polyData->GetPointData()->GetArray(clusterIdName);
  vtkIdTypeArray *clusterIds = vtkIdTypeArray::SafeDownCast(data);
  clusterIds->Reset();

  // Setup "MarkerType" point data array (1 component vector)
  vtkUnsignedCharArray *types =
    this->SetupUCharArray(polyData, "MarkerType", 1);

  // Setup "Color" array as point data (vector)
  vtkUnsignedCharArray *colors = this->SetupUCharArray(polyData, "Color", 3);
  unsigned char kwBlue[] = {0, 83, 155};
  unsigned char kwGreen[] = {0, 169, 179};

  // Copy cluster info into polydata
  vtkNew<vtkPoints> points;
  double coord[3];
  unsigned char markerType[1];
  markerType[0] = 0;

  std::set<MapCluster*> clusterSet = this->Internals->ClusterTable[0];
  std::set<MapCluster*>::const_iterator iter;
  for (iter = clusterSet.begin(); iter != clusterSet.end(); iter++)
    {
    MapCluster *cluster = *iter;
    coord[0] = cluster->Longitude;
    coord[1] = lat2y(cluster->Latitude);
    coord[2] = 0.0;
    points->InsertNextPoint(coord);
    clusterIds->InsertNextValue(cluster->ClusterId);
    if (cluster->NumberOfMarkers == 1)  // point marker
      {
      markerType[0] = 0;
      types->InsertNextTupleValue(markerType);
      colors->InsertNextTupleValue(kwBlue);
      }
    else  // cluster marker
      {
      markerType[0] = 1;
      types->InsertNextTupleValue(markerType);
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
  // Set marker id
  int markerId = this->Internals->NumberOfMarkers++;

  // Instantiate MapCluster instance
  MapCluster *cluster = new MapCluster;
  cluster->Latitude = latitude;
  cluster->Longitude = longitude;

  // Todo - insert into cluster table
  // For now, make every other one a cluster
  if (this->Internals->NumberOfMarkers % 2)
    {
    cluster->NumberOfMarkers = 2 + markerId;
    cluster->MarkerId = -1;
    }
  else
    {
    cluster->NumberOfMarkers = 1;
    cluster->MarkerId = markerId;
    }
  cluster->ClusterId = this->Internals->ClusterList.size();
  this->Internals->ClusterList.push_back(cluster);

  // Hard code to level 0 for now
  this->Internals->ClusterTable[0].insert(cluster);

  this->Modified();
  return markerId;
}

//----------------------------------------------------------------------------
void vtkMapClusteredMarkerSource::RemoveMapMarkers()
{
  std::vector<std::set<MapCluster*> >::iterator tableIter =
    this->Internals->ClusterTable.begin();
  for (; tableIter != this->Internals->ClusterTable.end(); tableIter++)
    {
    std::set<MapCluster*> clusterSet = *tableIter;
    clusterSet.empty();
    }

  std::vector<MapCluster*>::iterator listIter =
    this->Internals->ClusterList.begin();
  for (; listIter != this->Internals->ClusterList.end(); listIter++)
    {
    delete *listIter;
    }
  this->Internals->ClusterList.clear();

  this->Internals->NumberOfMarkers = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray *
vtkMapClusteredMarkerSource::
SetupUCharArray(vtkPolyData *polyData, const char *name, int numberOfComponents)
{
  // Check for array in point data
  if (!polyData->GetPointData()->HasArray(name))
    {
    vtkNew<vtkUnsignedCharArray> newArray;
    newArray->SetName(name);
    newArray->SetNumberOfComponents(numberOfComponents);
    polyData->GetPointData()->AddArray(newArray.GetPointer());
    }
  vtkDataArray *data = polyData->GetPointData()->GetArray(name);
  vtkUnsignedCharArray *array = vtkUnsignedCharArray::SafeDownCast(data);
  array->Reset();
  return array;
}

