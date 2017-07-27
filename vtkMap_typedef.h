#ifndef __vtkMap_typedef_h
#define __vtkMap_typedef_h
#include <vtkCommand.h>

namespace vtkMapType {

enum class Move : unsigned short
{
  UP = 0,
  DOWN,
  TOP,
  BOTTOM
};

enum Interaction : unsigned short
{
  DisabledMode = 0,   // standard map interaction (select/pan)
  SelectionMode,      ///TODO change to ::DrawBandMode
  ZoomMode,
  DisplayOnlyMode,
  DrawPolyMode
};

enum class Event : unsigned short
{
  SelectionCompleteEvent = vtkCommand::UserEvent + 1,
  DisplayClickCompleteEvent,   // DisplayOnlyMode && mouse click
  DisplayDrawCompleteEvent,    // DisplayOnlyMode && rectangle draw
  ZoomCompleteEvent,
  RightButtonCompleteEvent     // For application-context menus
};

}
#endif // __vtkMap_typedef_h
