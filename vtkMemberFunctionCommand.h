/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMemberFunctionCommand
 * @brief   Call a class member method in response to a
 * VTK event.
 *
 * vtkMemberFunctionCommand is a vtkCommand-derivative that will listen for VTK
 * events, calling a class member function when a VTK event is received.
 *
 * It is generally more useful than vtkCallbackCommand, which can only call
 * non-member functions in response to a VTK event.
 *
 * Usage: create an instance of vtkMemberFunctionCommand, specialized for the
 * class that will receive events.  Use the SetCallback() method to pass the
 * instance and member function that will be called when an event is received.
 * Use vtkObject::AddObserver() to control which VTK events the
 * vtkMemberFunctionCommand object will receive.
 *
 * Usage:
 *
 * vtkObject* subject = ...
 * foo* observer = ...
 * vtkMemberFunctionCommand<foo>* adapter = vtkMemberFunctionCommand<foo>::New();
 * adapter->SetCallback(observer, &foo::bar);
 * subject->AddObserver(vtkCommand::AnyEvent, adapter);
 *
 * Alternative Usage
 *
 * vtkCommand* command = vtkMakeMemberFunctionCommand(*observer, &foo::Callback);
 * subject->AddObserver(vtkCommand::AnyEvent, command);
 *
 * There are two types of callback methods that could be defined.
 * \li void Callback() -- which takes no arguments.
 * \li void Callback(vtkObject* caller, unsigned long event, void* calldata) --
 * which is passed the same arguments as vtkCommand::Execute(), thus making it
 * possible to get additional details about the event.
 * @sa
 * vtkCallbackCommand
*/

#include "vtkCommand.h"

template <class ClassT>
class vtkMemberFunctionCommand : public vtkCommand
{
  typedef vtkMemberFunctionCommand<ClassT> ThisT;

public:
  typedef vtkCommand Superclass;

  virtual const char* GetClassNameInternal() const { return "vtkMemberFunctionCommand"; }

  static ThisT* SafeDownCast(vtkObjectBase* o) { return dynamic_cast<ThisT*>(o); }

  static ThisT* New() { return new ThisT(); }

  void PrintSelf(ostream& os, vtkIndent indent) { vtkCommand::PrintSelf(os, indent); }

  //@{
  /**
   * Set which class instance and member function will be called when a VTK
   * event is received.
   */
  void SetCallback(ClassT& object, void (ClassT::*method)())
  {
    this->Object = &object;
    this->Method = method;
  }
  //@}

  void SetCallback(ClassT& object, void (ClassT::*method2)(vtkObject*, unsigned long, void*))
  {
    this->Object = &object;
    this->Method2 = method2;
  }

  virtual void Execute(vtkObject* caller, unsigned long event, void* calldata)
  {
    if (this->Object && this->Method)
    {
      (this->Object->*this->Method)();
    }
    if (this->Object && this->Method2)
    {
      (this->Object->*this->Method2)(caller, event, calldata);
    }
  }
  void Reset()
  {
    this->Object = 0;
    this->Method2 = 0;
    this->Method = 0;
  }

private:
  vtkMemberFunctionCommand()
  {
    this->Object = 0;
    this->Method = 0;
    this->Method2 = 0;
  }

  ~vtkMemberFunctionCommand() {}

  ClassT* Object;
  void (ClassT::*Method)();
  void (ClassT::*Method2)(vtkObject* caller, unsigned long event, void* calldata);

  vtkMemberFunctionCommand(const vtkMemberFunctionCommand&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMemberFunctionCommand&) VTK_DELETE_FUNCTION;
};

/**
 * Convenience function for creating vtkMemberFunctionCommand instances that
 * automatically deduces its arguments.

 * Usage:

 * vtkObject* subject = ...
 * foo* observer = ...
 * vtkCommand* adapter = vtkMakeMemberFunctionCommand(observer, &foo::bar);
 * subject->AddObserver(vtkCommand::AnyEvent, adapter);

 * See Also:
 * vtkMemberFunctionCommand, vtkCallbackCommand
 */

template <class ClassT>
vtkMemberFunctionCommand<ClassT>* vtkMakeMemberFunctionCommand(
  ClassT& object, void (ClassT::*method)())
{
  vtkMemberFunctionCommand<ClassT>* result = vtkMemberFunctionCommand<ClassT>::New();
  result->SetCallback(object, method);
  return result;
}

template <class ClassT>
vtkMemberFunctionCommand<ClassT>* vtkMakeMemberFunctionCommand(
  ClassT& object, void (ClassT::*method)(vtkObject*, unsigned long, void*))
{
  vtkMemberFunctionCommand<ClassT>* result = vtkMemberFunctionCommand<ClassT>::New();
  result->SetCallback(object, method);
  return result;
}
//-----------------------------------------------------------------------------
// VTK-HeaderTest-Exclude: vtkMemberFunctionCommand.h
