/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreadedOsmLayer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMultiThreadedOsmLayer.h"

#include <vtkObjectFactory.h>
#include <vtkMultiThreader.h>

#include <curl/curl.h>
#include <cstdio>  // for remove()
#include <sstream>

vtkStandardNewMacro(vtkMultiThreadedOsmLayer)

// Limit number of concurrent http requests
#define NUMBER_OF_REQUEST_THREADS 6

//----------------------------------------------------------------------------
class vtkMultiThreadedOsmLayer::vtkMultiThreadedOsmLayerInternals
{
public:
  vtkMultiThreader *RequestThreader;

  // Allocate tile-spec list for each request thread
  TileSpecList ThreadTileSpecs[NUMBER_OF_REQUEST_THREADS];
};

//----------------------------------------------------------------------------
static VTK_THREAD_RETURN_TYPE StaticRequestThreadExecute(void *arg)
{
  vtkMultiThreadedOsmLayer *self;
  vtkMultiThreader::ThreadInfo *info =
    static_cast<vtkMultiThreader::ThreadInfo *>(arg);
  self = static_cast<vtkMultiThreadedOsmLayer*>(info->UserData);
  int threadId = info->ThreadID;
  self->RequestThreadExecute(threadId);
  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
vtkMultiThreadedOsmLayer::vtkMultiThreadedOsmLayer()
{
  this->Internals = new vtkMultiThreadedOsmLayerInternals;
  this->Internals->RequestThreader = vtkMultiThreader::New();
  this->Internals->RequestThreader->SetNumberOfThreads(
    NUMBER_OF_REQUEST_THREADS);
  this->Internals->RequestThreader->SetSingleMethod(
    StaticRequestThreadExecute, this);
}

//----------------------------------------------------------------------------
vtkMultiThreadedOsmLayer::~vtkMultiThreadedOsmLayer()
{
  this->Internals->RequestThreader->Delete();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMultiThreadedOsmLayer"
     << std::endl;
}

//----------------------------------------------------------------------------
// (Thread Safe) Downloads image file then creates tile
// Updates internal tile spec list
void vtkMultiThreadedOsmLayer::RequestThreadExecute(int threadId)
{
  vtkDebugMacro("Enter RequestThreadExecute, thread " << threadId);
  std::stringstream oss;

  // Get reference to tiles assigned to this thread
  TileSpecList& tileSpecs = this->Internals->ThreadTileSpecs[threadId];
  if (tileSpecs.empty())
    {
    return;
    }

  // Process all tile specs
  std::vector<vtkMapTileSpecInternal>::iterator tileSpecIter =
    tileSpecs.begin();
  for (; tileSpecIter != tileSpecs.end(); tileSpecIter++)
    {
    vtkMapTileSpecInternal& spec = *tileSpecIter;

    oss.str("");
    oss << this->GetCacheDirectory() << "/"
        << spec.ZoomRowCol[0]
        << spec.ZoomRowCol[1]
        << spec.ZoomRowCol[2]
        << ".png";
    std::string filename = oss.str();

    oss.str("");
    oss << "http://tile.openstreetmap.org"
        << "/" << spec.ZoomRowCol[0]
        << "/" << spec.ZoomRowCol[1]
        << "/" << spec.ZoomRowCol[2]
        << ".png";
    std::string url = oss.str();

    if (this->DownloadImageFile(url, filename))
      {
      this->CreateTile(spec);
      spec.Tile->Init();
      }
    }  // for
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::AddTiles()
{
  if (!this->Renderer)
    {
    return;
    }

  std::vector<vtkMapTile*> tiles;
  std::vector<vtkMapTileSpecInternal> tileSpecs;

  this->SelectTiles(tiles, tileSpecs);
  while (tileSpecs.size() > 0)
    {
    this->AssignOneTileSpecPerThread(tileSpecs);
    this->Internals->RequestThreader->SingleMethodExecute();
    this->CollateThreadResults(tiles, tileSpecs);
    }
  this->RenderTiles(tiles);
}

//----------------------------------------------------------------------------
// Note: This method is very similar to vtkMapTile::DownloadImage()
bool vtkMultiThreadedOsmLayer::
DownloadImageFile(std::string url, std::string filename)
{
  //std::cout << "Downloading " << filename << std::endl;
  CURL* curl;
  FILE* fp;
  CURLcode res;
  char errorBuffer[CURL_ERROR_SIZE];  // for debug
  long httpStatus = 0;  // for debug
  curl = curl_easy_init();
  if (!curl)
    {
    vtkErrorMacro(<< "curl_easy_init() failed" );
    return false;
    }

  fp = fopen(filename.c_str(), "wb");
  if(!fp)
    {
    vtkErrorMacro( << "Cannot open file " << filename.c_str());
    return false;
    }

  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  res = curl_easy_perform(curl);

  curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpStatus);
  vtkDebugMacro("Download " << url.c_str() << " status: " << httpStatus);
  curl_easy_cleanup(curl);
  fclose(fp);

  // If there was an error, remove invalid image file
  if (res != CURLE_OK)
    {
    remove(filename.c_str());
    vtkErrorMacro(<< errorBuffer);
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
// Consider moving this to base class (vtkOsmLayer)
vtkMapTile *vtkMultiThreadedOsmLayer::
CreateTile(vtkMapTileSpecInternal& spec)
{
  std::stringstream oss;

  vtkMapTile *tile = vtkMapTile::New();
  tile->SetLayer(this);
  tile->SetCorners(spec.Corners);

  // Set the image key
  oss.str("");
  oss << spec.ZoomRowCol[0]
      << spec.ZoomRowCol[1]
      << spec.ZoomRowCol[2];
  tile->SetImageKey(oss.str());

  // Set tile texture source
  oss.str("");
  oss << "http://tile.openstreetmap.org"
      << "/" << spec.ZoomRowCol[0]
      << "/" << spec.ZoomRowCol[1]
      << "/" << spec.ZoomRowCol[2]
      << ".png";
  tile->SetImageSource(oss.str());

  // Initialize the tile
  spec.Tile = tile;
  return tile;
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::
AssignOneTileSpecPerThread(TileSpecList& tileSpecs)
{
  // Move tileSpecs from end of tileSpecs to thread
  int numThreads = this->Internals->RequestThreader->GetNumberOfThreads();
  for (int i=0; i<numThreads; i++)
    {
    this->Internals->ThreadTileSpecs[i].clear();
    if (tileSpecs.size() > 0)
      {
      // vtkMapTileSpecInternal spec = tileSpecs.back();
      // std::stringstream oss;
      // oss << "Assign "
      //     << spec.ZoomRowCol[0] << spec.ZoomRowCol[1] << spec.ZoomRowCol[2]
      //     << std::endl;
      // std::cout << oss.str();

      this->Internals->ThreadTileSpecs[i].push_back(tileSpecs.back());
      tileSpecs.pop_back();
      }
    }
}

//----------------------------------------------------------------------------
// Checks thread results and updates lists
void vtkMultiThreadedOsmLayer::
CollateThreadResults(std::vector<vtkMapTile*> tiles, TileSpecList& tileSpecs)
{
  int numThreads = this->Internals->RequestThreader->GetNumberOfThreads();
  for (int i=0; i<numThreads; i++)
    {
    TileSpecList::iterator specIter =
      this->Internals->ThreadTileSpecs[i].begin();
    for (; specIter != this->Internals->ThreadTileSpecs[i].end(); specIter++)
      {
      vtkMapTileSpecInternal& spec = *specIter;
      if (spec.Tile)
        {
        tiles.push_back(spec.Tile);
        }
      else
        {
        tileSpecs.push_back(spec);
        }
      }
    }
}
