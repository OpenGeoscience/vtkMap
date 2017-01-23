/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapMarkerSet.h"
#include "vtkMercator.h"
#include "vtkTeardropSource.h"

#include <vtkActor.h>
#include <vtkBitArray.h>
#include <vtkDataArray.h>
#include <vtkDistanceToCamera.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkGlyph3DMapper.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

const int NumberOfClusterLevels = 20;
unsigned int vtkMapMarkerSet::NextMarkerHue = 0;
#define MARKER_TYPE 0
#define CLUSTER_TYPE 1
#define SQRT_TWO sqrt(2.0)

//----------------------------------------------------------------------------
// Internal class for cluster tree nodes
// Each node represents either one marker or a cluster of nodes
class vtkMapMarkerSet::ClusteringNode
{
public:
  int NodeId;
  int Level;  // for dev
  double gcsCoords[3];
  ClusteringNode *Parent;
  std::set<ClusteringNode*> Children;
  int NumberOfMarkers;  // 1 for single-point nodes, >1 for clusters
  int MarkerId;  // only relevant for single-point markers (not clusters)
  int NumberOfVisibleMarkers;
  int NumberOfSelectedMarkers;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMapMarkerSet)

//----------------------------------------------------------------------------
class vtkMapMarkerSet::MapMarkerSetInternals
{
public:
  vtkGlyph3DMapper *GlyphMapper;
  bool MarkersChanged;
  std::vector<ClusteringNode*> CurrentNodes;  // in this->PolyData

  // Used for marker clustering:
  int ZoomLevel;
  std::vector<std::set<ClusteringNode*> > NodeTable;
  int NumberOfMarkers;
  int NumberOfNodes;  // for dev use
  std::vector<bool> MarkerVisible;  // for single-markers only (not clusters)
  std::vector<bool> MarkerSelected;  // for single-markers only (not clusters)
  std::vector<ClusteringNode*> AllNodes;   // for dev

  // Used to quickly locate non-cluster nodes (ordered by MarkerId)
  std::vector<ClusteringNode*> MarkerNodes;
};

//----------------------------------------------------------------------------
vtkMapMarkerSet::vtkMapMarkerSet() : vtkPolydataFeature()
{
  this->Initialized = false;
  this->ZCoord = 0.1;
  this->SelectedZOffset = 0.0;
  this->PolyData = vtkPolyData::New();
  this->Clustering = false;
  this->ClusterDistance = 80;
  this->MaxClusterScaleFactor = 2.0;

  // Initialize color table
  this->ColorTable = vtkLookupTable::New();
  this->ColorTable->SetNumberOfTableValues(2);
  this->ColorTable->Build();

  // Selected color
  this->SelectionHue = 5.0/6.0;  // magenta (fushia?)
  double hsv[3] = {this->SelectionHue, 1.0, 1.0};
  double rgb[3] = {0.0, 0.0, 0.0};
  vtkMath::HSVToRGB(hsv, rgb);
  double  selected[4] = {rgb[0], rgb[1], rgb[2], 1.0};
  this->ColorTable->SetTableValue(1, selected);

  // Default color
  double color[4] = {0.0, 0.0, 0.0, 1.0};
  this->ComputeNextColor(color);
  this->ColorTable->SetTableValue(0, color);

  // Set Nan color for development & test
  double red[] = {1.0, 0.0, 0.0, 1.0};
  this->ColorTable->SetNanColor(red);

  this->Internals = new MapMarkerSetInternals;
  this->Internals->MarkersChanged = false;
  this->Internals->ZoomLevel = -1;
  std::set<ClusteringNode*> clusterSet;
  std::fill_n(std::back_inserter(this->Internals->NodeTable),
              NumberOfClusterLevels, clusterSet);
  this->Internals->NumberOfMarkers = 0;
  this->Internals->NumberOfNodes = 0;
  this->Internals->GlyphMapper = vtkGlyph3DMapper::New();
  this->Internals->GlyphMapper->SetLookupTable(this->ColorTable);
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << this->GetClassName() << "\n"
     << indent << "Initialized: " << this->Initialized << "\n"
     << indent << "Clustering: " << this->Clustering << "\n"
     << indent << "NumberOfMarkers: "
     << this->Internals->NumberOfMarkers
     << std::endl;
}

//----------------------------------------------------------------------------
vtkMapMarkerSet::~vtkMapMarkerSet()
{
  if (this->PolyData)
    {
    this->PolyData->Delete();
    }
  if (this->ColorTable)
    {
    this->ColorTable->Delete();
    }
  this->Internals->GlyphMapper->Delete();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::SetColor(double rgba[4])
{
  this->ColorTable->SetTableValue(0, rgba);
}

//----------------------------------------------------------------------------
int vtkMapMarkerSet::GetNumberOfMarkers()
{
  return this->Internals->NumberOfMarkers;
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::AddMarker(double latitude, double longitude)
{
  // Set marker id
  int markerId = this->Internals->NumberOfMarkers++;
  vtkDebugMacro("Adding marker " << markerId);

  // if (markerId == 1)
  //   {
  //   vtkRenderer *renderer = this->Layer->GetMap()->GetRenderer();
  //   int *viewPortSize = renderer->GetSize();
  //   std::cout << "Using ViewPort " << viewPortSize[0]
  //             << ", " << viewPortSize[1] << std::endl;
  //   }

  // Insert nodes at bottom level
  int level = this->Internals->NodeTable.size() - 1;

  // Instantiate ClusteringNode
  ClusteringNode *node = new ClusteringNode;
  this->Internals->AllNodes.push_back(node);
  node->NodeId = this->Internals->NumberOfNodes++;
  node->Level = level;
  node->gcsCoords[0] = longitude;
  node->gcsCoords[1] = vtkMercator::lat2y(latitude);
  node->gcsCoords[2] = this->ZCoord;
  node->NumberOfMarkers = 1;
  node->Parent = 0;
  node->MarkerId = markerId;
  node->NumberOfVisibleMarkers = 1;
  node->NumberOfSelectedMarkers = 0;
  vtkDebugMacro("Inserting ClusteringNode " << node->NodeId
                << " into level " << level);
  this->Internals->NodeTable[level].insert(node);
  this->Internals->MarkerVisible.push_back(true);
  this->Internals->MarkerSelected.push_back(false);
  this->Internals->MarkerNodes.push_back(node);

  // todo refactor into separate method
  // todo calc initial cluster distance here and divide down
  if (this->Clustering)
    {
    // Insertion step: Starting at bottom level, populate NodeTable until
    // a clustering partner is found.
    level--;
    double threshold2 = this->ComputeDistanceThreshold2(
      latitude, longitude, this->ClusterDistance);
    for (; level >= 0; level--)
      {
      ClusteringNode *closest =
        this->FindClosestNode(node, level, threshold2);
      if (closest)
        {
        // Todo Update closest node with marker info
        vtkDebugMacro("Found closest node to " << node->NodeId
                      << " at " << closest->NodeId);
        double denominator = 1.0 + closest->NumberOfMarkers;
        for (unsigned i=0; i<2; i++)
          {
          double numerator = closest->gcsCoords[i]*closest->NumberOfMarkers +
            node->gcsCoords[i];
          closest->gcsCoords[i] = numerator/denominator;
          }
        closest->NumberOfMarkers++;
        closest->NumberOfVisibleMarkers++;
        closest->MarkerId = -1;
        closest->Children.insert(node);
        node->Parent = closest;

        // Insertion step ends with first clustering
        node = closest;
        break;
        }
      else
        {
        // Copy node and add to this level
        ClusteringNode *newNode = new ClusteringNode;
        this->Internals->AllNodes.push_back(newNode);
        newNode->NodeId = this->Internals->NumberOfNodes++;
        newNode->Level = level;
        newNode->gcsCoords[0] = node->gcsCoords[0];
        newNode->gcsCoords[1] = node->gcsCoords[1];
        newNode->gcsCoords[2] = node->gcsCoords[2];
        newNode->NumberOfMarkers = node->NumberOfMarkers;
        newNode->NumberOfVisibleMarkers = node->NumberOfVisibleMarkers;
        newNode->NumberOfSelectedMarkers = node->NumberOfSelectedMarkers;
        newNode->MarkerId = node->MarkerId;
        newNode->Parent = NULL;
        newNode->Children.insert(node);
        this->Internals->NodeTable[level].insert(newNode);
        vtkDebugMacro("Level " << level << " add node " << node->NodeId
                      << " --> " << newNode->NodeId);

        node->Parent = newNode;
        node = newNode;
        }
      }

    // Advance to next level up
    node = node->Parent;
    level--;

    // Refinement step: Continue iterating up while
    // * Merge any nodes identified in previous iteration
    // * Update node coordinates
    // * Check for closest node
    std::set<ClusteringNode*> nodesToMerge;
    std::set<ClusteringNode*> parentsToMerge;
    while (level >= 0)
      {
      // Merge nodes identified in previous iteration
      std::set<ClusteringNode*>::iterator mergingNodeIter =
        nodesToMerge.begin();
      for (; mergingNodeIter != nodesToMerge.end(); mergingNodeIter++)
        {
        ClusteringNode *mergingNode = *mergingNodeIter;
        if (node == mergingNode)
          {
          vtkWarningMacro("Node & merging node the same " << node->NodeId);
          }
        else
          {
          vtkDebugMacro("At level " << level
                        << "Merging node " << mergingNode
                        << " into " << node);
          this->MergeNodes(node, mergingNode, parentsToMerge, level);
          }
        }

      // Update coordinates?

      // Update count
      int numMarkers = 0;
      int numSelectedMarkers = 0;
      int numVisibleMarkers = 0;
      double numerator[2];
      numerator[0] = numerator[1] = 0.0;
      std::set<ClusteringNode*>::iterator childIter = node->Children.begin();
      for (; childIter != node->Children.end(); childIter++)
        {
        ClusteringNode *child = *childIter;
        numMarkers += child->NumberOfMarkers;
        numSelectedMarkers += child->NumberOfSelectedMarkers;
        numVisibleMarkers += child->NumberOfVisibleMarkers;
        for (int i=0; i<2; i++)
          {
          numerator[i] += child->NumberOfMarkers * child->gcsCoords[i];
          }
        }
      node->NumberOfMarkers = numMarkers;
      node->NumberOfSelectedMarkers = numSelectedMarkers;
      node->NumberOfVisibleMarkers = numVisibleMarkers;
      if (numMarkers > 1)
        {
        node->MarkerId = -1;
        }
      node->gcsCoords[0] = numerator[0] / numMarkers;
      node->gcsCoords[1] = numerator[1] / numMarkers;

      // Check for new clustering partner
      ClusteringNode *closest =
        this->FindClosestNode(node, level, threshold2);
      if (closest)
        {
        this->MergeNodes(node, closest, parentsToMerge, level);
        }

      // Setup for next iteration
      nodesToMerge.clear();
      nodesToMerge = parentsToMerge;
      parentsToMerge.clear();
      node = node->Parent;
      level--;
      }
    }

  this->Internals->MarkersChanged = true;

  if (false)
    {
    // Dump all nodes
    for (int i=0; i<this->Internals->AllNodes.size(); i++)
      {
      ClusteringNode *currentNode = this->Internals->AllNodes[i];
      std::cout << "Node " << i << " has ";
      if (currentNode)
        {
      std::cout << currentNode->Children.size() << " children, "
                << currentNode->NumberOfMarkers << " markers, and "
                << " marker id " << currentNode->MarkerId;
        }
      else
        {
        std::cout << " been deleted";
        }
      std::cout << "\n";
      }
    std::cout << std::endl;
    }

  return markerId;
}

//----------------------------------------------------------------------------
bool vtkMapMarkerSet::DeleteMarker(vtkIdType markerId)
{
  ClusteringNode *markerNode = this->Internals->MarkerNodes[markerId];

  // Check if marker has already been removed
  if (!markerNode)
    {
    return true;
    }

  // Recursively update ancestors (ClusteringNode instances)
  int deltaVisible = this->Internals->MarkerVisible[markerId] ? 1 : 0;
  int deltaSelected = this->Internals->MarkerSelected[markerId] ? 1 : 0;
  ClusteringNode *node = markerNode;
  ClusteringNode *parent = node->Parent;
  while (parent)
    {
    // Erase node if it is empty
    if (node->NumberOfMarkers < 1)
      {
      vtkDebugMacro("Deleting node " << node->NodeId << " level " << node->Level);
      parent->Children.erase(node);
      int level = node->Level;
      std::set<ClusteringNode*> nodeSet = this->Internals->NodeTable[level];
      nodeSet.erase(node);
      this->Internals->NodeTable[level] = nodeSet;
      delete node;
      }

    if (parent->NumberOfMarkers > 1)
      {
      // Update gcsCoords
      double denom = static_cast<double>(parent->NumberOfMarkers - 1);
      for (int i=0; i<3; ++i)
        {
        double num = (parent->NumberOfMarkers * parent->gcsCoords[i]) -
          markerNode->gcsCoords[i];
        parent->gcsCoords[i] = num / denom;
        }
      }

    parent->NumberOfMarkers -= 1;
    if (parent->NumberOfMarkers == 1)
      {
      // Get MarkerId from remaining node
      std::set<ClusteringNode*>::iterator iter = parent->Children.begin();
      ClusteringNode *extantNode = *iter;
      parent->MarkerId = extantNode->MarkerId;
      }
    parent->NumberOfVisibleMarkers -= deltaVisible;
    parent->NumberOfSelectedMarkers -= deltaSelected;

    // Setup next iteration
    node = parent;
    parent = parent->Parent;
    }

  // Update Internals and delete marker itself
  this->Internals->NumberOfMarkers -= 1;
  this->Internals->AllNodes[markerId] = 0;
  this->Internals->MarkerNodes[markerId] = 0;

  vtkDebugMacro("Deleting marker " << markerNode->NodeId);
  delete markerNode;

  this->Internals->MarkersChanged = true;
  return true;
}

//----------------------------------------------------------------------------
bool vtkMapMarkerSet::SetMarkerVisibility(int markerId, bool visible)
{
  // std::cout << "Set marker id " << markerId
  //           << " to visible: " << visible << std::endl;
  if ((markerId < 0) || (markerId > this->Internals->MarkerNodes.size()))
    {
    vtkWarningMacro("Invalid Marker Id: " << markerId);
    return false;
    }

  bool isModified = visible != this->Internals->MarkerVisible[markerId];
  if (!isModified)
    {
    return false;
    }

  // Check that node wasn't deleted
  ClusteringNode *node = this->Internals->MarkerNodes[markerId];
  if (!node)
    {
    std::cerr << "WARNING: Marker " << markerId << " was deleted" << std::endl;
    return false;
    }

  // Update marker's node
  node->NumberOfVisibleMarkers = visible ? 1 : 0;
  // Recursively update ancestor ClusteringNode instances
  int delta = visible ? 1 : -1;
  ClusteringNode *parent = node->Parent;
  while (parent)
    {
    parent->NumberOfVisibleMarkers += delta;
    parent = parent->Parent;
    }

  this->Internals->MarkerVisible[markerId] = visible;
  this->Internals->MarkersChanged = true;
  // Do we need to set Modified?
  // Better still - rebuild visibility (mask) array for current display
  // Better still - incrementally update visibility array
  return true;
}

//----------------------------------------------------------------------------
bool vtkMapMarkerSet::SetMarkerSelection(int markerId, bool selected)
{
  // std::cout << "Set marker id " << markerId
  //           << " to selected: " << selected << std::endl;
  if ((markerId < 0) || (markerId > this->Internals->MarkerNodes.size()))
    {
    vtkWarningMacro("Invalid Marker Id: " << markerId);
    return false;
    }

  bool isModified = selected != this->Internals->MarkerSelected[markerId];
  if (!isModified)
    {
    return false;
    }

  ClusteringNode *node = this->Internals->MarkerNodes[markerId];
  if (!node)
    {
    std::cerr << "WARNING: Marker " << markerId << " was deleted" << std::endl;
    return false;
    }

  node->NumberOfSelectedMarkers = selected ? 1 : 0;

  // Recursively update ancestor ClusteringNoe instances
  int delta = selected ? 1 : -1;
  ClusteringNode *parent = node->Parent;
  while (parent)
    {
    parent->NumberOfSelectedMarkers += delta;
    parent = parent->Parent;
    }

  this->Internals->MarkerSelected[markerId] = selected;
  this->Internals->MarkersChanged = true;
  return true;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::
GetClusterChildren(vtkIdType clusterId, vtkIdList *childMarkerIds,
                   vtkIdList *childClusterIds)
{
  childMarkerIds->Reset();
  childClusterIds->Reset();
  if ((clusterId < 0) || (clusterId >= this->Internals->AllNodes.size()))
    {
    return;
    }

  // Check if node has been deleted
  ClusteringNode *node = this->Internals->AllNodes[clusterId];
  if (!node)
    {
    return;
    }

  std::set<ClusteringNode*>::iterator childIter = node->Children.begin();
  for (; childIter != node->Children.end(); childIter++)
    {
    ClusteringNode *child = *childIter;
    if (child->NumberOfMarkers == 1)
      {
      childMarkerIds->InsertNextId(child->MarkerId);
      }
    else
      {
      childClusterIds->InsertNextId(child->NodeId);
      }  // else
    }  // for (childIter)
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::
GetAllMarkerIds(vtkIdType clusterId, vtkIdList *markerIds)
{
  markerIds->Reset();
  // Check if input id is marker
  ClusteringNode *node = this->Internals->AllNodes[clusterId];
  if (!node)
    {
    return;
    }

  if (node->NumberOfMarkers == 1)
    {
    markerIds->InsertNextId(clusterId);
    return;
    }

  this->GetMarkerIdsRecursive(clusterId, markerIds);
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::
GetMarkerIdsRecursive(vtkIdType clusterId, vtkIdList *markerIds)
{
  // Get children markers & clusters
  vtkNew<vtkIdList> childMarkerIds;
  vtkNew<vtkIdList> childClusterIds;
  this->GetClusterChildren(clusterId, childMarkerIds.GetPointer(),
                           childClusterIds.GetPointer());

  // Copy marker ids
  for (int i=0; i<childMarkerIds->GetNumberOfIds(); i++)
    {
    markerIds->InsertNextId(childMarkerIds->GetId(i));
    }

  // Traverse cluster ids
  for (int j=0; j<childClusterIds->GetNumberOfIds(); j++)
    {
    vtkIdType childId = childClusterIds->GetId(j);
    this->GetMarkerIdsRecursive(childId, markerIds);
    }
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::Init()
{
  // Set up rendering pipeline

  // Add "Visible" array to polydata - to mask display
  const char *maskName = "Visible";
  vtkNew<vtkBitArray> visibles;
  visibles->SetName(maskName);
  visibles->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(visibles.GetPointer());

  // Add "Selected" array to polydata - for seleted state
  const char *selectName = "Selected";
  vtkNew<vtkBitArray> selects;
  selects->SetName(selectName);
  selects->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(selects.GetPointer());

  // Add "MarkerType" array to polydata - to select glyph
  const char *typeName = "MarkerType";
  vtkNew<vtkUnsignedCharArray> types;
  types->SetName(typeName);
  types->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(types.GetPointer());

  // Add "MarkerScale" to scale cluster glyph size
  const char *scaleName = "MarkerScale";
  vtkNew<vtkDoubleArray> scales;
  scales->SetName(scaleName);
  scales->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(scales.GetPointer());

  // Use DistanceToCamera filter to scale markers to constant screen size
  vtkNew<vtkDistanceToCamera> dFilter;
  dFilter->SetScreenSize(50.0);
  dFilter->SetRenderer(this->Layer->GetRenderer());
  dFilter->SetInputData(this->PolyData);
  if (this->Clustering)
    {
    dFilter->ScalingOn();
    dFilter->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "MarkerScale");
    }

  // Use teardrop shape for individual markers
  vtkNew<vtkTeardropSource> markerGlyphSource;
  markerGlyphSource->SetResolution(6);
  markerGlyphSource->FrontSideOnlyOn();
  markerGlyphSource->ProjectToXYPlaneOn();
  // Rotate to point downward (parallel to y axis)
  vtkNew<vtkTransformFilter> rotateMarker;
  rotateMarker->SetInputConnection(markerGlyphSource->GetOutputPort());
  vtkNew<vtkTransform> transform;
  transform->RotateZ(90.0);
  rotateMarker->SetTransform(transform.GetPointer());

  // Use sphere for cluster markers
  vtkNew<vtkSphereSource> clusterGlyphSource;
  clusterGlyphSource->SetPhiResolution(20);
  clusterGlyphSource->SetThetaResolution(20);
  clusterGlyphSource->SetRadius(0.25);

  // Switch in our mapper, and do NOT call Superclass::Init()
  this->GetActor()->SetMapper(this->Internals->GlyphMapper);
  this->Layer->GetRenderer()->AddActor(this->Actor);

  // Set up glyph mapper inputs
  this->Internals->GlyphMapper->SetSourceConnection(0, rotateMarker->GetOutputPort());
  this->Internals->GlyphMapper->SetSourceConnection(1, clusterGlyphSource->GetOutputPort());
  this->Internals->GlyphMapper->SetInputConnection(dFilter->GetOutputPort());

  // Select glyph type by "MarkerType" array
  this->Internals->GlyphMapper->SourceIndexingOn();
  this->Internals->GlyphMapper->SetSourceIndexArray(typeName);

  // Set scale by "DistanceToCamera" array
  this->Internals->GlyphMapper->ScalingOn();
  this->Internals->GlyphMapper->SetScaleFactor(1.0);
  this->Internals->GlyphMapper->SetScaleModeToScaleByMagnitude();
  this->Internals->GlyphMapper->SetScaleArray("DistanceToCamera");

  // Set visibility by "Visible" array
  this->Internals->GlyphMapper->MaskingOn();
  this->Internals->GlyphMapper->SetMaskArray(maskName);

  // Set color by "Selected" array
  this->Internals->GlyphMapper->SetColorModeToMapScalars();
  this->PolyData->GetPointData()->SetActiveScalars(selectName);

  this->Internals->GlyphMapper->Update();
  this->Initialized = true;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::Update()
{
  if (!this->Initialized)
    {
    vtkErrorMacro("vtkMapMarkerSet has NOT been initialized");
    }

  // Clip zoom level to size of cluster table
  int zoomLevel = this->Layer->GetMap()->GetZoom();
  if (zoomLevel >= NumberOfClusterLevels)
    {
    zoomLevel = NumberOfClusterLevels - 1;
    }

  // If not clustering, only update if markers have changed
  if (!this->Clustering && !this->Internals->MarkersChanged)
    {
    return;
    }

  // If clustering, only update if either zoom or markers changed
  if (this->Clustering && !this->Internals->MarkersChanged &&
      (zoomLevel == this->Internals->ZoomLevel))
    {
    return;
    }

  // In non-clustering mode, markers stored at level 0
  if (!this->Clustering)
    {
    zoomLevel = this->Internals->NodeTable.size() - 1;
    }

  // Copy marker info into polydata
  vtkNew<vtkPoints> points;

  // Get pointers to data arrays
  vtkDataArray *array;
  array = this->PolyData->GetPointData()->GetArray("Visible");
  vtkBitArray *visibles = vtkBitArray::SafeDownCast(array);
  visibles->Reset();
  array = this->PolyData->GetPointData()->GetArray("Selected");
  vtkBitArray *selects = vtkBitArray::SafeDownCast(array);
  selects->Reset();
  array = this->PolyData->GetPointData()->GetArray("MarkerType");
  vtkUnsignedCharArray *types = vtkUnsignedCharArray::SafeDownCast(array);
  types->Reset();
  array = this->PolyData->GetPointData()->GetArray("MarkerScale");
  vtkDoubleArray *scales = vtkDoubleArray::SafeDownCast(array);
  scales->Reset();

  // Coefficients for scaling cluster size, using simple 2nd order model
  // The equation is y = k*x^2 / (x^2 + b), where k,b are coefficients
  // Logic hard-codes the min cluster factor to 1, i.e., y(2) = 1.0
  // Max value is k, which sets the horizontal asymptote.
  double k = this->MaxClusterScaleFactor;
  double b = 4.0*k - 4.0;

  this->Internals->CurrentNodes.clear();
  std::set<ClusteringNode*> nodeSet = this->Internals->NodeTable[zoomLevel];
  std::set<ClusteringNode*>::const_iterator iter;
  for (iter = nodeSet.begin(); iter != nodeSet.end(); iter++)
    {
    ClusteringNode *node = *iter;
    if (!node->NumberOfVisibleMarkers)
      {
      continue;
      }

    double z = node->gcsCoords[2] +
      (node->NumberOfSelectedMarkers ? this->SelectedZOffset : 0.0);
    points->InsertNextPoint(node->gcsCoords[0], node->gcsCoords[1], z);
    this->Internals->CurrentNodes.push_back(node);
    if (node->NumberOfMarkers == 1)  // point marker
      {
      types->InsertNextValue(MARKER_TYPE);
      scales->InsertNextValue(1.0);
      }
    else  // cluster marker
      {
      types->InsertNextValue(CLUSTER_TYPE);
      double x = static_cast<double>(node->NumberOfMarkers);
      double scale = k*x*x / (x*x + b);
      scales->InsertNextValue(scale);
      }

    // Set visibility
    bool isVisible = node->NumberOfVisibleMarkers > 0;
    visibles->InsertNextValue(isVisible);

    // Set color
    bool isSelected = node->NumberOfSelectedMarkers > 0;
    selects->InsertNextValue(isSelected);
    }
  this->PolyData->Reset();
  this->PolyData->SetPoints(points.GetPointer());

  this->Internals->MarkersChanged = false;
  this->Internals->ZoomLevel = zoomLevel;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::Cleanup()
{
  // Explicitly delete node instances in the table
  std::vector<std::set<ClusteringNode*> >::iterator tableIter =
    this->Internals->NodeTable.begin();
  for (; tableIter != this->Internals->NodeTable.end(); tableIter++)
    {
    std::set<ClusteringNode*> nodeSet = *tableIter;
    std::set<ClusteringNode*>::iterator nodeIter = nodeSet.begin();
    for (; nodeIter != nodeSet.end(); nodeIter++)
      {
      delete *nodeIter;
      }
    nodeSet.clear();
    tableIter->operator=(nodeSet);
    }

  this->Internals->CurrentNodes.clear();
  this->Internals->NumberOfMarkers = 0;
  this->Internals->NumberOfNodes = 0;
  this->Internals->MarkersChanged = true;
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::GetClusterId(vtkIdType displayId)
{
  // Check input validity
  if ((displayId < 0) || (displayId >= this->Internals->CurrentNodes.size()))
    {
    return -1;
    }

  ClusteringNode *node = this->Internals->CurrentNodes[displayId];
  return node->NodeId;
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::GetMarkerId(vtkIdType displayId)
{
  // Check input validity
  if ((displayId < 0) || (displayId >= this->Internals->CurrentNodes.size()))
    {
    return -1;
    }

  ClusteringNode *node = this->Internals->CurrentNodes[displayId];
  if (node->NumberOfMarkers == 1)
    {
    return node->MarkerId;
    }

  // else
  return -1;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet:: PrintClusterPath(ostream &os, int markerId)
{
  // Gather up nodes in a list (bottom to top)
  std::vector<ClusteringNode*> nodeList;
  ClusteringNode *markerNode = this->Internals->MarkerNodes[markerId];
  if (!markerNode)
    {
    std::cerr << "WARNING: Marker " << markerId << " was deleted" << std::endl;
    return;
    }

  nodeList.push_back(markerNode);
  ClusteringNode *parent = markerNode->Parent;
  while (parent)
    {
    nodeList.push_back(parent);
    parent = parent->Parent;
    }

  // Write the list top to bottom (reverse order)
  os << "Level, NodeId, MarkerId, NumberOfVisibleMarkers" << '\n';
  std::vector<ClusteringNode*>::reverse_iterator iter = nodeList.rbegin();
  for (; iter != nodeList.rend(); ++iter)
    {
    ClusteringNode *node = *iter;
    os << std::setw(2) << node->Level
       << "  " << std::setw(5) << node->NodeId
       << "  " << std::setw(5) << node->MarkerId
       << "  " << std::setw(4) << node->NumberOfVisibleMarkers
       << '\n';
    }

}

//----------------------------------------------------------------------------
double vtkMapMarkerSet::
ComputeDistanceThreshold2(double latitude, double longitude,
                          int clusteringDistance) const
{
  if (!this->Layer->GetMap()->GetPerspectiveProjection())
    {
    // For orthographic projection, the scaling is trivial.
    // At level 0, world coordinates range is 360.0
    // At level 0, map tile is 256 pixels.
    // Convert clusteringDistance to that scale:
    double scale = 360.0 * clusteringDistance / 256.0;
    return scale * scale;
    }


  // Get display coordinates for input point
  double inputLatLonCoords[2] = {latitude, longitude};
  double inputDisplayCoords[3];
  this->Layer->GetMap()->ComputeDisplayCoords(
    inputLatLonCoords, 0.0, inputDisplayCoords);

  // Offset the display coords by clustering distance
  double delta = clusteringDistance * SQRT_TWO / 2.0;
  double secondDisplayCoords[2] =
    {
      inputDisplayCoords[0] + delta,
      inputDisplayCoords[1] + delta
    };

  // Convert 2nd display coords to lat-lon
  double secondLatLonCoords[3] = {0.0, 0.0, 0.0};
  this->Layer->GetMap()->ComputeLatLngCoords(
    secondDisplayCoords, 0.0, secondLatLonCoords);

  // Convert both points to world coords
  double inputWorldCoords[3] =
    {
      inputLatLonCoords[1],
      vtkMercator::lat2y(inputLatLonCoords[0]),
      0.0
    };
  double secondWorldCoords[3] =
    {
      secondLatLonCoords[1],
      vtkMercator::lat2y(secondLatLonCoords[0]),
      0.0
    };

  // Return value is the distance squared
  double threshold2 = vtkMath::Distance2BetweenPoints(
    inputWorldCoords, secondWorldCoords);

  // Need to adjust by current zoom level
  int zoomLevel = this->Layer->GetMap()->GetZoom();
  double scale = static_cast<double>(1<<zoomLevel);
  threshold2 *= (scale * scale);

  return threshold2;
}

//----------------------------------------------------------------------------
vtkMapMarkerSet::ClusteringNode*
vtkMapMarkerSet::
FindClosestNode(ClusteringNode *node, int zoomLevel, double distanceThreshold2)
{
  // Convert distanceThreshold from image to gcs coords
  // double level0Scale = 360.0 / 256.0;  // 360 degress <==> 256 tile pixels
  // double scale = level0Scale / static_cast<double>(1<<zoomLevel);
  // double gcsThreshold = scale * distanceThreshold;
  // double gcsThreshold2 = gcsThreshold * gcsThreshold;
  double scale = static_cast<double>(1<<zoomLevel);
  double gcsThreshold2 = distanceThreshold2 / scale / scale;

  ClusteringNode *closestNode = NULL;
  double closestDistance2 = gcsThreshold2;
  std::set<ClusteringNode*> nodeSet = this->Internals->NodeTable[zoomLevel];
  std::set<ClusteringNode*>::const_iterator setIter = nodeSet.begin();
  for (; setIter != nodeSet.end(); setIter++)
    {
    ClusteringNode *other = *setIter;
    if (other == node)
      {
      continue;
      }

    double d2 = 0.0;
    for (int i=0; i<2; i++)
      {
      double d1 = other->gcsCoords[i] - node->gcsCoords[i];
      d2 += d1 * d1;
      }
    if (d2 < closestDistance2)
      {
      closestNode = other;
      closestDistance2 = d2;
      }
    }

  return closestNode;
}

//----------------------------------------------------------------------------
void
vtkMapMarkerSet::
MergeNodes(ClusteringNode *node, ClusteringNode *mergingNode,
           std::set<ClusteringNode*>& parentsToMerge, int level)
{
  vtkDebugMacro("Merging " << mergingNode->NodeId
                << " into " << node->NodeId);
  if (node->Level != mergingNode->Level)
    {
    vtkErrorMacro("Node " << node->NodeId
                  << "and node " << mergingNode->NodeId
                  << "not at the same level");
    }

  // Update gcsCoords
  int numMarkers = node->NumberOfMarkers + mergingNode->NumberOfMarkers;
  double denominator = static_cast<double>(numMarkers);
  for (unsigned i=0; i<2; i++)
    {
    double numerator = node->gcsCoords[i]*node->NumberOfMarkers +
      mergingNode->gcsCoords[i]*mergingNode->NumberOfMarkers;
    node->gcsCoords[i] = numerator/denominator;
    }
  node->NumberOfMarkers = numMarkers;
  node->NumberOfVisibleMarkers += mergingNode->NumberOfVisibleMarkers;
  node->MarkerId  = -1;

  // Update links to/from children of merging node
  // Make a working copy of the child set
  std::set<ClusteringNode*> childNodeSet(mergingNode->Children);
  std::set<ClusteringNode*>::iterator childNodeIter =
    childNodeSet.begin();
  for (; childNodeIter != childNodeSet.end(); childNodeIter++)
    {
    ClusteringNode *childNode = *childNodeIter;
    node->Children.insert(childNode);
    childNode->Parent = node;
    }

  // Adjust parent marker counts
  // Todo recompute from children
  int n = mergingNode->NumberOfMarkers;
  node->Parent->NumberOfMarkers += n;
  mergingNode->Parent->NumberOfMarkers -= n;

  // Remove mergingNode from its parent
  ClusteringNode *parent =  mergingNode->Parent;
  parent->Children.erase(mergingNode);

  // Remember parent node if different than node's parent
  if (mergingNode->Parent && mergingNode->Parent != node->Parent)
    {
    parentsToMerge.insert(mergingNode->Parent);
    }

  // Delete mergingNode
  // todo only delete if valid level specified?
  int count = this->Internals->NodeTable[level].count(mergingNode);
  if (count == 1)
    {
    this->Internals->NodeTable[level].erase(mergingNode);
    }
  else
    {
    vtkErrorMacro("Node " << mergingNode->NodeId
                  << " not found at level " << level);
    }
  // todo Check CurrentNodes too?
  this->Internals->AllNodes[mergingNode->NodeId] = NULL;
  delete mergingNode;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::ComputeNextColor(double color[3])
{
  // Generate "next" hue using logic adapted from
  // http://ridiculousfish.com/blog/posts/colors.html

  unsigned int bitCount = 6;  // 2^6 == 64 colors
  double hue;

  // Use loop that includes checking for overlap with selection hue
  int i = 0;
  for (i=0; i<2; i++)
    {
    // Reverse the bits in vtkMapMarkerSet::NextMarkerHue
    unsigned int forwardBits = vtkMapMarkerSet::NextMarkerHue;
    unsigned int reverseBits = 0;
    for (int i=0; i<bitCount; i++)
      {
      reverseBits = (reverseBits << 1) | (forwardBits & 1);
      forwardBits >>= 1;
      }

    // Divide by max
    unsigned int maxCount = 1 << bitCount;

    hue = static_cast<double>(reverseBits) / maxCount;
    hue += 0.6;  // offset to start at blue
    hue = (hue > 1.0) ? hue - 1.0 : hue;
    vtkMapMarkerSet::NextMarkerHue += 1;

    // Check distance from SelectionHue
    if (fabs(hue - this->SelectionHue) > 1.0/30.0)
      {
      break;
      }

    vtkDebugMacro("ComputeNextColor skipping hue " << hue);
    }  // for (i)

  if (i > 1)
    {
    vtkWarningMacro("ComputeNextColor default after 2 iterations");
    }

  vtkDebugMacro("ComputeNextColor hue: " << hue);
  double hsv[3] = {hue, 1.0, 1.0};
  vtkMath::HSVToRGB(hsv, color);
}

#undef SQRT_TWO
#undef MARKER_TYPE
#undef CLUSTER_TYPE
