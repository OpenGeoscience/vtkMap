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
#include "vtkJPEGReader.h"
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
  outfilename = 0;
  texturePlane = 0;
  Actor = 0;
  Mapper = 0;

  this->Center[0] = 0; this->Center[1] = 0; this->Center[2] = 0;
}

//----------------------------------------------------------------------------
vtkMapTile::~vtkMapTile()
{
  if (Plane)
    Plane->Delete();

  if (outfilename)
    delete[] outfilename;

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
  this->Plane->SetCenter(this->Center[0], this->Center[1], this->Center[2]);
  this->Plane->SetNormal(0, 0, 1);

  texturePlane = vtkTextureMapToPlane::New();
  this->InitializeTexture();

  // Read the image which will be the texture
  vtkJPEGReader* jPEGReader = vtkJPEGReader::New();
  jPEGReader->SetFileName (outfilename);
  jPEGReader->Update();

  // Apply the texture
  vtkTexture* texture = vtkTexture::New();
  texture->SetInputConnection(jPEGReader->GetOutputPort());
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
  // Generate URL to Bing Map tile corresponding to the QuadKey
  char *url = new char[200];
  url[0] = 0;
  strcat(url, "http://t0.tiles.virtualearth.net/tiles/a");
  strcat(url, QuadKey);
  strcat(url, ".jpeg?g=854&token=A");

  // Generate destination file name
  outfilename = new char[100];
  outfilename[0] = 0;
  strcat(outfilename, "Temp/temp_");
  strcat(outfilename, QuadKey);
  strcat(outfilename, ".jpeg");

  std::cerr << "outfilename " << outfilename << std::endl;

  // Check if texture already exists.
  // If not, download
  while(!this->IsTextureDownloaded(outfilename))
    {
    this->DownloadTexture(url, outfilename);
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
  CURL *curl;
  FILE *fp;
  CURLcode res;
  curl = curl_easy_init();

  if(curl)
    {
    fp = fopen(outfilename,"wb");
    std::cerr << outfilename << std::endl;
    if(!fp)
      {
      vtkErrorMacro( << "Not Open")
      return;
      }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);
    }
}

//----------------------------------------------------------------------------
void vtkMapTile::SetCenter(double *center)
{
  this->Center[0] = center[0];
  this->Center[1] = center[1];
  this->Center[2] = center[2];
}

//----------------------------------------------------------------------------
void vtkMapTile::SetCenter(double x, double y, double z)
{
  this->Center[0] = x;
  this->Center[1] = y;
  this->Center[2] = z;
}

//----------------------------------------------------------------------------
void vtkMapTile::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMapTile" << std::endl
     << "QuadKey: " << QuadKey << std::endl
     << "Position: " << this->Center[0] << indent << this->Center[1] <<  indent
     << this->Center[2] << std::endl;
}
