#ifndef __vtkMap_h
#define __vtkMap_h

#include <vtkObject.h>

class vtkRenderer;

class vtkMap : public vtkObject
{
public:
    static vtkMap *New();
    virtual void PrintSelf(ostream &os, vtkIndent indent);

    vtkGetMacro(Renderer, vtkRenderer *)
    vtkSetMacro(Renderer, vtkRenderer *)

    vtkGetMacro(Zoom, int)
    vtkSetMacro(Zoom, int)

    vtkGetMacro(latitude, double)
    vtkSetMacro(latitude, double)

    vtkGetMacro(longitude, double)
    vtkSetMacro(longitude, double)

    void SetCenter(double *center);
    void SetCenter(double x, double y, double z);

    void Update();

    static void func(vtkObject *caller, unsigned long eid, void* clientdata, void *calldata);

protected:
    vtkMap();
    ~vtkMap();

    void ViewToWorld(double &x, double &y, double &z);
    void DisplayToWorld(double x, double y);
    void AddTiles();
    void RemoveTiles();

    std::string TileXYToQuadKey(int tileX, int tileY, int levelOfDetail);
    void PixelXYToTileXY(int pixelX, int pixelY, int &tileX, int &tileY);
    void LatLongToPixelXY(double latitude, double longitude, int levelOfDetail, int &pixelX, int &pixelY);
    double Clip(double n, double minValue, double maxValue);
    uint MapSize(int levelOfDetail);

    static const double EarthRadius = 6378137;
    static const double MinLatitude = -85.05112878;
    static const double MaxLatitude = 85.05112878;
    static const double MinLongitude = -180;
    static const double MaxLongitude = 180;

    vtkRenderer *Renderer;
    int Zoom;
    double Center[3];
    double latitude;
    double longitude;

private:
    vtkMap(const vtkMap&);  //Not implemented
    void operator=(const vtkMap&); //Not implemented
};

#endif // __vtkMap_h
