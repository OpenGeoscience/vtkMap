#ifndef __vtkMapTile_h
#define __vtkMapTile_h

#include "vtkObject.h"

class vtkStdString;
class vtkPlaneSource;
class vtkActor;
class vtkPolyDataMapper;
class vtkTextureMapToPlane;

class vtkMapTile : public vtkObject
{
public:
  static vtkMapTile *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMapTile, vtkObject)

  void SetQuadKey(const char *in) {strcpy(QuadKey, in);}
  char *GetQuadKey() {return this->QuadKey;}

  vtkGetMacro(Plane, vtkPlaneSource *)
  vtkGetMacro(Actor, vtkActor *)
  vtkGetMacro(Mapper, vtkPolyDataMapper *)

  void SetCenter(double *center);
  void SetCenter(double x, double y, double z);
  double *GetCenter();
  void GetCenter(double &x, double &y, double &z);

  void init();

protected:
  vtkMapTile();
  ~vtkMapTile();

  bool IsTextureDownloaded(const char *outfile);
  void DownloadTexture(const char *url, const char *outfilename);

  void InitialiseTile();
  void InitialiseTexture();

  char QuadKey[30];
  char *outfilename;
  vtkPlaneSource *Plane;
  vtkTextureMapToPlane *texturePlane;
  vtkActor *Actor;
  vtkPolyDataMapper *Mapper;
  double Center[3];

private:
  vtkMapTile(const vtkMapTile&);  //Not implemented
  void operator=(const vtkMapTile&); //Not implemented
};

#endif // __vtkMapTile_h
