#include <QApplication>
#include <QFrame>
#include <QPushButton>
#include <QMainWindow>

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkJPEGReader.h>

#include <QVTKWidget.h>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  QMainWindow mainWindow;
  QFrame frame;
  mainWindow.setCentralWidget(&frame);

  QPushButton *button = new QPushButton("Test", &frame);
  button->move(100, 100);

  QVTKWidget widget(&frame);
  widget.resize(256,256);
  widget.move(200, 200);

  // Setup sphere
  vtkSmartPointer<vtkSphereSource> sphereSource =
      vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->Update();
  vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  vtkSmartPointer<vtkActor> sphereActor =
      vtkSmartPointer<vtkActor>::New();
  sphereActor->SetMapper(sphereMapper);

  // Setup window
  vtkSmartPointer<vtkRenderWindow> renderWindow =
      vtkSmartPointer<vtkRenderWindow>::New();

  // Setup renderer
  vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
  renderWindow->AddRenderer(renderer);

  renderer->AddActor(sphereActor);
  renderer->ResetCamera();

  widget.SetRenderWindow(renderWindow);


  mainWindow.show();
  mainWindow.resize(1000, 800);

  app.exec();

  return EXIT_SUCCESS;
}
