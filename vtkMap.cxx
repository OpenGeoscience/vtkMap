#include "vtkMap.h"

#include "vtkObjectFactory.h"
#include "vtkMapTile.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkPlaneSource.h"

#include <sstream>
vtkStandardNewMacro(vtkMap)

vtkMap::vtkMap()
{
    this->Zoom = 1;
    this->latitude = 0;
    this->longitude = 0;
}

vtkMap::~vtkMap()
{

}

void vtkMap::PrintSelf(ostream &os, vtkIndent indent)
{

}

void vtkMap::Update()
{
    RemoveTiles();
    AddTiles();
}

void vtkMap::RemoveTiles()
{

}

void vtkMap::AddTiles()
{
    double xmax = Renderer->GetSize()[0];
    double ymax = Renderer->GetSize()[1];

    double *bottomLeft, *topRight;

    Renderer->SetWorldPoint(0, 0, 0, 0);
    Renderer->WorldToDisplay();
    bottomLeft = Renderer->GetDisplayPoint();
    std::cout << "Bottom Left Display: " << bottomLeft[0] << " " << bottomLeft[1] << " " << bottomLeft[2] << std::endl;
    double bottomLeftArray[2];
    bottomLeftArray[0] = bottomLeft[0];
    bottomLeftArray[1] = bottomLeft[1];

    Renderer->SetWorldPoint(1, 1, 0, 0);
    Renderer->WorldToDisplay();
    topRight = Renderer->GetDisplayPoint();
    std::cout << "Top Right Display: " << topRight[0] << " " << topRight[1] << " " << topRight[2] << std::endl;
    double topRightArray[2];
    topRightArray[0] = topRight[0];
    topRightArray[1] = topRight[1];

    std::cout << "XMax: " << xmax << " YMax: " << ymax << std::endl;
    int height = topRightArray[0] - bottomLeftArray[0];
    int width = topRightArray[1] - bottomLeftArray[1];

    std::cout << "Height: " << height << " Width: " << width << " Zoom: " << this->Zoom << std::endl;

    int ROWS = ymax / height + 1;
    ROWS = ROWS < (2 << (this->Zoom - 1)) ? ROWS : (2 << (this->Zoom - 1));
    int COLS = xmax / width + 1;
    COLS = COLS < (2 << (this->Zoom - 1)) ? COLS : (2 << (this->Zoom - 1));

    std::cout << "Rows: " << ROWS << " COLS: " << COLS << std::endl;

    int pixX, pixY, tileX, tileY;
    LatLongToPixelXY(latitude, longitude, this->Zoom, pixX, pixY);
    PixelXYToTileXY(pixX, pixY, tileX, tileY);

    std::cout << "TileX: " << tileX << " TileY: " << tileY << std::endl;

    int temp_tileX = tileX - ROWS / 2;
    int temp_tileY = tileY - ROWS / 2 > 0 ? tileY - ROWS / 2 : 0;

    for(int i = -ROWS / 2; i < ROWS / 2; i++)
    {
        temp_tileX = tileX - ROWS / 2 > 0 ? tileX - ROWS / 2 : 0;
        for(int j = -COLS / 2; j < COLS / 2; j++)
        {
            vtkMapTile *tile = vtkMapTile::New();
            tile->SetCenter(j, -i, 0);
            tile->SetQuadKey(TileXYToQuadKey(temp_tileX, temp_tileY, this->Zoom).c_str());
            tile->init();
            std::cout << tile->GetQuadKey() << " ( " << temp_tileX << " " << temp_tileY << " )";
            Renderer->AddActor(tile->GetActor());
            temp_tileX++;
        }
        std::cout << std::endl;
        temp_tileY++;
    }
}

double vtkMap::Clip(double n, double minValue, double maxValue)
{
    double max = n > minValue ? n : minValue;
    double min = max > maxValue ? maxValue : max;
    return min;
}

void vtkMap::LatLongToPixelXY(double latitude, double longitude, int levelOfDetail, int &pixelX, int &pixelY)
{
    latitude = Clip(latitude, MinLatitude, MaxLatitude);
    longitude = Clip(longitude, MinLongitude, MaxLongitude);

    double x = (longitude + 180) / 360;
    double sinLatitude = sin(latitude * 3.142 / 180);
    double y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * 3.142);

    uint mapSize = MapSize(levelOfDetail);
    pixelX = (int) Clip(x * mapSize + 0.5, 0, mapSize - 1);
    pixelY = (int) Clip(y * mapSize + 0.5, 0, mapSize - 1);
}

uint vtkMap::MapSize(int levelOfDetail)
{
    return (uint) 256 << levelOfDetail;
}

void vtkMap::PixelXYToTileXY(int pixelX, int pixelY, int &tileX, int &tileY)
{
    tileX = pixelX / 256;
    tileY = pixelY / 256;
}

std::string vtkMap::TileXYToQuadKey(int tileX, int tileY, int levelOfDetail)
{
    std::stringstream quadKey;
    for (int i = levelOfDetail; i > 0; i--)
    {
        char digit = '0';
        int mask = 1 << (i - 1);
        if ((tileX & mask) != 0)
        {
            digit++;
        }
        if ((tileY & mask) != 0)
        {
            digit++;
            digit++;
        }
        quadKey << digit;
    }
    return quadKey.str();
}

void vtkMap::SetCenter(double *center)
{
    Center[0] = center[0];
    Center[1] = center[1];
    Center[2] = center[2];
}

void vtkMap::SetCenter(double x, double y, double z)
{
    Center[0] = x;
    Center[1] = y;
    Center[2] = z;
}

void vtkMap::DisplayToWorld(double x, double y)
{
    std::cout << "Display Point: " << x << " " << y << std::endl;
    Renderer->SetDisplayPoint(x, y, 0);
    Renderer->DisplayToView();

    double *view = Renderer->GetViewPoint();
    std::cout << "View Point: " << view[0] << " " << view[1] << " " << view[2] << std::endl;

    ViewToWorld(view[0], view[1], view[2]);
    std::cout << "World Points: " << view[0] << " " << view[1] << " " << view[2] << std::endl << std::endl;
}
void vtkMap::ViewToWorld(double &x, double &y, double &z)
{
    double mat[16];
    double result[4];

    // get the perspective transformation from the active camera
    vtkMatrix4x4 *matrix = Renderer->GetActiveCamera()->
            GetCompositeProjectionTransformMatrix(
                Renderer->GetTiledAspectRatio(),0,1);

    // use the inverse matrix
    vtkMatrix4x4::Invert(*matrix->Element, mat);

    // Transform point to world coordinates
    result[0] = x;
    result[1] = y;
    result[2] = z;
    result[3] = 1.0;

    vtkMatrix4x4::MultiplyPoint(mat,result,result);

    // Get the transformed vector & set WorldPoint
    // while we are at it try to keep w at one
    if (result[3])
    {
        x = result[0] / result[3];
        y = result[1] / result[3];
        z = result[2] / result[3];
    }
}
