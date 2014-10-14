/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapMarkerSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapMarkerSet.h"
#include "vtkMapPickResult.h"
#include "vtkMercator.h"
#include "vtkTeardropSource.h"

#include <vtkActor.h>
#include <vtkDataArray.h>
#include <vtkDistanceToCamera.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkGlyph3D.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>

#include <algorithm>
#include <vector>

const int NumberOfClusterLevels = 20;

//----------------------------------------------------------------------------
// Internal class for cluster tree nodes
// Each node represents either one marker or a cluster of nodes
class vtkMapMarkerSet::ClusteringNode
{
public:
  int NodeId;  // for dev use
  int Level;  // for dev
  double gcsCoords[2];
  ClusteringNode *Parent;
  std::set<ClusteringNode*> Children;
  int NumberOfMarkers;  // 1 for single-point nodes, >1 for clusters
  int MarkerId;  // only relevant for single-point markers (not clusters)
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMapMarkerSet)

//----------------------------------------------------------------------------
class vtkMapMarkerSet::MapMarkerSetInternals
{
public:
  bool MarkersChanged;
  std::vector<ClusteringNode*> CurrentNodes;  // in this->PolyData

  // Used for marker clustering:
  int ZoomLevel;
  std::vector<std::set<ClusteringNode*> > NodeTable;
  int NumberOfMarkers;
  double ClusterDistance;
  int NumberOfNodes;  // for dev use
  std::vector<ClusteringNode*> AllNodes;   // for dev
};

//----------------------------------------------------------------------------
vtkMapMarkerSet::vtkMapMarkerSet()
{
  this->Initialized = false;
  this->Renderer = NULL;
  this->PolyData = vtkPolyData::New();
  this->Mapper = NULL;
  this->Actor = NULL;
  this->Clustering = false;
  this->MaxClusterScaleFactor = 2.0;

  this->Internals = new MapMarkerSetInternals;
  this->Internals->MarkersChanged = false;
  this->Internals->ZoomLevel = -1;
  std::set<ClusteringNode*> clusterSet;
  std::fill_n(std::back_inserter(this->Internals->NodeTable),
              NumberOfClusterLevels, clusterSet);
  this->Internals->NumberOfMarkers = 0;
  this->Internals->ClusterDistance = 80.0;
  this->Internals->NumberOfNodes = 0;
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
  if (this->Mapper)
    {
    this->Mapper->Delete();
    }
  if (this->Actor)
    {
    this->Actor->Delete();
    }
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::AddMarker(double latitude, double longitude)
{
  // Set marker id
  int markerId = this->Internals->NumberOfMarkers++;
  vtkDebugMacro("Adding marker " << markerId);

  // Instantiate ClusteringNode
  ClusteringNode *node = new ClusteringNode;
  this->Internals->AllNodes.push_back(node);
  node->NodeId = this->Internals->NumberOfNodes++;
  node->Level = 0;
  node->gcsCoords[0] = longitude;
  node->gcsCoords[1] = vtkMercator::lat2y(latitude);
  node->NumberOfMarkers = 1;
  node->Parent = 0;
  node->MarkerId = markerId;
  vtkDebugMacro("Created ClusteringNode id " << node->NodeId);

  // todo refactor into separate method
  // todo calc initial cluster distance here and divide down
  if (this->Clustering)
    {
    // Insertion step: Starting at bottom level, populate NodeTable until
    // a clustering partner is found.
    int level = this->Internals->NodeTable.size() - 1;
    node->Level = level;
    vtkDebugMacro("Inserting Node " << node->NodeId
                  << " into level " << level);
    this->Internals->NodeTable[level].insert(node);
    double coords[2];

    level--;
    double threshold = this->Internals->ClusterDistance;
    for (; level >= 0; level--)
      {
      ClusteringNode *closest =
        this->FindClosestNode(node, level, threshold);
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
        newNode->NumberOfMarkers = node->NumberOfMarkers;
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
      double numerator[2];
      numerator[0] = numerator[1] = 0.0;
      std::set<ClusteringNode*>::iterator childIter = node->Children.begin();
      for (; childIter != node->Children.end(); childIter++)
        {
        ClusteringNode *child = *childIter;
        numMarkers += child->NumberOfMarkers;
        for (int i=0; i<2; i++)
          {
          numerator[i] += child->NumberOfMarkers * child->gcsCoords[i];
          }
        }
      node->NumberOfMarkers = numMarkers;
      if (numMarkers > 1)
        {
        node->MarkerId = -1;
        }
      node->gcsCoords[0] = numerator[0] / numMarkers;
      node->gcsCoords[1] = numerator[1] / numMarkers;

      // Check for new clustering partner
      ClusteringNode *closest =
        this->FindClosestNode(node, level, threshold);
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
      ClusteringNode *node = this->Internals->AllNodes[i];
      std::cout << "Node " << i << " has ";
      if (node)
        {
      std::cout << node->Children.size() << " children, "
                << node->NumberOfMarkers << " markers, and "
                << " marker id " << node->MarkerId;
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
void vtkMapMarkerSet::RemoveMarkers()
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
void vtkMapMarkerSet::Update(int zoomLevel)
{
  // Make sure everything is initialized
  if (!this->Initialized && this->Renderer)
    {
    this->InitializeRenderingPipeline();
    this->Initialized = true;
    }

  // Clip zoom level to size of cluster table
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
    zoomLevel = 0;
    }

  // Copy marker info into polydata
  vtkNew<vtkPoints> points;
  unsigned char markerType;  // 0 == point marker, 1 == cluster marker
  markerType = 0;

  // Get pointers to data arrays
  vtkDataArray *array;
  array = this->PolyData->GetPointData()->GetArray("Color");
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::SafeDownCast(array);
  colors->Reset();
  array = this->PolyData->GetPointData()->GetArray("MarkerType");
  vtkUnsignedCharArray *types = vtkUnsignedCharArray::SafeDownCast(array);
  types->Reset();
  array = this->PolyData->GetPointData()->GetArray("MarkerScale");
  vtkDoubleArray *scales = vtkDoubleArray::SafeDownCast(array);
  scales->Reset();

  unsigned char kwBlue[] = {0, 83, 155};
  unsigned char kwGreen[] = {0, 169, 179};

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
    points->InsertNextPoint(node->gcsCoords);
    this->Internals->CurrentNodes.push_back(node);
    if (node->NumberOfMarkers == 1)  // point marker
      {
      markerType = 0;
      types->InsertNextValue(markerType);
      colors->InsertNextTupleValue(kwBlue);
      scales->InsertNextValue(1.0);
      }
    else  // cluster marker
      {
      markerType = 1;
      types->InsertNextValue(markerType);
      colors->InsertNextTupleValue(kwGreen);
      double x = static_cast<double>(node->NumberOfMarkers);
      double scale = k*x*x / (x*x + b);
      scales->InsertNextValue(scale);
      }
    }
  this->PolyData->Reset();
  this->PolyData->SetPoints(points.GetPointer());

  this->Internals->MarkersChanged = false;
  this->Internals->ZoomLevel = zoomLevel;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::
PickPoint(vtkRenderer *renderer, vtkPicker *picker, int displayCoords[2],
    vtkMapPickResult *result)
{
  result->SetDisplayCoordinates(displayCoords);
  result->SetMapLayer(0);
  // TODO Need general display <--> gcs coords
  result->SetMapFeatureType(VTK_MAP_FEATURE_NONE);
  result->SetNumberOfMarkers(0);
  result->SetMapFeatureId(-1);

  vtkPointPicker *pointPicker = vtkPointPicker::SafeDownCast(picker);
  if (pointPicker &&
      (pointPicker->Pick(displayCoords[0], displayCoords[1], 0.0, renderer)))
    {
    // std::cout << "Using tolerance " << pointPicker->GetTolerance() << std::endl;
    // vtkPoints *pickPoints = pointPicker->GetPickedPositions();
    // std::cout << "Picked " << pickPoints->GetNumberOfPoints() << " points "
    //           << std::endl;

    vtkIdType pointId = pointPicker->GetPointId();
    if (pointId > 0)
      {
      vtkDataArray *dataArray =
        pointPicker->GetDataSet()->GetPointData()->GetArray("InputPointIds");
      vtkIdTypeArray *glyphIdArray = vtkIdTypeArray::SafeDownCast(dataArray);
      if (glyphIdArray)
        {
        int glyphId = glyphIdArray->GetValue(pointId);
        // std::cout << "Point id " << pointId
        //           << " - Data " << glyphId << std::endl;

        ClusteringNode *node = this->Internals->CurrentNodes[glyphId];
        // std::cout << "Marker id " << marker->MarkerId
        //           << ", Count " << marker->NumberOfMarkers
        //           << ", at " << marker->Latitude << ", " << marker->Longitude
        //           << std::endl;
        result->SetNumberOfMarkers(node->NumberOfMarkers);
        if (node->NumberOfMarkers == 1)
          {
          result->SetMapFeatureType(VTK_MAP_FEATURE_MARKER);
          result->SetMapFeatureId(node->MarkerId);
          }
        else if (node->NumberOfMarkers > 1)
          {
          result->SetMapFeatureType(VTK_MAP_FEATURE_CLUSTER);
          }

        result->SetLatitude(vtkMercator::y2lat(node->gcsCoords[1]));
        result->SetLongitude(node->gcsCoords[0]);
        //result->Print(std::cout);
        }
      }
    }

}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::InitializeRenderingPipeline()
{

  // Add "Color" data array to polydata
  const char *colorName = "Color";
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetName(colorName);
  colors->SetNumberOfComponents(3);  // for RGB
  this->PolyData->GetPointData()->AddArray(colors.GetPointer());

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
  dFilter->SetRenderer(this->Renderer);
  dFilter->SetInputData(this->PolyData);
  if (this->Clustering)
    {
    dFilter->ScalingOn();
    dFilter->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "MarkerScale");
    }

  // Use teardrop shape for individual markers
  vtkNew<vtkTeardropSource> markerGlyphSource;
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

  // Setup glyph
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetSourceConnection(0, rotateMarker->GetOutputPort());
  glyph->SetSourceConnection(1, clusterGlyphSource->GetOutputPort());
  glyph->SetInputConnection(dFilter->GetOutputPort());
  glyph->SetIndexModeToVector();
  glyph->ScalingOn();
  glyph->SetScaleFactor(1.0);
  glyph->SetScaleModeToScaleByScalar();
  glyph->SetColorModeToColorByScalar();
  // Just gotta know this:
  glyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");
  glyph->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "MarkerType");
  glyph->SetInputArrayToProcess(
    3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Color");
  glyph->GeneratePointIdsOn();

  // Setup mapper and actor
  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(glyph->GetOutputPort());
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Renderer->AddActor(this->Actor);
}

//----------------------------------------------------------------------------
vtkMapMarkerSet::ClusteringNode*
vtkMapMarkerSet::
FindClosestNode(ClusteringNode *node, int zoomLevel, double distanceThreshold)
{
  // Convert distanceThreshold from image to gcs coords
  double level0Scale = 360.0 / 256.0;  // 360 degress <==> 256 tile pixels
  double scale = level0Scale / static_cast<double>(1<<zoomLevel);
  double gcsThreshold = scale * distanceThreshold;
  double gcsThreshold2 = gcsThreshold * gcsThreshold;

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
