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
// GridAllocateChannel allocates a new Channel in Grid with a required name
// and optional layout.
//
static PyObject*
PyGridAllocateChannel(PyGrid* self, PyObject* args, PyObject* kwargs)
{
  PyObject* name = NULL;
  PyObject* layout = NULL;

  // note: name, layout are borrowed references
  static const char* kwlist[] = { "name", "layout", NULL };
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O",
                                  (char**) kwlist, &name, &layout))
  {
    PyErr_SetString(PyExc_AttributeError,
                    "Invalid arguments");
    return NULL;
  }

  auto name_utf8 = PyUnicode_AsUTF8(name);
  if (strlen(name_utf8) == 0)
  {
    PyErr_SetString(PyExc_AttributeError,
                    "Invalid name for the channel");
    return NULL;
  }

  auto channel = self->grid->AllocateChannel(name_utf8);
  if (!channel)
  {
    PyErr_SetString(PyExc_AttributeError,
                    "Channel with that name already exists");
    return NULL;
  }

  PyChannel* pychannel = (PyChannel*) PyType_GenericAlloc(&pychannel_type, 0);
  if (pychannel == NULL)
  {
    self->grid->RemoveChannel(channel);
    return PyErr_NoMemory();
  }

  Py_INCREF(self);
  pychannel->grid = self;
  Py_INCREF(name);
  pychannel->name = name;
  pychannel->channel = *channel;

  if (layout != NULL && !PyChannelCompile(pychannel, layout))
  {
    self->grid->RemoveChannel(channel);
    Py_DECREF(pychannel);
    // error set in PyChannelCompile
    return NULL;
  }

  // pass on ownership
  return (PyObject*) pychannel;
}


//
// GridGetChannels returns all Channels in the Grid.
//
static PyObject*
GridGetChannels(PyGrid* self, PyObject* args)
{
  PyObject* list = PyList_New(0);
  if (list == NULL)
    return NULL;

  grid::Registry<grid::Channel>& channels = self->grid->GetChannels();
  for (auto chan_it = channels.Begin(); chan_it != channels.End(); ++chan_it)
  {
    PyChannel* pychannel = (PyChannel*) PyType_GenericAlloc(&pychannel_type, 0);

    Py_INCREF(self);
    pychannel->grid = self;
    pychannel->channel = *chan_it;

    PyObject* name = PyUnicode_FromString(chan_it.Key().c_str());
    Py_INCREF(name);
    pychannel->name = name;

    if (PyList_Append(list, (PyObject*) pychannel) != 0)
    {
      Py_DECREF(list);
      return NULL;
    }
  }

  return list;
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


static PyMethodDef pygrid_methods[] =
{
  {
    "allocate_channel",
    (PyCFunction) PyGridAllocateChannel,
    METH_VARARGS | METH_KEYWORDS,
    "Allocate a new channel and add it to the grid"
  },
  {
    "channels",
    (PyCFunction) GridGetChannels,
    METH_NOARGS,
    "Return all channels in the Grid"
  },
  {
    NULL  /* Sentinel */
  }
};


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
  .tp_methods = pygrid_methods,
  .tp_members = pygrid_members,
  .tp_init = (initproc) PyGridInit,
  .tp_new = PyType_GenericNew,
};


} // end of extern "C"
