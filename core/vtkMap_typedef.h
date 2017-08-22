#ifndef __vtkMap_typedef_h
#define __vtkMap_typedef_h

namespace vtkMapType
{

enum class Move : unsigned short
{
  UP = 0,
  DOWN,
  TOP,
  BOTTOM
};

enum Interaction : unsigned short
{
  Default = 0, // standard map interaction (select/pan)
  RubberBandSelection,
  RubberBandZoom,
  RubberBandDisplayOnly,
  PolygonSelection
};
}
#endif // __vtkMap_typedef_h
