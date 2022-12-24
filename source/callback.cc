//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//

#include "gridmodule.h"

#include <grid/fw/callback.h>
#include <grid/util/function.h>

#include <Python.h>

#include <algorithm>
#include <cstdarg>

extern "C" {


//
// PyCallbackStr implements __str__ and returns the registered name of the
// Callback.
//
static PyObject* PyCallbackStr(PyCallback* self)
{
  PyObject* name = self->name;
  Py_XINCREF(name);
  return name;
}


//
// PyCallbackInit implements __init__, which just returns an error.
//
static int
PyCallbackInit(PyCallback* self)
{
  PyErr_SetString(PyExc_TypeError,
                  "Callbacks can only be created using the Channel API.");
  return -1;
}


//
// PyCallbackDealloc is the deallocator
//
static void PyCallbackDealloc(PyCallback* self)
{
  Py_XDECREF(self->name);
  self->callback.reset();
  Py_TYPE(self)->tp_free((PyObject*) self);
}


//
// OnClose is the registered callback when a callback is closed.
// It releases all functions and sets the PyCallback inactive.
//
// TODO: support adding 'close' functions
static void OnClose(const grid::Slot& slot, uintptr_t context)
{
  PyCallback* self = (PyCallback*) context;
  self->callback.reset();
  self->active = false;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  for (std::list<PyObject*>::iterator it = self->functions.begin();
       it != self->functions.end();
       ++it)
    Py_DECREF(*it);

  self->functions.clear();
  PyGILState_Release(gstate);
}


//
// OnCallback is the registered callback function that handles all registered
// python callbacks.
//
// GIL: https://docs.python.org/3/c-api/init.html#releasing-the-gil-from-extension-code


void OnCallback(uintptr_t context...)
{
  va_list args;
  va_start(args, context);

  PyCallback* self = (PyCallback*) context;

  auto cb = self->callback;
  const unsigned long* traits = cb->Signature();

  // -- start of Python GIL --

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  PyObject* tuple = PyTuple_New(traits[0]);

  for (size_t i = 1; i <= traits[0]; i++)
  {
    PyObject* item = NULL;

    unsigned long trait = traits[i];
    unsigned int count = trait >> grid::kCountShift;
    size_t size = 1 << (trait & grid::kSizeMask);

    // support only char arrays
    if (count > 1)
    {
      unsigned long t =  (trait & ~grid::kCountMask) | (1 << grid::kCountShift);
      if (t == grid::TypeT<uint8_t>::Sig)
        item = PyUnicode_FromStringAndSize(va_arg(args, char*), size);
      else
        PyErr_SetString(PyExc_TypeError,
            "Genereric arrays not supported as parameters.");
    }
    else
    {
      switch (traits[i])
      {
        case grid::TypeT<uint8_t>::Sig:
          item = PyLong_FromUnsignedLong(va_arg(args, unsigned int)); break;
        case grid::TypeT<uint16_t>::Sig:
          item = PyLong_FromUnsignedLong(va_arg(args, unsigned int)); break;
        case grid::TypeT<uint32_t>::Sig:
          item = PyLong_FromUnsignedLong(va_arg(args, unsigned int)); break;
        case grid::TypeT<uint64_t>::Sig:
          item = PyLong_FromUnsignedLongLong(va_arg(args, uint64_t)); break;
        case grid::TypeT<int8_t>::Sig:
          item = PyLong_FromLong(va_arg(args, int)); break;
        case grid::TypeT<int16_t>::Sig:
          item = PyLong_FromLong(va_arg(args, int)); break;
        case grid::TypeT<int32_t>::Sig:
          item = PyLong_FromLong(va_arg(args, int)); break;
        case grid::TypeT<int64_t>::Sig:
          item = PyLong_FromLongLong(va_arg(args, int64_t)); break;
        case grid::TypeT<bool>::Sig:
          item = PyBool_FromLong(va_arg(args, int)); break;
        case grid::TypeT<float>::Sig:
          item = PyFloat_FromDouble(va_arg(args, double)); break;
        case grid::TypeT<double>::Sig:
          item = PyFloat_FromDouble(va_arg(args, double)); break;
        case grid::TypeT<long double>::Sig:
          item = PyFloat_FromDouble(va_arg(args, double)); break;
#if 0
        case grid::TypeT<std::string&>::Sig:
          item = PyUnicode_FromString(va_arg(args, void*).c_str()); break;
#endif
        default: break;
      }
    }

    if (item == NULL)
      goto out;

    PyTuple_SET_ITEM(tuple, i - 1, item);
  }

  for (auto& func : self->functions)
    if (PyObject_CallObject(func, tuple))
      PyErr_Print();

out:
  Py_XDECREF(tuple);

  PyGILState_Release(gstate);

  // -- end of Python GIL --

  va_end(args);
}


//
// PyCallbackConnect sets the callback function.
//
static PyObject* PyCallbackConnect(PyCallback* self, PyObject* func)
{
  if (!PyCallable_Check(func))
  {
    PyErr_SetString(PyExc_AttributeError, "Invalid arguments");
    return NULL;
  }

  if (!self->active)
  {
    PyErr_SetString(PyExc_AttributeError, "Callback closed");
    return NULL;
  }

  // TODO: move to initialization
  if (self->slot == nullptr)
    self->slot = self->callback->Connect(OnCallback, OnClose, (uintptr_t)self);

  Py_INCREF(func);
  self->functions.push_back(func);

  Py_RETURN_TRUE;
}


//
// PyCallbackDisconnect disconnects the specified :
//

static int PyCallbackDisconnect(PyCallback* self, PyObject* func)
{
  if (!PyCallable_Check(func))
  {
    PyErr_SetString(PyExc_AttributeError, "Invalid arguments");
    return 0;
  }

  auto funcs = self->functions;
  auto it = std::find(funcs.begin(), funcs.end(), func);
  if (it != funcs.end())
  {
    Py_DECREF(*it);
    funcs.erase(it);
  }
  else
  {
    PyErr_SetString(PyExc_AttributeError,"function not registered");
    return 0;
  }

  return 1;
}


// TODO: Callback doesn't store the value
#if 0
//
// PyCallbackValueGet returns the most recent callback values as a tuple
//
static PyObject* PyCallbackValueGet(PyCallback* self)
{
  auto cb = self->callback;
  if (cb == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "Invalid argument, not a callback");
    return NULL;
  }

  size_t arg_buf_sz = cb->ArgumentBufferSize();
  char arg_buf[arg_buf_sz];
  if (!cb->GetValues(arg_buf, arg_buf_sz))
  {
    PyErr_SetString(PyExc_TypeError, "Failed to get callback values");
    return NULL;
  }

  return PyGridStreamerReadArguments(arg_buf, arg_buf_sz, cb->Signature());
}


//
// Define callback attributes
//
static PyGetSetDef pycallback_getsets[] =
{
  {
    "value",
    (getter) PyCallbackValueGet,
    (setter) NULL,
    NULL,
    NULL
  },
  {
    NULL  /* Sentinel */
  }
};
#endif


static PyMethodDef pycallback_methods[] =
{
  {
    "connect",
    (PyCFunction) PyCallbackConnect,
    METH_O,
    "Connect a function to the callback",
  },
  {
    "disconnect",
    (PyCFunction) PyCallbackDisconnect,
    METH_O,
    "Disconnect a function to the callback",
  },
  {
    NULL
  }
}; 


//
// Define the PyCallback type
//
PyTypeObject pycallback_type =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "gridstreamer.Callback",
  .tp_basicsize = sizeof(PyCallback),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor) PyCallbackDealloc,
  .tp_repr = (reprfunc) PyCallbackStr,
  .tp_str = (reprfunc) PyCallbackStr,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_doc = PyDoc_STR("Callback describe a callback"),
#if 0
  .tp_getset = pycallback_getsets,
#endif
  .tp_methods = pycallback_methods,
  .tp_init = (initproc) PyCallbackInit,
  .tp_new = PyType_GenericNew,
};


} // end of extern "C"
