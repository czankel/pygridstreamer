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
// PyChannelSetState sets the state of the channel.
//
static int PyChannelSetState(PyChannel* self, PyObject* pystate)
{
  const char* state = PyUnicode_AsUTF8AndSize(pystate, NULL);
  if (state == NULL)
    return -1;

  grid::State next_state = grid::kStateInvalid;
  if (!strcmp(state, "null"))
    next_state = grid::kStateNull;
  else if (!strcmp(state, "ready"))
    next_state = grid::kStateReady;
  else if (!strcmp(state, "set"))
    next_state = grid::kStateSet;
  else if (!strcmp(state, "flushing"))
    next_state = grid::kStateFlushing;
  else if (!strcmp(state, "running"))
    next_state = grid::kStateRunning;
  else if (!strcmp(state, "paused"))
    next_state = grid::kStatePaused;

  if (next_state != grid::kStateInvalid && !self->channel->SetState(next_state))
    return -1;

  return 0;
}


//
// PyChannelOpen initializes and opens  the channel and sets the state to
// "set" unless the state is already higher.
//
static PyObject* PyChannelOpen(PyChannel* self)
{
  bool ret = true;

  grid::State curr_state = self->channel->GetState();
  if (curr_state < grid::kStateSet)
    ret = self->channel->SetStateCond(curr_state, grid::kStateSet);

  return PyBool_FromLong(ret);
}


//
// PyChannelClose closes the channel.
//
static PyObject* PyChannelClose(PyChannel* self)
{
  return PyBool_FromLong(self->channel->SetState(grid::kStateNull));
}


//
// PyChannelRun runs the channel (initializes it if not already initialized).
//
static PyObject* PyChannelRun(PyChannel* self)
{
  return PyBool_FromLong(self->channel->SetState(grid::kStateRunning));
}


//
// PyChannelPause pauses a running channel (initializes and opens it if not
// already in at least the state 'set')
//
static PyObject* PyChannelPause(PyChannel* self)
{
  return PyBool_FromLong(self->channel->SetState(grid::kStatePaused));
}


//
// PyChannelFlush flushes a channel if running or initializes the channel if
// uninitialized.
//
static PyObject* PyChannelFlush(PyChannel* self)
{
  return PyBool_FromLong(self->channel->SetState(grid::kStateFlushing));
}


//
// PyChannelStop stops a channel unless it hasn't been anitialized.
// Stopping a channel will drop any outstanding data.
//
static PyObject* PyChannelStop(PyChannel* self)
{
  bool ret = true;

  grid::State curr_state = self->channel->GetState();
  if (curr_state >= grid::kStateSet)
    ret = self->channel->SetStateCond(curr_state, grid::kStateSet);

  return PyBool_FromLong(ret);
}


//
// PyChannelGetState returns the state of the channel.
//
static PyObject* PyChannelGetState(PyChannel* self)
{
  grid::State state = self->channel->GetState();

  if (state == grid::kStateInvalid)
    return PyUnicode_FromString("invalid");
  else if (state == grid::kStateNull)
    return PyUnicode_FromString("null");
  else if (state == grid::kStateReady)
    return PyUnicode_FromString("ready");
  else if (state == grid::kStateSet)
    return PyUnicode_FromString("set");
  else if (state == grid::kStateFlushing)
    return PyUnicode_FromString("flushing");
  else if (state == grid::kStateRunning)
    return PyUnicode_FromString("running");
  else if (state == grid::kStatePaused)
    return PyUnicode_FromString("paused");
  else if (state == grid::kStateEnd)
    return PyUnicode_FromString("end");
  else if (state == grid::kStateError)
    return PyUnicode_FromString("error");

  return PyUnicode_FromString("unknown");
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


//
// Define PyChannel attributes
//
static PyGetSetDef pychannel_getsets[] =
{
  {
    "state",
    (getter) PyChannelGetState,
    (setter) PyChannelSetState,
    NULL,
    NULL
  },
  {
    NULL
  }
};


//
// Define the PyChannel methods
static PyMethodDef pychannel_methods[] =
{
  {
    "cells",
    (PyCFunction) PyChannelGetCells,
    METH_NOARGS,
    "Return all pipeline cells in the channel"
  },
  {
    "open",
    (PyCFunction) PyChannelOpen,
    METH_NOARGS,
    "Open the channel and set state to Set",
  },
  {
    "close",
    (PyCFunction) PyChannelClose,
    METH_NOARGS,
    "Close the channel and set state to Set",
  },
  {
    "run",
    (PyCFunction) PyChannelRun,
    METH_NOARGS,
    "Run the channel and set the state to Running",
  },
  {
    "pause",
    (PyCFunction) PyChannelPause,
    METH_NOARGS,
    "Pause the channel",
  },
  {
    "flush",
    (PyCFunction) PyChannelFlush,
    METH_NOARGS,
    "Flush the channel",
  },
  {
    "stop",
    (PyCFunction) PyChannelStop,
    METH_NOARGS,
    "Stop the channel and drop any outstanding transports",
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
  .tp_getset = pychannel_getsets,
  .tp_init = (initproc) PyChannelInit,
  .tp_new = PyType_GenericNew,
};


} // end of extern "C"
