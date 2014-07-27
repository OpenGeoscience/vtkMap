#include "vtkMapTile.h"

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

vtkMapTile::vtkMapTile()
{
    Center[0] = 0; Center[1] = 0; Center[2] = 0;
}

vtkMapTile::~vtkMapTile()
{

}

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

void vtkMapTile::InitialiseTexture()
{
    char *url = new char[200];
    url[0] = 0;
    strcat(url, "http://t0.tiles.virtualearth.net/tiles/a");
    strcat(url, QuadKey);
    strcat(url, ".jpeg?g=854&token=A");

    outfilename = new char[100];
    outfilename[0] = 0;
    strcat(outfilename, "Temp/temp_");
    strcat(outfilename, QuadKey);
    strcat(outfilename, ".jpeg");

    while(!IsTextureDownloaded(outfilename))
    {
        DownloadTexture(url, outfilename);
    }
}

bool vtkMapTile::IsTextureDownloaded(const char *outfile)
{
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

void vtkMapTile::DownloadTexture(const char *url, const char *outfilename)
{
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
        if(!curl)
            std::cout << "Error 1" << std::endl;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        if(!curl)
            std::cout << "Error 2" << std::endl;

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        if(!curl)
            std::cout << "Error 3" << std::endl;

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }
}

void vtkMapTile::SetCenter(double *center)
{
    Center[0] = center[0];
    Center[1] = center[1];
    Center[2] = center[2];
}

void vtkMapTile::SetCenter(double x, double y, double z)
{
    Center[0] = x;
    Center[1] = y;
    Center[2] = z;
}

void vtkMapTile::PrintSelf(ostream &os, vtkIndent indent)
{

}
