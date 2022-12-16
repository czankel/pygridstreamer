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
// PyCellAddParamters is a helper function to add parameters to PyCell.
//
static bool PyCellAddParameters(PyCell* self)
{
  bool ret = true;
  auto& params = self->cell->GetParameters();

  PyObject* dict = PyObject_GenericGetDict((PyObject*)self, NULL);
  for (auto param_it = params.Begin();
       ret && param_it != params.End();
       ++param_it)
  {
    PyParameter* pyparameter =
      (PyParameter*) PyType_GenericAlloc(&pyparameter_type, 0);

    pyparameter->parameter = *param_it;

    std::string key = PythonifyName(param_it.Key());
    pyparameter->name = PyUnicode_FromString(key.c_str());

    ret = PyDict_SetItemString(dict, key.c_str(), (PyObject*) pyparameter) == 0;
  }

  Py_DECREF(dict);
  return ret;
}


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
    PyErr_SetString(PyExc_TypeError, "Cell is not a pipeline or cluster");
    return NULL;
  }

  grid::Registry<grid::Cell>& cells = pipeline != nullptr ?
    pipeline->GetCells() : cluster->GetCells();

  PyObject* dict = PyDict_New();

  for (auto cell_it = cells.Begin(); cell_it != cells.End(); ++cell_it)
  {
    PyCell* pycell = (PyCell*) PyType_GenericAlloc(&pycell_type, 0);
    pycell->cell = *cell_it;

    pycell->name = PyUnicode_FromString(cell_it.Key().c_str());
    pycell->type = PyUnicode_FromString(cell_it->Type().c_str());

    if (!PyCellAddParameters(pycell))
    {
      Py_DECREF(dict);
      return NULL;
    }

    const char* key = cell_it.Key().c_str();
    if (PyDict_SetItemString(dict, key, (PyObject*) pycell) != 0)
    {
      Py_DECREF(dict);
      return NULL;
    }
  }
  return dict;
}


//
// PyCellGetParameters returns a list of all parameters of the cell.
//
static PyObject* PyCellGetParameters(PyCell* self)
{
  auto cell = self->cell;
  if (cell == NULL)
  {
    PyErr_SetString(PyExc_AttributeError, "cell");
    return NULL;
  }

  PyObject* list = PyList_New(0);
  auto& params = cell->GetParameters();
  for (auto param_it = params.Begin(); param_it != params.End(); ++param_it)
    PyList_Append(list, PyUnicode_FromString(param_it.Key().c_str()));

  return list;
}


static PyMethodDef pycell_methods[] =
{
  {
    "type",
    (PyCFunction) PyCellGetType,
    METH_NOARGS,
    "Return the type of the cell, such as pipeline, cluster, or cell"
  },
  {
    "cells",
    (PyCFunction) PyCellGetCells,
    METH_NOARGS,
    "Return the cells of a pipeline or cluster"
  },
  {
    // TODO: move to member?
    "parameters",
    (PyCFunction) PyCellGetParameters,
    METH_NOARGS,
    "Return the parameters of the cell"
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
