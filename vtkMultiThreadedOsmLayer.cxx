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
#include "vtkMapTile.h"

#include <vtkAtomicInt.h>
#include <vtkCallbackCommand.h>
#include <vtkConditionVariable.h>
#include <vtkMultiThreader.h>
#include <vtkMutexLock.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtksys/SystemTools.hxx>

#include <curl/curl.h>
#include <cstdio>  // for remove()
#include <sstream>
#include <stack>

// Limit the number of concurrent http requests
#define NUMBER_OF_REQUEST_THREADS 6

vtkStandardNewMacro(vtkMultiThreadedOsmLayer)

//----------------------------------------------------------------------------
class vtkMultiThreadedOsmLayer::vtkMultiThreadedOsmLayerInternals
{
public:
  vtkMultiThreader *BackgroundThreader;  // for BackgroundThreadExecute()
  vtkMultiThreader *RequestThreader;  // for RequestThreadExecute()
  int BackgroundThreadId;
  bool DownloadMode;  // sets RequestThreader behavior

  vtkAtomicInt<vtkTypeInt32> ThreadingEnabled;
  vtkConditionVariable *ThreadingCondition;

  std::stack<TileSpecList> ScheduledTiles;
  vtkAtomicInt<vtkTypeInt32> ScheduledStackSize;
  vtkMutexLock *ScheduledTilesLock;

  TileSpecList NewTiles;
  vtkMutexLock *NewTilesLock;

  // Allocate tile-spec list for each request thread
  TileSpecList ThreadTileSpecs[NUMBER_OF_REQUEST_THREADS];
};

//----------------------------------------------------------------------------
static VTK_THREAD_RETURN_TYPE StaticBackgroundThreadExecute(void *arg)
{
  vtkMultiThreadedOsmLayer *self;
  vtkMultiThreader::ThreadInfo *info =
    static_cast<vtkMultiThreader::ThreadInfo *>(arg);
  self = static_cast<vtkMultiThreadedOsmLayer*>(info->UserData);
  self->BackgroundThreadExecute();
  return VTK_THREAD_RETURN_VALUE;
}

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
  this->AsyncMode = true;

  this->Internals = new vtkMultiThreadedOsmLayerInternals;

  this->Internals->BackgroundThreader = vtkMultiThreader::New();
  this->Internals->BackgroundThreadId =
    this->Internals->BackgroundThreader->SpawnThread(
      StaticBackgroundThreadExecute, this);

  this->Internals->ThreadingEnabled = 1;
  this->Internals->ThreadingCondition = vtkConditionVariable::New();
  this->Internals->ScheduledStackSize = 0;
  this->Internals->ScheduledTilesLock = vtkMutexLock::New();
  this->Internals->NewTilesLock = vtkMutexLock::New();

  this->Internals->RequestThreader = vtkMultiThreader::New();
  this->Internals->RequestThreader->SetNumberOfThreads(
    NUMBER_OF_REQUEST_THREADS);
  this->Internals->RequestThreader->SetSingleMethod(
    StaticRequestThreadExecute, this);
}

//----------------------------------------------------------------------------
vtkMultiThreadedOsmLayer::~vtkMultiThreadedOsmLayer()
{
  this->Internals->ThreadingEnabled = 0;
  this->Internals->ThreadingCondition->Broadcast();

  this->Internals->BackgroundThreader->TerminateThread(
    this->Internals->BackgroundThreadId);
  vtkDebugMacro("Terminate Thread " << this->Internals->BackgroundThreadId);

  this->Internals->ThreadingCondition->Delete();
  this->Internals->ScheduledTilesLock->Delete();
  this->Internals->NewTilesLock->Delete();

  this->Internals->BackgroundThreader->Delete();
  this->Internals->RequestThreader->Delete();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMultiThreadedOsmLayer"
     << "\n" << indent << "BackgroundThreadId: "
     << this->Internals->BackgroundThreadId
     << "\n" << indent << "NumberOfRequestThreads: "
     << this->Internals->RequestThreader->GetNumberOfThreads()
     << std::endl;
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::Update()
{
  vtkOsmLayer::Update();
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::BackgroundThreadExecute()
{
  vtkDebugMacro("Enter BackgroundThreadExecute()");
  TileSpecList tileSpecs;
  TileSpecList newTiles;
  int workingStackSize;  // snapshot of stack size
  while (this->Internals->ThreadingEnabled)
    {
    // Check if there are scheduled tiles
    this->Internals->ScheduledTilesLock->Lock();
    workingStackSize = this->Internals->ScheduledTiles.size();
    if (workingStackSize > 0)
      {
      tileSpecs = this->Internals->ScheduledTiles.top();
      this->Internals->ScheduledTiles.pop();
      workingStackSize--;
      }
    this->Internals->ScheduledStackSize = workingStackSize;
    this->Internals->ScheduledTilesLock->Unlock();

    // Process tileSpecs (if any) using 2-pass algorithm
    // The 1st pass initializes those tiles that have an
    // image file available in the image cache
    // (from a previous application/session).
    // The 2nd pass performs http requests for the files
    // that aren't already in the image cache.
    if (!tileSpecs.empty())
      {
      // Pass 1 initializes new tiles that have image file in cache
      this->AssignTileSpecsToThreads(tileSpecs);
      this->Internals->DownloadMode = false;
      this->Internals->RequestThreader->SingleMethodExecute();
      newTiles.clear();
      tileSpecs.clear();
      this->CollateThreadResults(newTiles, tileSpecs);

      // std::cout << "After pass 1: "
      //           << "newTiles.size() " << newTiles.size()
      //           << ", tileSpecs.size() " << tileSpecs.size()
      //           << std::endl;

      // Copy new tiles to shared list
      this->UpdateNewTiles(newTiles);

      // Check if there are newer tiles in the scheduled stack
      if (this->Internals->ScheduledStackSize > workingStackSize)
        {
        // If there are newer tiles, loop back and do them next
        continue;
        }

      // Pass 2 downloads image files and initializes new tiles.
      // Process one tile per thread so that we can break out
      // sooner when new tiles are scheduled.
      while (tileSpecs.size() > 0 &&
             this->Internals->ScheduledStackSize == workingStackSize)
        {
        //std::cout << "Before pass 2: " << tileSpecs.size() << std::endl;
        this->AssignOneTileSpecPerThread(tileSpecs);
        //std::cout << "After assign : " << tileSpecs.size() << std::endl;
        this->Internals->DownloadMode = true;
        this->Internals->RequestThreader->SingleMethodExecute();
        newTiles.clear();
        this->CollateThreadResults(newTiles, tileSpecs);
        //std::cout << "After collate : " << tileSpecs.size() << std::endl;
        this->UpdateNewTiles(newTiles);
        newTiles.clear();
        }  // while
      }  // if (tileSpecs not empty)

    // If we broke out of loop early, continue processing
    if (tileSpecs.size() > 0)
      {
      continue;
      }

    // Check if there are more scheduled tiles to process now
    this->Internals->ScheduledTilesLock->Lock();
    if (this->Internals->ScheduledTiles.size() == 0)
      {
      // If not, wait for condition variable
      this->Internals->ThreadingCondition->Wait(
        this->Internals->ScheduledTilesLock);
      }
    this->Internals->ScheduledTilesLock->Unlock();
    }  // while
}

//----------------------------------------------------------------------------
// Checks if image file is in cache, and if so, creates tile
void vtkMultiThreadedOsmLayer::RequestThreadExecute(int threadId)
{
  //vtkDebugMacro("Enter RequestThreadExecute, thread " << threadId);
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

    if (this->Internals->DownloadMode)
      {
      // If DownloadMode, perform http request
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
        }
      }
    else
      {
      // If *not* DownloadMode, check for image file in cache
      if (vtksys::SystemTools::FileExists(filename.c_str(), true))
        {
        this->CreateTile(spec);
        }
      }
    }  // for
}

//----------------------------------------------------------------------------
vtkMap::AsyncState vtkMultiThreadedOsmLayer::ResolveAsync()
{
  // Check for new tiles
  TileSpecList newTiles;
  this->Internals->NewTilesLock->Lock();
  if (this->Internals->NewTiles.size() > 0)
    {
    // Copy to local list and clear Internals list
    newTiles = this->Internals->NewTiles;
    this->Internals->NewTiles.clear();
    }
  this->Internals->NewTilesLock->Unlock();

  // Add newTiles to cache
  TileSpecList::iterator specIter = newTiles.begin();
  for (; specIter != newTiles.end(); specIter++)
    {
    vtkMapTileSpecInternal spec = *specIter;
    int zoom = spec.ZoomXY[0];
    int x = spec.ZoomXY[1];
    int y = spec.ZoomXY[2];
    this->AddTileToCache(zoom, x, y, spec.Tile);
    }

  vtkMap::AsyncState result = vtkMap::AsyncIdle;  // return value
  bool tilesTodo = this->Internals->ScheduledStackSize > 0;
  if (newTiles.size() > 0)
    {
    //std::cout << "Added new tiles: " << newTiles.size() << std::endl;
    this->Modified();
    if (tilesTodo)
      {
      result = vtkMap::AsyncPartialUpdate;
      }
    else
      {
      result = vtkMap::AsyncFullUpdate;
      }
    }
  else if (tilesTodo)
    {
    result = vtkMap::AsyncPending;
    }

  return result;
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
  if (tileSpecs.size() > 0)
    {
    // Add newTileSpecs to scheduled tiles stack
    //std::cout << "Scheduling tiles " << newTileSpecs.size() << std::endl;
    this->Internals->ScheduledTilesLock->Lock();
    this->Internals->ScheduledTiles.push(tileSpecs);
    this->Internals->ScheduledStackSize =
      this->Internals->ScheduledTiles.size();
    this->Internals->ThreadingCondition->Broadcast();
    this->Internals->ScheduledTilesLock->Unlock();
    }
  else
    {
    this->RenderTiles(tiles);
    }
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
AssignTileSpecsToThreads(TileSpecList& tileSpecs)
{
  // Clear current thread tile spec lists
  int numThreads = this->Internals->RequestThreader->GetNumberOfThreads();
  for (int i=0; i<numThreads; i++)
    {
    this->Internals->ThreadTileSpecs[i].clear();
    }

  // Distribute inputs across threads
  int index;
  for (int i=0; i<tileSpecs.size(); i++)
    {
    index = i % numThreads;
    this->Internals->ThreadTileSpecs[index].push_back(tileSpecs[i]);
    }
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
CollateThreadResults(TileSpecList& newTiles, TileSpecList& tileSpecs)
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
        newTiles.push_back(spec);
        }
      else
        {
        tileSpecs.push_back(spec);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::UpdateNewTiles(TileSpecList& newTiles)
{
  if (newTiles.empty())
    {
    return;
    }

  this->Internals->NewTilesLock->Lock();
  this->Internals->NewTiles.insert(this->Internals->NewTiles.begin(),
                                   newTiles.begin(), newTiles.end());
  this->Internals->NewTilesLock->Unlock();
}
