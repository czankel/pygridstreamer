//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//

#include "gridmodule.h"

#include <iostream>

#include <grid/base/basegrid.h>
#include <grid/fw/grid.h>

#include <Python.h>
#include <structmember.h>

extern "C" {

//
// PyGridStr implements __str__ and returns any given name of the grid
//
static PyObject* PyGridStr(PyGrid* self)
{
  PyObject* name = self->name;
  Py_XINCREF(name);
  return name;
}


//
// PyGridInit implements __init__
//
static int PyGridInit(PyGrid* self, PyObject* args, PyObject* kwargs)
{
  PyObject* name;

  // note: name is a borrowed references
  static const char* kwlist[] = { "name", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char**) kwlist, &name))
    return -1;

  Py_XINCREF(name);
  self->name = name;
  self->grid = std::make_shared<grid::BaseGrid>();
  return 0;
}


//
// PyGridDealloc is the deallocator
//
static void PyGridDealloc(PyGrid* self)
{
  Py_XDECREF(self->name);
  self->~PyGrid();
  Py_TYPE(self)->tp_free((PyObject*) self);
}


static PyMemberDef pygrid_members[] =
{
  {
    "name",
    T_OBJECT_EX, offsetof(PyGrid, name),
    0,
    "name for the grid instance"
  },
  {
    NULL  /* Sentinel */
  }
};


PyTypeObject pygrid_type =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "gridstreamer.Grid",
  .tp_basicsize = sizeof(PyGrid),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor) PyGridDealloc,
  .tp_str = (reprfunc) PyGridStr,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_doc = PyDoc_STR(
      "Grid provides the base for encapsulating the streaming network"),
  .tp_members = pygrid_members,
  .tp_init = (initproc) PyGridInit,
  .tp_new = PyType_GenericNew,
};


} // end of extern "C"
