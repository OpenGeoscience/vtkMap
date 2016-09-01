/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapTile.h"
#include "vtkOsmLayer.h"

// VTK Includes
#include <vtkActor.h>
#include <vtkJPEGReader.h>
#include <vtkObjectFactory.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkPNGReader.h>
#include <vtkProperty.h>
#include <vtkTexture.h>
#include <vtkTextureMapToPlane.h>
#include <vtkNew.h>
#include <vtksys/SystemTools.hxx>

#include <curl/curl.h>

#include <cstdio>  // for remove()
#include <sstream>
#include <fstream>

vtkStandardNewMacro(vtkMapTile)

//----------------------------------------------------------------------------
vtkMapTile::vtkMapTile()
{
  Plane = 0;
  TexturePlane = 0;
  Actor = 0;
  Mapper = 0;
  this->Bin = Hidden;
  this->VisibleFlag = false;
  this->Corners[0] = this->Corners[1] =
  this->Corners[2] = this->Corners[3] = 0.0;
}

//----------------------------------------------------------------------------
vtkMapTile::~vtkMapTile()
{
  if (Plane)
    {
    Plane->Delete();
    }

  if (TexturePlane)
    {
    TexturePlane->Delete();
    }

  if (Actor)
    {
    Actor->Delete();
    }

  if (Mapper)
    {
    Mapper->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkMapTile::Build(const char* cacheDirectory)
{
  this->Plane = vtkPlaneSource::New();
  this->Plane->SetPoint1(this->Corners[2], this->Corners[1], 0.0);
  this->Plane->SetPoint2(this->Corners[0], this->Corners[3], 0.0);
  this->Plane->SetOrigin(this->Corners[0], this->Corners[1], 0.0);
  this->Plane->SetNormal(0, 0, 1);

  this->TexturePlane = vtkTextureMapToPlane::New();
  this->InitializeDownload(cacheDirectory);

  // Read the image which will be the texture
  vtkImageReader2 *imageReader = NULL;
  std::string fileExtension =
    vtksys::SystemTools::GetFilenameLastExtension(this->ImageFile);
  if (fileExtension == ".png")
    {
    vtkPNGReader *pngReader = vtkPNGReader::New();
    imageReader = pngReader;
    }
  else if (fileExtension == ".jpg")
    {
    vtkJPEGReader *jpgReader = vtkJPEGReader::New();
    imageReader = jpgReader;
    }
  else
    {
    vtkErrorMacro("Unsupported map-tile extension " << fileExtension);
    return;
    }
  imageReader->SetFileName (this->ImageFile.c_str());
  imageReader->Update();

  // Apply the texture
  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(imageReader->GetOutputPort());
  texture->SetQualityTo32Bit();
  texture->SetInterpolate(1);
  this->TexturePlane->SetInputConnection(Plane->GetOutputPort());

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->TexturePlane->GetOutputPort());

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(Mapper);
  this->Actor->SetTexture(texture.GetPointer());
  this->Actor->PickableOff();

  this->BuildTime.Modified();
  imageReader->Delete();
}

//----------------------------------------------------------------------------
void vtkMapTile::SetVisible(bool val)
{
  if (val != this->VisibleFlag)
    {
    this->VisibleFlag = val;
    if (this->VisibleFlag)
      {
      this->Bin = VisibleFlag;
      }
    else
      {
      this->Bin = Hidden;
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkMapTile::IsVisible()
{
  return this->Visible;
}

//----------------------------------------------------------------------------
void vtkMapTile::InitializeDownload(const char *cacheDirectory)
{
  // Generate destination file name
  this->ImageFile = std::string(cacheDirectory) + "/" + this->ImageKey;

  // Check if texture already exists.
  // If not, download
  while(!this->IsImageDownloaded(this->ImageFile.c_str()))
    {
    std::cerr << "Downloading " << this->ImageSource.c_str()
              << " to " << this->ImageFile
              << std::endl;
    this->DownloadImage(this->ImageSource.c_str(), this->ImageFile.c_str());
    }
}

//----------------------------------------------------------------------------
bool vtkMapTile::IsImageDownloaded(const char *outfile)
{
  // Check if file can be opened
  // Additional checks to confirm existence of correct
  // texture can be added
  std::ifstream file(outfile);
  if ( file.is_open() )
    {
    file.close();
    return true;
    }
  else
    {
    file.close();
    return false;
    }
}

//----------------------------------------------------------------------------
void vtkMapTile::DownloadImage(const char *url, const char *outfilename)
{
  // Download file from url and store in outfilename
  // Uses libcurl
  CURL* curl;
  FILE* fp;
  CURLcode res;
  char errorBuffer[CURL_ERROR_SIZE];
  curl = curl_easy_init();

  if(curl)
    {
    fp = fopen(outfilename, "wb");
    if(!fp)
      {
      vtkErrorMacro( << "Not Open")
      return;
      }

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);

    //if curl failed remove the file
    if (res != CURLE_OK)
      {
      remove(outfilename);
      vtkWarningMacro(<<errorBuffer);
      }
    }
}

//----------------------------------------------------------------------------
void vtkMapTile::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMapTile" << std::endl
     << "ImageSource: " << this->ImageSource << std::endl;
}

//----------------------------------------------------------------------------
void vtkMapTile::Init()
{
  if (this->GetMTime() > this->BuildTime.GetMTime())
    {
    vtkOsmLayer *osmLayer = vtkOsmLayer::SafeDownCast(this->Layer);
    this->Build(osmLayer->GetCacheDirectory());
    }
}

//----------------------------------------------------------------------------
void vtkMapTile::CleanUp()
{
  this->Layer->GetRenderer()->RemoveActor(this->Actor);
  this->SetLayer(0);
}

//----------------------------------------------------------------------------
void vtkMapTile::Update()
{
  this->Actor->SetVisibility(this->IsVisible());
  this->UpdateTime.Modified();
}
