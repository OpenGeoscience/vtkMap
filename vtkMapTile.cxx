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

    Center[0] = 0; Center[1] = 0; Center[2] = 0;
}

//----------------------------------------------------------------------------
vtkMapTile::~vtkMapTile()
{
    if(Plane)
        Plane->Delete();

    if(outfilename)
        delete[] outfilename;

    if(texturePlane)
        texturePlane->Delete();

    if(Actor)
        Actor->Delete();

    if(Mapper)
        Mapper->Delete();
}

//----------------------------------------------------------------------------
void vtkMapTile::init()
{

    this->Plane = vtkPlaneSource::New();
    Plane->SetCenter(Center[0], Center[1], Center[2]);
    Plane->SetNormal(0, 0, 1);

    texturePlane = vtkTextureMapToPlane::New();
    InitialiseTexture();

    // Read the image which will be the texture
    vtkJPEGReader *jPEGReader = vtkJPEGReader::New();
    jPEGReader->SetFileName (outfilename);
    jPEGReader->Update();

    // Apply the texture
    vtkTexture *texture = vtkTexture::New();
    texture->SetInputConnection(jPEGReader->GetOutputPort());

    texturePlane->SetInputConnection(Plane->GetOutputPort());

    this->Mapper = vtkPolyDataMapper::New();
    Mapper->SetInputConnection(texturePlane->GetOutputPort());

    this->Actor = vtkActor::New();
    Actor->SetMapper(Mapper);
    Actor->SetTexture(texture);
}

//----------------------------------------------------------------------------
void vtkMapTile::InitialiseTexture()
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

    // Check if texture already exists.
    // If not, download
    while(!IsTextureDownloaded(outfilename))
    {
        DownloadTexture(url, outfilename);
    }
}

//----------------------------------------------------------------------------
bool vtkMapTile::IsTextureDownloaded(const char *outfile)
{
    // Check if file can be opened
    // Additional checks to confirm existence of correct
    // texture can be added
    std::ifstream file(outfile);
    if(file.is_open())
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
        if(!fp)
        {
            std::cout << "Not Open" << std::endl;
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
    Center[0] = center[0];
    Center[1] = center[1];
    Center[2] = center[2];
}

//----------------------------------------------------------------------------
void vtkMapTile::SetCenter(double x, double y, double z)
{
    Center[0] = x;
    Center[1] = y;
    Center[2] = z;
}

//----------------------------------------------------------------------------
void vtkMapTile::PrintSelf(ostream &os, vtkIndent indent)
{
    Superclass::PrintSelf(os, indent);
    os << "vtkMapTile" << std::endl
       << "QuadKey: " << QuadKey << std::endl
       << "Position: " << Center[0] << indent << Center[1] <<  indent << Center[2] << std::endl;
}
