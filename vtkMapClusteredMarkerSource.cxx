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
  this->SetNumberOfOutputPorts(2);
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
  // get the info objects
  vtkInformation *markerInfo = outputVector->GetInformationObject(0);
  vtkInformation *clusterInfo = outputVector->GetInformationObject(1);

  // get the outputs
  vtkPolyData *markerPolyData = vtkPolyData::SafeDownCast(
    markerInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *clusterPolyData = vtkPolyData::SafeDownCast(
    markerInfo->Get(vtkDataObject::DATA_OBJECT()));

  // check for Colors in output point data
  if (!markerPolyData->GetPointData()->HasArray("Colors"))
    {
    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetName("Colors");
    colors->SetNumberOfComponents(3);
    markerPolyData->GetPointData()->AddArray(colors.GetPointer());
    }
  vtkDataArray *data = markerPolyData->GetPointData()->GetArray("Colors");
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::SafeDownCast(data);
  colors->Reset();
  unsigned char kwBlue[] = {0, 83, 155};

  // copy clusters into markerPolyData
  vtkNew<vtkPoints> markerPoints;
  double coord[3];

  std::set<MapCluster*> clusterSet = this->Internals->ClusterTable[0];
  std::set<MapCluster*>::const_iterator iter;
  for (iter = clusterSet.begin(); iter != clusterSet.end(); iter++)
    {
    MapCluster *cluster = *iter;
    coord[0] = cluster->Longitude;
    coord[1] = lat2y(cluster->Latitude);
    coord[2] = 0.0;
    markerPoints->InsertNextPoint(coord);
    colors->InsertNextTupleValue(kwBlue);
    }
  markerPolyData->Reset();
  markerPolyData->SetPoints(markerPoints.GetPointer());

  return 1;
}

//----------------------------------------------------------------------------
int vtkMapClusteredMarkerSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *markerInfo = outputVector->GetInformationObject(0);
  markerInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);
  vtkInformation *clusterInfo = outputVector->GetInformationObject(1);
  clusterInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);
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
  cluster->NumberOfMarkers = 1;
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

