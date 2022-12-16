//
// opyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//

#include "gridmodule.h"

#include <grid/builder/builder.h>
#include <grid/fw/pipeline.h>

#include <Python.h>

extern "C" {

//
// PyChannelCompile compiles a new layout to the channel releasing any current
// layout.
//
PyObject* PyChannelCompile(PyChannel* self, PyObject* pylayout)
{
  auto& channel = self->channel;
  if (channel == NULL)
  {
    PyErr_SetString(PyExc_AttributeError, "channel");
    Py_RETURN_FALSE;
  }

  grid::Builder builder;
  std::string err;
  std::unique_ptr<grid::Layout> layout =
    builder.Compile(PyUnicode_AsUTF8(pylayout), err);
  if (layout == NULL)
  {
    PyErr_SetString(PyExc_AttributeError, err.c_str());
    Py_RETURN_FALSE;
  }

  channel->CreateLayout();

  PyGrid* grid = (PyGrid*)self->grid;
  if (!builder.UpdateChannel(*grid->grid, *channel, *layout))
  {
    // TODO: get error text from builder (not implemented yet)
    PyErr_SetString(PyExc_SyntaxError, "layout format");
    channel->AbortLayout();
    Py_RETURN_FALSE;
  }

  if (!channel->CommitLayout())
  {
    channel->AbortLayout();
    Py_RETURN_FALSE;
  }

  Py_RETURN_TRUE;
}


//
// PyChannelGetCells returns a list of cells in the channel.
// These could be pipelines or cells.
//
static PyObject* PyChannelGetCells(PyChannel* self)
{
  auto channel = self->channel;
  if (channel == NULL)
  {
    PyErr_SetString(PyExc_AttributeError, "channel");
    return NULL;
  }

  PyObject* dict = PyDict_New();

  grid::Registry<grid::Pipeline>& pipelines = channel->GetPipelines();
  for (auto pipe_it = pipelines.Begin(); pipe_it != pipelines.End(); ++pipe_it)
  {
    PyCell* pycell = (PyCell*) PyType_GenericAlloc(&pycell_type, 0);
    pycell->cell = *pipe_it;

    pycell->name = PyUnicode_FromString(pipe_it.Key().c_str());
    pycell->type = PyUnicode_FromString(pipe_it->Type().c_str());

    if (PyDict_SetItemString(dict, pipe_it.Key().c_str(), (PyObject*) pycell) != 0)
    {
      Py_DECREF(dict);
      return NULL;
    }
  }

  return dict;
}


//
// PyChannelStr implements __str__ and returns the registered name of
// the Channel
//
static PyObject* PyChannelStr(PyChannel* self)
{
  PyObject* name = self->name;
  Py_XINCREF(name);
  return name;
}


//
// PyChannelInit implements __init__; which just returns an error.
//
static int
PyChannelInit(PyChannel* self, PyObject* args, PyObject* kwargs)
{
  PyErr_SetString(PyExc_TypeError,
                  "Channels can only be created using the Grid API.");
  return -1;
}


//
// PyChannelDealloc is the deallocator
//
static void PyChannelDealloc(PyChannel* self)
{
  Py_XDECREF(self->name);
  self->channel.reset();
  Py_TYPE(self)->tp_free((PyObject*) self);
}


static PyMethodDef pychannel_methods[] =
{
  {
    "cells",
    (PyCFunction) PyChannelGetCells,
    METH_NOARGS,
    "Return all pipeline cells in the channel"
  },
  {
    NULL  /* Sentinel */
  }
};


PyTypeObject pychannel_type =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "gridstreamer.Channel",
  .tp_basicsize = sizeof(PyChannel),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor) PyChannelDealloc,
  .tp_repr = (reprfunc) PyChannelStr,
  .tp_str = (reprfunc) PyChannelStr,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_doc = PyDoc_STR("Channel is a contained system of pipelines and streams"),
  .tp_methods = pychannel_methods,
  .tp_init = (initproc) PyChannelInit,
  .tp_new = PyType_GenericNew,
};


} // end of extern "C"
