"""
"""
from __future__ import print_function

import glob
import os
import sys

import vtk

def load_vtk_file(name):
	print('Loading %s' % name)
	reader = vtk.vtkPolyDataReader()
	reader.SetFileName(name)
	reader.Update()
	return reader.GetOutput()

if __name__ == '__main__':
	# Set up rendering
	ren = vtk.vtkRenderer()
	renWin = vtk.vtkRenderWindow()
	renWin.AddRenderer(ren)
	renWin.SetSize(600, 300)
	iren = vtk.vtkRenderWindowInteractor()
	iren.SetRenderWindow(renWin)
	style = vtk.vtkInteractorStyleTrackballCamera()
	iren.SetInteractorStyle(style)
	ren.SetBackground(0, 0, 0)

	# Gather all .vtk files in this folder
	offset = 0.0
	source_dir = os.path.abspath(os.path.dirname(__file__))
	for filename in os.listdir(source_dir):
		if filename.endswith('.vtk'):
			path = os.path.join(source_dir, filename)
			polydata = load_vtk_file(path)
			mapper = vtk.vtkPolyDataMapper()
			mapper.SetInputData(polydata)
			actor = vtk.vtkActor()
			actor.SetMapper(mapper)
			actor.SetPosition(offset, 0.0, 0.0)
			actor.GetProperty().SetRepresentationToWireframe()
			offset += 1.2
			ren.AddActor(actor)

	renWin.Render()
	iren.Start()
