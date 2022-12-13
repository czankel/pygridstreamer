//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//

#include "gridmodule.h"

#include <grid/fw/cluster.h>
#include <grid/fw/grid.h>
#include <grid/fw/pipeline.h>

#include <Python.h>

extern "C" {

//
// PyCellStr implements __str__ and returns the registered name of the Cell.
//
static PyObject* PyCellStr(PyCell* self)
{
  PyObject* type = self->type;
  Py_XINCREF(type);
  return type;
}


//
// PyCellInit implements __init__, which just returns an error.
//
static int
PyCellInit(PyCell* self, PyObject* args, PyObject* kwargs)
{
  PyErr_SetString(PyExc_TypeError,
                  "Cells can only be created using the Channel API.");
  return -1;
}


//
// PyCellDealloc is the deallocator
//
static void PyCellDealloc(PyCell* self)
{
  Py_XDECREF(self->name);
  self->cell.reset();
  Py_TYPE(self)->tp_free((PyObject*) self);
}


//
// PyCellGetType returns the type of the cell, if it's a pipeline, cluster,
// or cell
//
PyObject* PyCellGetType(PyCell* self)
{
  auto cell = self->cell;
  if (cell == NULL)
  {
    PyErr_SetString(PyExc_AttributeError, "cell");
    return NULL;
  }

  if (cell->PipelineInterface() != nullptr)
    return PyUnicode_FromString("pipeline");
  else if (cell->ClusterInterface() != nullptr)
    return PyUnicode_FromString("cluster");
  return PyUnicode_FromString("cell");
}


//
// PyCellGetCells returns a dictionary of cells in the (pipeline/cluster) cell
// These could be pipelines or cells.
//
PyObject* PyCellGetCells(PyCell* self)
{
  auto cell = self->cell;
  if (cell == NULL)
  {
    PyErr_SetString(PyExc_AttributeError, "cell");
    return NULL;
  }

  grid::Cluster*  cluster =  cell->ClusterInterface();
  grid::Pipeline* pipeline = cell->PipelineInterface();
  if (cluster == nullptr && pipeline == nullptr)
  {
    PyErr_SetString(PyExc_AttributeError, "Cell is not a pipeline or cluster");
    return NULL;
  }

  grid::Registry<grid::Cell>& cells = pipeline != nullptr ?
    pipeline->GetCells() : cluster->GetCells();

  PyObject* dict = PyDict_New();

  for (auto cell_it = cells.Begin(); cell_it != cells.End(); ++cell_it)
  {
    PyCell* pycell = (PyCell*) PyType_GenericAlloc(&pycell_type, 0);

    Py_INCREF(self);
    pycell->cell = *cell_it;

    PyObject* name = PyUnicode_FromString(cell_it.Key().c_str());
    Py_INCREF(name);
    pycell->name = name;

    PyObject* type = PyUnicode_FromString(cell_it->Type().c_str());
    Py_INCREF(type);
    pycell->type = type;

    printf("ITEM %s %s\n", cell_it.Key().c_str(), cell_it->Type().c_str());

    if (PyDict_SetItemString(dict, cell_it.Key().c_str(), (PyObject*) pycell) != 0)
    {
      Py_DECREF(dict);
      return NULL;
    }
  }

  return dict;
}


static PyMethodDef pycell_methods[] =
{
  {
    "type",
    (PyCFunction) PyCellGetType,
    METH_NOARGS,
    "return the type of the cell, such as pipeline, cluster, or cell"
  },
  {
    "cells",
    (PyCFunction) PyCellGetCells,
    METH_NOARGS,
    "return the cells of a pipeline or cluster"
  },
  {
    NULL  /* Sentinel */
  }
};



PyTypeObject pycell_type =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "gridstreamer.Cell",
  .tp_basicsize = sizeof(PyCell),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor) PyCellDealloc,
  .tp_repr = (reprfunc) PyCellStr,
  .tp_str = (reprfunc) PyCellStr,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dictoffset = offsetof(PyCell,dict),
  .tp_doc = PyDoc_STR(
      "Cell is the basic unit describing a Cell, Cluster, or Pipeline"),
  .tp_methods = pycell_methods,
  .tp_init = (initproc) PyCellInit,
  .tp_new = PyType_GenericNew,
};


} // end of extern "C"
