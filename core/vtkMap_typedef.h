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

enum class Interaction : unsigned short
{
  Default = 0, // standard map interaction (select/pan)
  RubberBandSelection,
  RubberBandZoom,
  RubberBandDisplayOnly,
  PolygonSelection
};

enum class Shape : unsigned short
{
  TRIANGLE = 0,
  SQUARE,
  PENTAGON,
  HEXAGON,
  OCTAGON,
  TEARDROP,
};
}
#endif // __vtkMap_typedef_h
