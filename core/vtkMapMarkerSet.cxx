/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// vtkMap Includes
#include "vtkMapMarkerSet.h"
#include "assets/hexagon.h"
#include "assets/octagon.h"
#include "assets/pentagon.h"
#include "assets/square.h"
#include "assets/teardrop.h"
#include "assets/triangle.h"
#include "markersShadowImageData.h"
#include "vtkMapPointSelection.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMercator.h"

// VTK Includes
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkBitArray.h>
#include <vtkDataArray.h>
#include <vtkDistanceToCamera.h>
#include <vtkDoubleArray.h>
#include <vtkGlyph3DMapper.h>
#include <vtkIdList.h>
#include <vtkImageData.h>
#include <vtkLabeledDataMapper.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlaneSource.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderer.h>
#include <vtkTextProperty.h>
#include <vtkTexture.h>
#include <vtkTextureMapToPlane.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>

// Std/STL libs includes
#include <algorithm>
#include <functional>
#include <cmath>
#include <iostream>
#include <iterator> // std::back_inserter
#include <vector>
#include <unordered_map>

// Static field initialization
unsigned int vtkMapMarkerSet::NextMarkerHue = 0;

//----------------------------------------------------------------------------
namespace
{
enum
{
  MARKER_TYPE  = 0,
  CLUSTER_TYPE = 1
};

const auto SQRT_TWO = std::sqrt(2.0);

const char* GetMarkerGeometry(vtkMapType::Shape const shape)
{
  const char* array;
  switch (shape)
  {
    case vtkMapType::Shape::TRIANGLE:
      array = triangle;
      break;
    case vtkMapType::Shape::SQUARE:
      array = square;
      break;
    case vtkMapType::Shape::PENTAGON:
      array = pentagon;
      break;
    case vtkMapType::Shape::HEXAGON:
      array = hexagon;
      break;
    case vtkMapType::Shape::OCTAGON:
      array = octagon;
      break;
    case vtkMapType::Shape::TEARDROP:
      array = teardrop;
      break;
  }
  return array;
};

// Hard-code color palette to match leaflet-awesome markers
// The names are just a guess
unsigned char palette[][3] = {
  { 214, 62, 42 },  // red
  { 246, 151, 48 }, // orange
  { 114, 176, 38 }, // green
  { 56, 170, 221 }, // light blue
  //{210,  82, 185} // violet -- omit: too close to selection color (255,0,255)
  { 162, 51, 54 },   // dark red
  { 0, 103, 163 },   // dark blue
  { 114, 130, 36 },  // olive green
  { 91, 57, 107 },   // purple
  { 67, 105, 120 },  // teal
  { 255, 142, 127 }, // salmon
  { 255, 203, 146 }, // tan
  { 187, 249, 112 }, // light green
  { 138, 218, 255 }, // light blue
  { 255, 145, 234 }, // pink
  { 235, 125, 127 }, // slightly darker salmon
};

std::size_t paletteSize = sizeof(palette) / sizeof(unsigned char[3]);
std::size_t paletteIndex = 0;
} // namespace

//----------------------------------------------------------------------------
// Internal class for cluster tree nodes
// Each node represents either one marker or a cluster of nodes
class vtkMapMarkerSet::ClusteringNode
{
public:
  vtkIdType NodeId;
  vtkIdType Level; // for dev
  double gcsCoords[3];
  ClusteringNode* Parent;
  std::set<ClusteringNode*> Children;
  vtkIdType NumberOfMarkers; // 1 for single-point nodes, >1 for clusters
  vtkIdType MarkerId;        // only relevant for single-point markers (not clusters)
  vtkIdType NumberOfVisibleMarkers;
  vtkIdType NumberOfSelectedMarkers;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMapMarkerSet)

  //----------------------------------------------------------------------------
  class vtkMapMarkerSet::MapMarkerSetInternals
{
public:
  vtkGlyph3DMapper* GlyphMapper;

  vtkIdType ZoomLevel; // Used for marker clustering

  vtkIdType UniqueMarkerId;
  vtkIdType UniqueNodeId;

  // index: displayId (marker in the map), used to handle selections
  std::vector<ClusteringNode*> CurrentNodes; // in this->PolyData

  // index: zoom level
  std::vector<std::set<ClusteringNode*>> NodeTable; // Used for marker clustering

  // for single-markers only (not clusters)
  // key: markerId
  std::unordered_map<vtkIdType, bool> MarkerVisibleMap;
  std::unordered_map<vtkIdType, bool> MarkerSelectedMap;

  // key: nodeID, the nodes already exist in NodeTable !
  std::unordered_map<vtkIdType, ClusteringNode*> AllNodesMap; // for dev

  // key: markerID, the markers already exist in NodeTable's last set.
  // In fact, NodeTable last set, corresponding to the deepest zoom level
  // contains only markers !
  std::unordered_map<vtkIdType, ClusteringNode*> MarkerNodesMap;

  // Second mapper and actor for shadow image/texture
  vtkImageData* ShadowImage;
  vtkTexture* ShadowTexture;
  vtkActor* ShadowActor;
  vtkGlyph3DMapper* ShadowMapper;

  vtkSmartPointer<vtkActor2D> LabelActor;
  /**
   * Handles point-label rendering. An alternative would be to use one of its
   * derived classes (e.g. vtkDynamic2DLabelMapper), which handle label
   * overlapping.
   */
  vtkSmartPointer<vtkLabeledDataMapper> LabelMapper;
  vtkSmartPointer<vtkMapPointSelection> LabelSelector;

  vtkTimeStamp ShapeInitTime;
};

//----------------------------------------------------------------------------
vtkMapMarkerSet::vtkMapMarkerSet()
  : vtkPolydataFeature()
{
  this->Initialized = false;
  this->EnablePointMarkerShadow = true;
  this->ZCoord = 0.1;
  this->SelectedZOffset = 0.0;
  this->PolyData = vtkPolyData::New();
  this->Clustering = false;
  this->ClusteringTreeDepth = 14;
  this->ClusterDistance = 40;
  this->MaxClusterScaleFactor = 2.0;

  // Initialize color table
  this->ColorTable = vtkLookupTable::New();
  this->ColorTable->SetNumberOfTableValues(2);
  this->ColorTable->Build();

  // Selected color
  this->SelectionHue = 5.0 / 6.0; // magenta (fushia?)
  double hsv[3] = { this->SelectionHue, 1.0, 1.0 };
  double rgb[3] = { 0.0, 0.0, 0.0 };
  vtkMath::HSVToRGB(hsv, rgb);
  double selected[4] = { rgb[0], rgb[1], rgb[2], 1.0 };
  this->ColorTable->SetTableValue(1, selected);

  // Default color
  double color[4] = { 0.0, 0.0, 0.0, 1.0 };
  this->ComputeNextColor(color);
  this->ColorTable->SetTableValue(0, color);

  // Set Nan color for development & test
  double red[] = { 1.0, 0.0, 0.0, 1.0 };
  this->ColorTable->SetNanColor(red);

  this->Internals = new MapMarkerSetInternals;
  this->Internals->ZoomLevel = -1;
  this->Internals->NodeTable.resize(this->ClusteringTreeDepth);
  this->Internals->UniqueMarkerId = 0;
  this->Internals->UniqueNodeId = 0;
  this->Internals->GlyphMapper = vtkGlyph3DMapper::New();
  this->Internals->GlyphMapper->SetLookupTable(this->ColorTable);

  this->Internals->LabelActor = vtkSmartPointer<vtkActor2D>::New();
  this->Internals->LabelMapper = vtkSmartPointer<vtkLabeledDataMapper>::New();
  this->Internals->LabelSelector = vtkSmartPointer<vtkMapPointSelection>::New();

  // Initialize shadow for point map markers
  this->Internals->ShadowImage = vtkImageData::New();
  // For now, you just gotta know that the image size
  const int dims[] = { 35, 16, 1 };
  this->Internals->ShadowImage->SetDimensions(dims);
  this->Internals->ShadowImage->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  unsigned char* ptr = static_cast<unsigned char*>(
    this->Internals->ShadowImage->GetScalarPointer());
  for (unsigned int i = 0; i < dims[0] * dims[1] * 4; ++i)
  {
    *ptr++ = markersShadowImageData[i];
  }

  // Initialize the texture plane (geometry)
  /*
    Setting dimensions requires knowing the marker and shadow dimensions, in
    https://github.com/lvoogdt/Leaflet.awesome-markers/blob/2.0/develop/dist/leaflet.awesome-markers.css
    Marker image width = 35, height = 46
    Shadow image width = 36, height = 16

    And since marker's PolyData is scaled to height of 1.0, set shadow geometry as
    follows:
   */
  double imageHeight = 1.0;
  double shadowHeight = imageHeight * (16.0 / 46.0); // 0.348
  double imageWidth = imageHeight * (35.0 / 46.0);
  double shadowWidth = imageWidth * (36.0 / 35.0); // 0.783

  vtkNew<vtkPlaneSource> plane;
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetPoint1(shadowWidth, 0.0, 0.0);
  plane->SetPoint2(0.0, shadowHeight, 0.0);
  plane->SetNormal(0.0, 0.0, 1.0);

  // Initialize texture plane (generates texture coords)
  // This is the glyph source
  vtkNew<vtkTextureMapToPlane> texturePlane;
  texturePlane->SetInputConnection(plane->GetOutputPort());

  // Initialize mapper and actor
  this->Internals->ShadowTexture = vtkTexture::New();
  this->Internals->ShadowTexture->SetInputData(this->Internals->ShadowImage);
  this->Internals->ShadowMapper = vtkGlyph3DMapper::New();
  this->Internals->ShadowMapper->SetSourceConnection(
    0, texturePlane->GetOutputPort());

  // Need an empty source for second source (to omit shadows from cluster markers)
  vtkNew<vtkPoints> nullPoints;
  vtkNew<vtkPolyData> nullSource;
  nullSource->SetPoints(nullPoints.GetPointer());
  this->Internals->ShadowMapper->SetSourceData(1, nullSource.GetPointer());

  this->Internals->ShadowActor = vtkActor::New();
  this->Internals->ShadowActor->PickableOff();
  this->Internals->ShadowActor->SetMapper(this->Internals->ShadowMapper);
  this->Internals->ShadowActor->SetTexture(this->Internals->ShadowTexture);

  //SetDebug(true);
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << this->GetClassName() << "\n"
     << indent << "Initialized: " << this->Initialized << "\n"
     << indent << "Clustering: " << this->Clustering << "\n"
     << indent << "Clustering distance: " << this->ClusterDistance << "\n"
     << indent << "NumberOfMarkers: " << this->Internals->MarkerNodesMap.size()
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
  this->Internals->ShadowActor->Delete();
  this->Internals->ShadowTexture->Delete();
  this->Internals->ShadowMapper->Delete();
  this->Internals->ShadowImage->Delete();

  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::SetColor(double rgba[4])
{
  this->ColorTable->SetTableValue(0, rgba);
}

//----------------------------------------------------------------------------
size_t vtkMapMarkerSet::GetNumberOfMarkers()
{
  return this->Internals->MarkerNodesMap.size();
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::AddMarker(double latitude, double longitude)
{
  if (!this->Initialized)
  {
    vtkErrorMacro("Is not initialized!");
    return -1;
  }

  // Set marker id
  const vtkIdType markerId = this->Internals->UniqueMarkerId++;

  vtkDebugMacro("Adding marker " << markerId);

  // if (markerId == 1)
  //   {
  //   vtkRenderer *renderer = this->Layer->GetMap()->GetRenderer();
  //   int *viewPortSize = renderer->GetSize();
  //   std::cout << "Using ViewPort " << viewPortSize[0]
  //             << ", " << viewPortSize[1] << std::endl;
  //   }

  // Insert nodes at bottom level
  const vtkIdType level = vtkIdType(this->Internals->NodeTable.size()) - 1;

  // Instantiate ClusteringNode
  ClusteringNode* const node = new ClusteringNode;

  // first node of this marker in the node's tree (zoom level 13)
  const vtkIdType nodeId = this->Internals->UniqueNodeId++;

  // check unicity of nodeId and markerId
  assert(this->Internals->AllNodesMap.find(nodeId) ==
         this->Internals->AllNodesMap.end());
  assert(this->Internals->MarkerVisibleMap.find(markerId) ==
         this->Internals->MarkerVisibleMap.end());
  assert(this->Internals->MarkerSelectedMap.find(markerId) ==
         this->Internals->MarkerSelectedMap.end());
  assert(this->Internals->MarkerNodesMap.find(markerId) ==
         this->Internals->MarkerNodesMap.end());

  this->Internals->AllNodesMap.emplace(nodeId, node);
  node->NodeId = nodeId;
  node->Level = level;
  node->gcsCoords[0] = longitude;
  node->gcsCoords[1] = vtkMercator::lat2y(latitude);
  node->gcsCoords[2] = this->ZCoord;
  node->NumberOfMarkers = 1;
  node->Parent = nullptr;
  node->MarkerId = markerId;
  node->NumberOfVisibleMarkers = 1;
  node->NumberOfSelectedMarkers = 0;
  vtkDebugMacro(
    "Inserting ClusteringNode " << node->NodeId << " into level " << level);
  this->Internals->NodeTable[size_t(level)].insert(node);
  this->Internals->MarkerVisibleMap.emplace(markerId, true);
  this->Internals->MarkerSelectedMap.emplace(markerId, false);
  this->Internals->MarkerNodesMap.emplace(markerId, node);

  // For now, always insert into cluster tree even if clustering disabled
  this->InsertIntoNodeTable(node);
  this->Modified();

  //DumpAllNodesMap();

  return markerId;
}

//----------------------------------------------------------------------------
bool vtkMapMarkerSet::DeleteMarker(const vtkIdType markerId)
{
  if (this->Internals->MarkerNodesMap.find(markerId) ==
      this->Internals->MarkerNodesMap.end())
  {
    vtkDebugMacro("DeleteMarker: Marker " << markerId << "doesn't exist !");
    return false;
  }

  ClusteringNode* const markerNode = this->Internals->MarkerNodesMap[markerId];

  assert(this->Internals->MarkerVisibleMap.find(markerId) !=
         this->Internals->MarkerVisibleMap.end());
  assert(this->Internals->MarkerSelectedMap.find(markerId) !=
         this->Internals->MarkerSelectedMap.end());

  // Recursively update ancestors (ClusteringNode instances)
  int deltaVisible = this->Internals->MarkerVisibleMap[markerId] ? 1 : 0;
  int deltaSelected = this->Internals->MarkerSelectedMap[markerId] ? 1 : 0;

  ClusteringNode* node = markerNode;
  ClusteringNode* parent = node->Parent;

  assert(node->NumberOfMarkers >= 1);

  // First of all, we remove the marker from its Parent's Children set
  if (parent)
  {
    parent->Children.erase(node);
  }

  while (parent)
  {
    // Erase node if it is empty
    if (node->NumberOfMarkers < 1)
    {
      vtkDebugMacro(
        "Deleting node " << node->NodeId << " level " << node->Level);
      parent->Children.erase(node);
      auto level = node->Level;

      this->Internals->NodeTable[size_t(level)].erase(node);

      assert(this->Internals->AllNodesMap.find(node->NodeId) !=
             this->Internals->AllNodesMap.end());

      this->Internals->AllNodesMap.erase(node->NodeId);

      delete node;
      node = nullptr;
    }

    if (parent->NumberOfMarkers > 1)
    {
      // Update gcsCoords
      double denom = static_cast<double>(parent->NumberOfMarkers - 1);
      for (int i = 0; i < 3; ++i)
      {
        double num = (parent->NumberOfMarkers * parent->gcsCoords[i]) -
          markerNode->gcsCoords[i];
        parent->gcsCoords[i] = num / denom;
      }
    }

    parent->NumberOfMarkers -= 1;

    if (parent->NumberOfMarkers == 1 && !parent->Children.empty())
    {
      // Get MarkerId from remaining node
      std::set<ClusteringNode*>::iterator iter = parent->Children.begin();
      ClusteringNode* const extantNode = *iter;
      parent->MarkerId = extantNode->MarkerId;
    }

    parent->NumberOfVisibleMarkers -= deltaVisible;
    parent->NumberOfSelectedMarkers -= deltaSelected;

    // Setup next iteration
    node = parent;
    parent = parent->Parent;
  }

  // delete last node (at level 0)
  if (node &&
      node->Parent == nullptr &&
      node->NumberOfMarkers == 0)
  {
    const size_t indexInNodeTable = size_t(node->Level);

    assert(this->Internals->NodeTable[indexInNodeTable].find(node) !=
           this->Internals->NodeTable[indexInNodeTable].end());

    if (indexInNodeTable < this->Internals->NodeTable.size())
    {
      this->Internals->NodeTable[indexInNodeTable].erase(node);
    }

    assert(this->Internals->AllNodesMap.find(node->NodeId) !=
           this->Internals->AllNodesMap.end());

    this->Internals->AllNodesMap.erase(node->NodeId);

    delete node;
    node = nullptr;
  }

  // Update Internals and delete marker itself
  this->Internals->AllNodesMap.erase(markerNode->NodeId);
  this->Internals->MarkerNodesMap.erase(markerId);
  this->Internals->MarkerVisibleMap.erase(markerId);
  this->Internals->MarkerSelectedMap.erase(markerId);
  // delete marker (at the end of the node table)
  this->Internals->NodeTable.back().erase(markerNode);

  vtkDebugMacro("Deleting marker " << markerNode->MarkerId);
  delete markerNode; // free memory occupied by the marker's node

  //DumpAllNodesMap();

  this->Modified();

  return true;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::DeleteAllMarkers()
{
  this->Internals->CurrentNodes.clear(); // to avoid selecting deleted nodes

  // NodeTable contains all markers/clusters nodes
  for (auto& nodesSet : this->Internals->NodeTable)
  {
    for (auto node : nodesSet)
    {
      delete node;
    }
  }
  this->Internals->NodeTable.clear();
  this->Internals->NodeTable.resize(this->ClusteringTreeDepth);

  this->Internals->MarkerVisibleMap.clear();
  this->Internals->MarkerSelectedMap.clear();

  this->Internals->AllNodesMap.clear();
  this->Internals->MarkerNodesMap.clear();

  this->Internals->UniqueMarkerId = 0;
  this->Internals->UniqueNodeId = 0;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::RecomputeClusters()
{
  //std::cout << "Enter RecomputeClusters()" << std::endl;
  // Clear current data
  std::vector<std::set<ClusteringNode*> >::iterator tableIter =
    this->Internals->NodeTable.begin();

  // delete all nodes except markers, their pointers are still
  // stored in MarkerNodesMap.
  vtkIdType lastClusterLevel = this->ClusteringTreeDepth - 1;
  for (vtkIdType i = 0; i < lastClusterLevel; ++i, ++tableIter)
  {
    std::set<ClusteringNode*>& clusterSet = *tableIter;
    std::set<ClusteringNode*>::iterator clusterIter = clusterSet.begin();
    for (; clusterIter != clusterSet.end(); ++clusterIter)
    {
      ClusteringNode* cluster = *clusterIter;
      delete cluster;
    }
  }
  this->Internals->NodeTable.clear();
  this->Internals->AllNodesMap.clear();

  // Re-initialize node table
  this->Internals->NodeTable.resize(this->ClusteringTreeDepth);

  // Keep markers ID, do not reset markers unique ID "generators"
  // as user's application might hold markers IDs and thus it will
  // be unable to delete them via DeleteMarker or even designate them
  // correctly.
  //this->Internals->UniqueMarkerId = 0;
  this->Internals->UniqueNodeId = 0;

  // Add marker nodes back into node table
  auto markerIter = this->Internals->MarkerNodesMap.begin();
  for (; markerIter != this->Internals->MarkerNodesMap.end(); ++markerIter)
  {
    ClusteringNode* const markerNode = markerIter->second;
    const auto nodeId = this->Internals->UniqueNodeId++;

    markerNode->NodeId = nodeId;
    markerNode->Level = lastClusterLevel;

    this->Internals->NodeTable[size_t(lastClusterLevel)].insert(markerNode);
    this->Internals->AllNodesMap.emplace(nodeId, markerNode);

    this->InsertIntoNodeTable(markerNode);
  }

  //assert(this->Internals->NodeTable.back().size() ==
  //       this->Internals->MarkerNodesMap.size());
  //assert(this->Internals->MarkerNodesMap.size() ==
  //       this->Internals->MarkerVisibleMap.size());
  //assert(this->Internals->MarkerSelectedMap.size() ==
  //       this->Internals->MarkerVisibleMap.size());
  //size_t nodesCount = 0;
  //for (auto& nodesSet : this->Internals->NodeTable)
  //{
  //  nodesCount += nodesSet.size();
  //}
  //assert(nodesCount == this->Internals->AllNodesMap.size());


  // Sanity check node table
  // tableIter = this->Internals->NodeTable.begin();
  // for (auto i=0u; i < this->ClusteringTreeDepth; ++i, ++tableIter)
  //   {
  //   const std::set<ClusteringNode*>& clusterSet = *tableIter;
  //   std::cout << "Level " << i << " node count " << clusterSet.size() << std::endl;
  //   }

  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkMapMarkerSet::SetMarkerVisibility(vtkIdType markerId, bool visible)
{
  // std::cout << "Set marker id " << markerId
  //           << " to visible: " << visible << std::endl;
  if (this->Internals->MarkerNodesMap.find(markerId) ==
      this->Internals->MarkerNodesMap.end())
  {
    vtkWarningMacro("Invalid Marker Id: " << markerId);
    return false;
  }

  assert(this->Internals->MarkerVisibleMap.find(markerId) !=
         this->Internals->MarkerVisibleMap.end());

  if (visible == this->Internals->MarkerVisibleMap[markerId])
  {
    return false; // no change
  }

  // Check that node wasn't deleted
  ClusteringNode* const node = this->Internals->MarkerNodesMap[markerId];

  // Update marker's node
  node->NumberOfVisibleMarkers = visible ? 1 : 0;
  // Recursively update ancestor ClusteringNode instances
  int delta = visible ? 1 : -1;
  ClusteringNode* parent = node->Parent;
  while (parent)
  {
    parent->NumberOfVisibleMarkers += delta;
    parent = parent->Parent;
  }

  this->Internals->MarkerVisibleMap[markerId] = visible;
  this->Modified();
  return true;
}

//----------------------------------------------------------------------------
bool vtkMapMarkerSet::SetMarkerSelection(vtkIdType markerId, bool selected)
{
  // std::cout << "Set marker id " << markerId
  //           << " to selected: " << selected << std::endl;
  if (this->Internals->MarkerNodesMap.find(markerId) ==
      this->Internals->MarkerNodesMap.end())
  {
    vtkWarningMacro("Invalid Marker Id: " << markerId);
    return false;
  }

  assert(this->Internals->MarkerSelectedMap.find(markerId) !=
         this->Internals->MarkerSelectedMap.end());

  if (selected == this->Internals->MarkerSelectedMap[markerId])
  {
    return false; // no change
  }

  ClusteringNode* node = this->Internals->MarkerNodesMap[markerId];
  if (!node)
  {
    std::cerr << "WARNING: Marker " << markerId << " was deleted" << std::endl;
    return false;
  }

  node->NumberOfSelectedMarkers = selected ? 1 : 0;

  // Recursively update ancestor ClusteringNoe instances
  int delta = selected ? 1 : -1;
  ClusteringNode* parent = node->Parent;
  while (parent)
  {
    parent->NumberOfSelectedMarkers += delta;
    parent = parent->Parent;
  }

  this->Internals->MarkerSelectedMap[markerId] = selected;
  this->Modified();
  return true;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::GetClusterChildren(vtkIdType clusterId,
  vtkIdList* childMarkerIds,
  vtkIdList* childClusterIds)
{
  childMarkerIds->Reset();
  childClusterIds->Reset();

  if (this->Internals->AllNodesMap.find(clusterId) ==
      this->Internals->AllNodesMap.end())
  {
    return;
  }

  // Check if node has been deleted
  ClusteringNode* node = this->Internals->AllNodesMap[clusterId];
  if (!node)
  {
    return;
  }

  std::set<ClusteringNode*>::iterator childIter = node->Children.begin();
  for (; childIter != node->Children.end(); childIter++)
  {
    ClusteringNode* child = *childIter;
    if (child->NumberOfMarkers == 1)
    {
      childMarkerIds->InsertNextId(child->MarkerId);
    }
    else
    {
      childClusterIds->InsertNextId(child->NodeId);
    } // else
  }   // for (childIter)
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::GetAllMarkerIds(vtkIdType clusterId, vtkIdList* markerIds)
{
  markerIds->Reset();
  // Check if input id is marker

  if (this->Internals->AllNodesMap.find(clusterId) ==
      this->Internals->AllNodesMap.end())
  {
    return;
  }
  ClusteringNode* node = this->Internals->AllNodesMap[clusterId];

  if (node->NumberOfMarkers == 1)
  {
    markerIds->InsertNextId(clusterId);
    return;
  }

  this->GetMarkerIdsRecursive(clusterId, markerIds);
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::GetMarkerIdsRecursive(vtkIdType clusterId,
  vtkIdList* markerIds)
{
  // Get children markers & clusters
  vtkNew<vtkIdList> childMarkerIds;
  vtkNew<vtkIdList> childClusterIds;
  this->GetClusterChildren(
    clusterId, childMarkerIds.GetPointer(), childClusterIds.GetPointer());

  // Copy marker ids
  for (int i = 0; i < childMarkerIds->GetNumberOfIds(); i++)
  {
    markerIds->InsertNextId(childMarkerIds->GetId(i));
  }

  // Traverse cluster ids
  for (int j = 0; j < childClusterIds->GetNumberOfIds(); j++)
  {
    vtkIdType childId = childClusterIds->GetId(j);
    this->GetMarkerIdsRecursive(childId, markerIds);
  }
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::Init()
{
  if (!this->Layer)
  {
    vtkErrorMacro("Invalid Layer!");
    return;
  }

  // Set up rendering pipeline

  // Add "Visible" array to polydata - to mask display
  const char* maskName = "Visible";
  vtkNew<vtkBitArray> visibles;
  visibles->SetName(maskName);
  visibles->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(visibles.GetPointer());

  // Add "Selected" array to polydata - for seleted state
  const char* selectName = "Selected";
  vtkNew<vtkBitArray> selects;
  selects->SetName(selectName);
  selects->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(selects.GetPointer());

  // Add "MarkerType" array to polydata - to select glyph
  const char* typeName = "MarkerType";
  vtkNew<vtkUnsignedCharArray> types;
  types->SetName(typeName);
  types->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(types.GetPointer());

  // Add "MarkerScale" to scale cluster glyph size
  const char* scaleName = "MarkerScale";
  vtkNew<vtkDoubleArray> scales;
  scales->SetName(scaleName);
  scales->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(scales.GetPointer());

  // Use DistanceToCamera filter to scale markers to constant screen size.
  // This scaling factor is also modified to account for marker size changes
  // defined by the user.
  const auto rend = this->Layer->GetRenderer();
  vtkNew<vtkDistanceToCamera> dFilter;
  dFilter->SetScreenSize(this->BaseMarkerSize);
  dFilter->SetRenderer(rend);
  dFilter->SetInputData(this->PolyData);
  dFilter->ScalingOn();
  dFilter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "MarkerScale");

  // Use regular polygon for cluster marker
  vtkNew<vtkRegularPolygonSource> clusterMarkerSource;
  clusterMarkerSource->SetNumberOfSides(18);
  clusterMarkerSource->SetRadius(0.25);
  clusterMarkerSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  // Switch in our mapper, and do NOT call Superclass::Init()
  this->GetActor()->SetMapper(this->Internals->GlyphMapper);
  this->Layer->AddActor(this->Actor);

  // Set up glyph mapper inputs
  this->UpdateSingleMarkerGeometry();
  this->Internals->GlyphMapper->SetSourceConnection(
    1, clusterMarkerSource->GetOutputPort());
  this->Internals->GlyphMapper->SetInputConnection(dFilter->GetOutputPort());

  // Select glyph type by "MarkerType" array
  this->Internals->GlyphMapper->SourceIndexingOn();
  this->Internals->GlyphMapper->SetSourceIndexArray(typeName);

  // Set scale by "DistanceToCamera" array
  this->Internals->GlyphMapper->SetScaleModeToScaleByMagnitude();
  this->Internals->GlyphMapper->SetScaleArray("DistanceToCamera");

  // Set visibility by "Visible" array
  this->Internals->GlyphMapper->MaskingOn();
  this->Internals->GlyphMapper->SetMaskArray(maskName);

  // Set color by "Selected" array
  this->Internals->GlyphMapper->SetColorModeToMapScalars();
  this->PolyData->GetPointData()->SetActiveScalars(selectName);

  // Set up shadow actor
  if (this->EnablePointMarkerShadow)
  {
    this->Internals->ShadowMapper->SetInputConnection(dFilter->GetOutputPort());
    this->Internals->ShadowMapper->MaskingOn();
    this->Internals->ShadowMapper->SetMaskArray(maskName);
    this->Internals->ShadowMapper->SourceIndexingOn();
    this->Internals->ShadowMapper->SetSourceIndexArray(typeName);
    this->Internals->ShadowMapper->SetScaleModeToScaleByMagnitude();
    this->Internals->ShadowMapper->SetScaleArray("DistanceToCamera");
    this->Layer->AddActor(this->Internals->ShadowActor);

    // Adjust actor's position to be behind markers
    this->Internals->ShadowActor->SetPosition(0, 0, -0.5 * this->ZCoord);
    this->Internals->ShadowMapper->Update();
  }
  this->Internals->GlyphMapper->Update();

  this->InitializeLabels(rend);
  this->Initialized = true;
}

void vtkMapMarkerSet::UpdateSingleMarkerGeometry()
{
  vtkNew<vtkPolyDataReader> pointMarkerReader;
  pointMarkerReader->ReadFromInputStringOn();

  auto shape = static_cast<const vtkMapType::Shape>(this->MarkerShape);
  pointMarkerReader->SetInputString(GetMarkerGeometry(shape));
  const int shadowVis = shape == vtkMapType::Shape::TEARDROP ? 1 : 0;
  this->Internals->ShadowActor->SetVisibility(shadowVis);

  this->Internals->GlyphMapper->SetSourceConnection(
    0, pointMarkerReader->GetOutputPort());

  this->Internals->ShapeInitTime.Modified();
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::InitializeLabels(vtkRenderer* rend)
{
  // Label mapper arrays
  const std::string labelMaskName = "LabelVis";
  vtkNew<vtkBitArray> labelVisArray;
  labelVisArray->SetName(labelMaskName.c_str());
  labelVisArray->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(labelVisArray.GetPointer());

  const std::string numMarkersName = "NumMarkers";
  vtkNew<vtkUnsignedIntArray> numMarkers;
  numMarkers->SetName(numMarkersName.c_str());
  numMarkers->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(numMarkers.GetPointer());

  // Filter-out labels of single-marker clusters and apply offset on display
  // coordinates
  auto labelSel = this->Internals->LabelSelector;
  labelSel->SetInputData(this->PolyData);
  labelSel->SelectionWindowOn();
  labelSel->SetRenderer(rend);
  labelSel->SetMaskArray(labelMaskName);
  labelSel->SetCoordinateSystem(vtkMapPointSelection::DISPLAY);
  // This places the labels approximately in the center of the cluster marker
  labelSel->SetPointOffset(2., -11., 0.);

  // Use filtered output (points on DISPLAY coordinates)
  auto mapper = this->Internals->LabelMapper;
  mapper->SetInputConnection(labelSel->GetOutputPort());
  mapper->SetLabelModeToLabelFieldData();
  mapper->SetFieldDataName(numMarkersName.c_str());
  mapper->SetCoordinateSystem(vtkLabeledDataMapper::DISPLAY);
  this->Internals->LabelActor->SetMapper(mapper);
  this->Layer->AddActor2D(this->Internals->LabelActor);

  // Set text defaults
  auto textProp = mapper->GetLabelTextProperty();
  textProp->SetFontSize(22);
  textProp->SetOpacity(0.9);
  textProp->ItalicOff();
  textProp->SetJustificationToCentered();

  // Setup callback to update necessary render parameters
  auto obs =
    vtkMakeMemberFunctionCommand(*this, &vtkMapMarkerSet::OnRenderStart);
  this->Observer.TakeReference(obs);
  rend->AddObserver(vtkCommand::StartEvent, this->Observer);

  mapper->Update();
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::OnRenderStart()
{
  if (!this->Layer)
  {
    vtkErrorMacro(<< "Invalid Layer!") return;
  }

  auto rend = this->Layer->GetRenderer();
  if (!rend)
  {
    vtkErrorMacro(<< "Invalid vtkRenderer!");
    return;
  }

  // Adjust viewport in case it changed, this is required for the point
  // selector to crop label points out of the viewport.
  int d[4] = { 0, 0, 0, 0 };
  rend->GetTiledSizeAndOrigin(&d[0], &d[1], &d[2], &d[3]);
  const int xmin = d[2];
  const int xmax = d[2] + d[0];
  const int ymin = d[3];
  const int ymax = d[3] + d[1];

  this->Internals->LabelSelector->SetSelection(xmin, xmax, ymin, ymax);
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::Update()
{
  //std::cout << __FILE__ << ":" << __LINE__ << "Enter Update()" << std::endl;
  if (!this->Initialized)
  {
    vtkErrorMacro("vtkMapMarkerSet has NOT been initialized");
  }

  // Clip zoom level to size of cluster table
  int zoomLevel = this->Layer->GetMap()->GetZoom();
  if (zoomLevel >= int(this->ClusteringTreeDepth))
  {
    zoomLevel = int(this->ClusteringTreeDepth) - 1;
  }

  // Only need to rebuild polydata if either
  // 1. Contents have been modified
  // 2. In clustering mode and zoom level changed
  bool changed = this->GetMTime() > this->UpdateTime.GetMTime();
  changed |= this->Clustering && (zoomLevel != this->Internals->ZoomLevel);
  changed |= this->GetMTime() > this->Internals->ShapeInitTime;
  if (!changed)
  {
    return;
  }

  // In non-clustering mode, markers stored at leaf level
  if (!this->Clustering)
  {
    zoomLevel = int(this->ClusteringTreeDepth) - 1;
  }
  //std::cout << __FILE__ << ":" << __LINE__ << " zoomLevel " << zoomLevel << std::endl;

  // Copy marker info into polydata
  vtkNew<vtkPoints> points;

  // Get pointers to data arrays
  vtkDataArray* array;
  array = this->PolyData->GetPointData()->GetArray("Visible");
  vtkBitArray* visibles = vtkBitArray::SafeDownCast(array);
  visibles->Reset();
  array = this->PolyData->GetPointData()->GetArray("LabelVis");
  vtkBitArray* labelVisArray = vtkBitArray::SafeDownCast(array);
  labelVisArray->Reset();
  array = this->PolyData->GetPointData()->GetArray("Selected");
  vtkBitArray* selects = vtkBitArray::SafeDownCast(array);
  selects->Reset();
  array = this->PolyData->GetPointData()->GetArray("MarkerType");
  vtkUnsignedCharArray* types = vtkUnsignedCharArray::SafeDownCast(array);
  types->Reset();
  array = this->PolyData->GetPointData()->GetArray("MarkerScale");
  vtkDoubleArray* scales = vtkDoubleArray::SafeDownCast(array);
  scales->Reset();
  array = this->PolyData->GetPointData()->GetArray("NumMarkers");
  auto numMarkersArray = vtkUnsignedIntArray::SafeDownCast(array);
  numMarkersArray->Reset();

  this->UpdateSingleMarkerGeometry();

  // Coefficients for scaling cluster size, using simple 2nd order model
  // The equation is y = k*x^2 / (x^2 + b), where k,b are coefficients
  // Logic hard-codes the min cluster factor to 1, i.e., y(2) = 1.0
  // Max value is k, which sets the horizontal asymptote.
  const double k = this->MaxClusterScaleFactor;
  const double b = 4.0 * k - 4.0;

  this->Internals->CurrentNodes.clear();

  std::set<ClusteringNode*>& nodeSet = this->Internals->NodeTable[size_t(zoomLevel)];
  std::set<ClusteringNode*>::const_iterator iter;
  for (iter = nodeSet.cbegin(); iter != nodeSet.cend(); iter++)
  {
    ClusteringNode* const node = *iter;
    if (!node->NumberOfVisibleMarkers)
    {
      continue;
    }

    double z = node->gcsCoords[2] +
      (node->NumberOfSelectedMarkers ? this->SelectedZOffset : 0.0);
    points->InsertNextPoint(node->gcsCoords[0], node->gcsCoords[1], z);

    this->Internals->CurrentNodes.push_back(node);

    if (node->NumberOfMarkers == 1)
    {
      types->InsertNextValue(MARKER_TYPE);
      const auto map = this->Layer->GetMap();
      const double adjustedMarkerSize =
        map->GetDevicePixelRatio() * int(this->PointMarkerSize);
      const double markerScale = adjustedMarkerSize / this->BaseMarkerSize;
      scales->InsertNextValue(markerScale);
    }
    else
    {
      types->InsertNextValue(CLUSTER_TYPE);
      switch (this->ClusterMarkerSizeMode)
      {
        case POINTS_CONTAINED:
        {
          // Scale with number of markers (quadratic model)
          const double x = static_cast<double>(node->NumberOfMarkers);
          const double scale = k * x * x / (x * x + b);
          scales->InsertNextValue(scale);
        }
        break;

        case USER_DEFINED:
        {
          // Scale with user defined size
          const auto map = this->Layer->GetMap();
          const double adjustedMarkerSize =
            map->GetDevicePixelRatio() * int(this->ClusterMarkerSize);
          const double markerScale = adjustedMarkerSize / this->BaseMarkerSize;
          scales->InsertNextValue(markerScale);
        }
        break;
      }
    }
    const vtkIdType numMarkers = node->NumberOfVisibleMarkers;

    // Set visibility
    const bool isVisible = numMarkers > 0;
    visibles->InsertNextValue(isVisible);

    // Set label visibility
    const bool labelVis = numMarkers > 1;
    labelVisArray->InsertNextValue(labelVis);

    // Set color
    const bool isSelected = node->NumberOfSelectedMarkers > 0;
    selects->InsertNextValue(isSelected);

    // Set number of markers
    numMarkersArray->InsertNextValue(
      static_cast<const unsigned int>(numMarkers));
  }
  this->PolyData->Reset();
  this->PolyData->SetPoints(points.GetPointer());

  this->Internals->ZoomLevel = zoomLevel;
  this->UpdateTime.Modified();

  this->Internals->LabelMapper->Update();
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::CleanUp()
{
  DeleteAllMarkers();

  auto rend = this->Layer->GetRenderer();
  rend->RemoveActor(this->Internals->ShadowActor);
  rend->RemoveActor2D(this->Internals->LabelActor);
  rend->RemoveObserver(this->Observer);

  this->Superclass::CleanUp();
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::GetClusterId(vtkIdType displayId)
{
  const size_t index = size_t(displayId);
  // Check input validity
  if (index >= this->Internals->CurrentNodes.size())
  {
    return -1;
  }

  ClusteringNode* const node = this->Internals->CurrentNodes[index];
  return node->NodeId;
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::GetMarkerId(vtkIdType displayId)
{
  const size_t index = size_t(displayId);
  // Check input validity
  if (index >= this->Internals->CurrentNodes.size())
  {
    return -1;
  }

  ClusteringNode* node = this->Internals->CurrentNodes[index];
  if (node->NumberOfMarkers == 1)
  {
    return node->MarkerId;
  }

  // else
  return -1;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::PrintClusterPath(ostream& os, vtkIdType markerId)
{
  // Gather up nodes in a list (bottom to top)
  std::vector<ClusteringNode*> nodeList;
  if (this->Internals->MarkerNodesMap.find(markerId) ==
      this->Internals->MarkerNodesMap.end())
  {
    std::cerr << "WARNING: Marker " << markerId << " was deleted" << std::endl;
    return;
  }

  ClusteringNode* markerNode = this->Internals->MarkerNodesMap[markerId];
  nodeList.push_back(markerNode);
  ClusteringNode* parent = markerNode->Parent;
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
    ClusteringNode* node = *iter;
    os << std::setw(2) << node->Level << "  " << std::setw(5) << node->NodeId
       << "  " << std::setw(5) << node->MarkerId << "  " << std::setw(4)
       << node->NumberOfVisibleMarkers << '\n';
  }
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::InsertIntoNodeTable(ClusteringNode* node)
{
  double longitude = node->gcsCoords[0];
  double latitude = vtkMercator::y2lat(node->gcsCoords[1]);
  double threshold2 =
    this->ComputeDistanceThreshold2(latitude, longitude, this->ClusterDistance);

  vtkIdType level = node->Level - 1;
  for (; level >= 0; level--)
  {
    ClusteringNode* closest = this->FindClosestNode(node, level, threshold2);
    if (closest)
    {
      // Todo Update closest node with marker info
      vtkDebugMacro(
        "Found closest node to " << node->NodeId << " at " << closest->NodeId);
      double denominator = 1.0 + closest->NumberOfMarkers;
      for (unsigned i = 0; i < 2; i++)
      {
        double numerator =
          closest->gcsCoords[i] * closest->NumberOfMarkers + node->gcsCoords[i];
        closest->gcsCoords[i] = numerator / denominator;
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
      ClusteringNode* newNode = new ClusteringNode;
      const auto newNodeId = this->Internals->UniqueNodeId++;
      this->Internals->AllNodesMap.emplace(newNodeId, newNode);
      newNode->NodeId = newNodeId;
      newNode->Level = level;
      newNode->gcsCoords[0] = node->gcsCoords[0];
      newNode->gcsCoords[1] = node->gcsCoords[1];
      newNode->gcsCoords[2] = node->gcsCoords[2];
      newNode->NumberOfMarkers = node->NumberOfMarkers;
      newNode->NumberOfVisibleMarkers = node->NumberOfVisibleMarkers;
      newNode->NumberOfSelectedMarkers = node->NumberOfSelectedMarkers;
      newNode->MarkerId = node->MarkerId;
      newNode->Parent = nullptr;
      newNode->Children.insert(node);
      this->Internals->NodeTable[size_t(level)].insert(newNode);
      vtkDebugMacro("Level " << level << " add node " << node->NodeId << " --> "
                             << newNode->NodeId);

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
    std::set<ClusteringNode*>::iterator mergingNodeIter = nodesToMerge.begin();
    for (; mergingNodeIter != nodesToMerge.end(); mergingNodeIter++)
    {
      ClusteringNode* mergingNode = *mergingNodeIter;
      if (node == mergingNode)
      {
        vtkWarningMacro("Node & merging node the same " << node->NodeId);
      }
      else
      {
        vtkDebugMacro("At level " << level << "Merging node " << mergingNode
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
      ClusteringNode* child = *childIter;
      numMarkers += child->NumberOfMarkers;
      numSelectedMarkers += child->NumberOfSelectedMarkers;
      numVisibleMarkers += child->NumberOfVisibleMarkers;
      for (int i = 0; i < 2; i++)
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
    ClusteringNode* closest = this->FindClosestNode(node, level, threshold2);
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

//----------------------------------------------------------------------------
double vtkMapMarkerSet::ComputeDistanceThreshold2(double latitude,
  double longitude,
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
  double inputLatLonCoords[2] = { latitude, longitude };
  double inputDisplayCoords[3];
  this->Layer->GetMap()->ComputeDisplayCoords(
    inputLatLonCoords, 0.0, inputDisplayCoords);

  // Offset the display coords by clustering distance
  double delta = clusteringDistance * SQRT_TWO / 2.0;
  double secondDisplayCoords[2] = { inputDisplayCoords[0] + delta,
    inputDisplayCoords[1] + delta };

  // Convert 2nd display coords to lat-lon
  double secondLatLonCoords[3] = { 0.0, 0.0, 0.0 };
  this->Layer->GetMap()->ComputeLatLngCoords(
    secondDisplayCoords, 0.0, secondLatLonCoords);

  // Convert both points to world coords
  double inputWorldCoords[3] = {
    inputLatLonCoords[1], vtkMercator::lat2y(inputLatLonCoords[0]), 0.0
  };
  double secondWorldCoords[3] = {
    secondLatLonCoords[1], vtkMercator::lat2y(secondLatLonCoords[0]), 0.0
  };

  // Return value is the distance squared
  double threshold2 =
    vtkMath::Distance2BetweenPoints(inputWorldCoords, secondWorldCoords);

  // Need to adjust by current zoom level
  int zoomLevel = this->Layer->GetMap()->GetZoom();
  double scale = static_cast<double>(1 << zoomLevel);
  threshold2 *= (scale * scale);

  return threshold2;
}

//----------------------------------------------------------------------------
vtkMapMarkerSet::ClusteringNode* vtkMapMarkerSet::FindClosestNode(
  ClusteringNode* node,
  vtkIdType zoomLevel,
  double distanceThreshold2)
{
  // Convert distanceThreshold from image to gcs coords
  // double level0Scale = 360.0 / 256.0;  // 360 degress <==> 256 tile pixels
  // double scale = level0Scale / static_cast<double>(1<<zoomLevel);
  // double gcsThreshold = scale * distanceThreshold;
  // double gcsThreshold2 = gcsThreshold * gcsThreshold;
  double scale = static_cast<double>(1 << zoomLevel);
  double gcsThreshold2 = distanceThreshold2 / scale / scale;

  ClusteringNode* closestNode = nullptr;
  double closestDistance2 = gcsThreshold2;
  std::set<ClusteringNode*>& nodeSet = this->Internals->NodeTable[size_t(zoomLevel)];
  std::set<ClusteringNode*>::const_iterator setIter = nodeSet.cbegin();
  for (; setIter != nodeSet.cend(); setIter++)
  {
    ClusteringNode* other = *setIter;
    if (other == node)
    {
      continue;
    }

    double d2 = 0.0;
    for (int i = 0; i < 2; i++)
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
void vtkMapMarkerSet::MergeNodes(ClusteringNode* node,
  ClusteringNode* mergingNode,
  std::set<ClusteringNode*>& parentsToMerge,
  vtkIdType level)
{
  vtkDebugMacro("Merging " << mergingNode->NodeId << " into " << node->NodeId);
  if (node->Level != mergingNode->Level)
  {
    vtkErrorMacro("Node " << node->NodeId << "and node " << mergingNode->NodeId
                          << "not at the same level");
  }

  // Update gcsCoords
  const vtkIdType numMarkers = node->NumberOfMarkers + mergingNode->NumberOfMarkers;
  const double denominator = static_cast<double>(numMarkers);
  for (unsigned i = 0; i < 2; i++)
  {
    double numerator = node->gcsCoords[i] * node->NumberOfMarkers +
      mergingNode->gcsCoords[i] * mergingNode->NumberOfMarkers;
    node->gcsCoords[i] = numerator / denominator;
  }
  node->NumberOfMarkers = numMarkers;
  node->NumberOfVisibleMarkers += mergingNode->NumberOfVisibleMarkers;
  node->MarkerId = -1;

  // Update links to/from children of merging node
  // Make a working copy of the child set
  std::set<ClusteringNode*> childNodeSet(mergingNode->Children);
  std::set<ClusteringNode*>::iterator childNodeIter = childNodeSet.begin();
  for (; childNodeIter != childNodeSet.end(); childNodeIter++)
  {
    ClusteringNode* childNode = *childNodeIter;
    node->Children.insert(childNode);
    childNode->Parent = node;
  }

  // Adjust parent marker counts
  // Todo recompute from children
  vtkIdType n = mergingNode->NumberOfMarkers;
  node->Parent->NumberOfMarkers += n;
  mergingNode->Parent->NumberOfMarkers -= n;

  // Remove mergingNode from its parent
  ClusteringNode* parent = mergingNode->Parent;
  parent->Children.erase(mergingNode);

  // Remember parent node if different than node's parent
  if (mergingNode->Parent && mergingNode->Parent != node->Parent)
  {
    parentsToMerge.insert(mergingNode->Parent);
  }

  // Delete mergingNode
  // todo only delete if valid level specified?
  auto count = this->Internals->NodeTable[size_t(level)].count(mergingNode);
  if (count == 1)
  {
    this->Internals->NodeTable[size_t(level)].erase(mergingNode);
  }
  else
  {
    vtkErrorMacro(
      "Node " << mergingNode->NodeId << " not found at level " << level);
  }

  this->Internals->AllNodesMap.erase(mergingNode->NodeId);

  // todo Check CurrentNodes too?
  delete mergingNode;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::ComputeNextColor(double color[3])
{
  for (unsigned i = 0; i < 3; ++i)
  {
    color[i] = static_cast<double>(palette[paletteIndex][i]) / 255.0;
  }
  paletteIndex = (paletteIndex + 1) % paletteSize;
}

void vtkMapMarkerSet::SetLabelProperties(vtkTextProperty* property)
{
  this->Internals->LabelMapper->SetLabelTextProperty(property);
}

vtkTextProperty* vtkMapMarkerSet::GetLabelProperties() const
{
  return this->Internals->LabelMapper->GetLabelTextProperty();
}

void vtkMapMarkerSet::SetLabelOffset(std::array<double, 3> offset)
{
  // Adjust device pixel ratio
  auto adjust = [](double& n, const double ratio) { n *= ratio; };
  auto func = std::bind(adjust,
    std::placeholders::_1,
    static_cast<double>(this->Layer->GetMap()->GetDevicePixelRatio()));
  std::for_each(offset.begin(), offset.end(), func);

  this->Internals->LabelSelector->SetPointOffset(offset.data());
}

std::array<double, 3> vtkMapMarkerSet::GetLabelOffset() const
{
  auto off = this->Internals->LabelSelector->GetPointOffset();
  std::array<double, 3> offset = { { off[0], off[1], off[2] } };

  // Adjust device pixel ratio (this makes offsets consistent
  // across devices).
  auto adjust = [](double& n, const double ratio) { n /= ratio; };
  auto func = std::bind(adjust,
    std::placeholders::_1,
    static_cast<double>(this->Layer->GetMap()->GetDevicePixelRatio()));
  std::for_each(offset.begin(), offset.end(), func);

  return offset;
}

void vtkMapMarkerSet::DumpAllNodesMap()
{
  // Dump all nodes
  for (const auto& entry : this->Internals->AllNodesMap)
  {
    ClusteringNode* currentNode = entry.second;
    std::cout << "Node " << entry.first << " has ";
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
