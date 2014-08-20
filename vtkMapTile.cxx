/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapTile.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapTile.h"

// VTK Includes
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPNGReader.h"
#include "vtkTextureMapToPlane.h"
#include "vtkTexture.h"

#include "curl/curl.h"
#include <sstream>
#include <fstream>

vtkStandardNewMacro(vtkMapTile)

//----------------------------------------------------------------------------
vtkMapTile::vtkMapTile()
{
  Plane = 0;
  texturePlane = 0;
  Actor = 0;
  Mapper = 0;
}

//----------------------------------------------------------------------------
vtkMapTile::~vtkMapTile()
{
  if (Plane)
    Plane->Delete();

  if (texturePlane)
    texturePlane->Delete();

  if (Actor)
    Actor->Delete();

  if (Mapper)
    Mapper->Delete();
}

//----------------------------------------------------------------------------
void vtkMapTile::init()
{

  this->Plane = vtkPlaneSource::New();
  this->Plane->SetPoint1(this->Corners[2], this->Corners[1], 0.0);
  this->Plane->SetPoint2(this->Corners[0], this->Corners[3], 0.0);
  this->Plane->SetOrigin(this->Corners[0], this->Corners[1], 0.0);
  this->Plane->SetNormal(0, 0, 1);

  texturePlane = vtkTextureMapToPlane::New();
  this->InitializeTexture();

  // Read the image which will be the texture
  vtkPNGReader* pngReader = vtkPNGReader::New();
  pngReader->SetFileName (this->ImageFile.c_str());
  pngReader->Update();

  // Apply the texture
  vtkTexture* texture = vtkTexture::New();
  texture->SetInputConnection(pngReader->GetOutputPort());
  texturePlane->SetInputConnection(Plane->GetOutputPort());

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(texturePlane->GetOutputPort());

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(Mapper);
  this->Actor->SetTexture(texture);
}

//----------------------------------------------------------------------------
void vtkMapTile::InitializeTexture()
{
  // Generate destination file name
  this->ImageFile = "Temp/" + this->ImageKey + ".png";

  std::cerr << "outfilename " << this->ImageFile << std::endl;

  // Check if texture already exists.
  // If not, download
  while(!this->IsTextureDownloaded(this->ImageFile.c_str()))
    {
    std::cerr << "Downloading " << this->ImageSource.c_str() << std::endl;
    this->DownloadTexture(this->ImageSource.c_str(), this->ImageFile.c_str());
    }
}

//----------------------------------------------------------------------------
bool vtkMapTile::IsTextureDownloaded(const char *outfile)
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
void vtkMapTile::DownloadTexture(const char *url, const char *outfilename)
{
  // Download file from url and store in outfilename
  // Uses libcurl
  CURL* curl;
  FILE* fp;
  CURLcode res;
  curl = curl_easy_init();

  if(curl)
    {
    fp = fopen(outfilename, "wb");
    std::cerr << outfilename << std::endl;
    if(!fp)
      {
      vtkErrorMacro( << "Not Open")
      return;
      }

    std::cerr << "Url " << url << std::endl;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);
    }
}

//----------------------------------------------------------------------------
void vtkMapTile::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMapTile" << std::endl
     << "ImageSource: " << this->ImageSource << std::endl;
}
