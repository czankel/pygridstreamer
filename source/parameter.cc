//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//

#include "gridmodule.h"

#include <grid/fw/parameter.h>

#include <Python.h>

extern "C" {


//
// PyParameterStr implements __str__ and returns the registered name of the
// Parameter.
//
static PyObject* PyParameterStr(PyParameter* self)
{
  PyObject* name = self->name;
  Py_XINCREF(name);
  return name;
}


//
// PyParameterInit implements __init__, which just returns an error.
//
static int
PyParameterInit(PyParameter* self, PyObject* args, PyObject* kwargs)
{
  PyErr_SetString(PyExc_TypeError,
                  "Parameters can only be created using the Channel API.");
  return -1;
}


//
// PyParameterDealloc is the deallocator
//
static void PyParameterDealloc(PyParameter* self)
{
  Py_XDECREF(self->name);
  self->parameter.reset();
  Py_TYPE(self)->tp_free((PyObject*) self);
}


//
// PyParameterFormat returns the format string of the parameter.
//
static PyObject* PyParameterFormatGet(PyParameter* self)
{
  return PyUnicode_FromString(self->parameter->GetFormat().c_str());
}


//
// PyParameterValueSet sets the parameter value
//
static int PyParameterValueSet(PyParameter* self, PyObject* args)
{
  auto param = self->parameter;
  if (param == nullptr)
  {
    PyErr_SetString(PyExc_TypeError, "Not a parameter");
    return -1;
  }
 
  const unsigned long* traits = param->GetSignature();
  if (PyUnicode_Check(args) && traits[0] > 1)
  {
    Py_ssize_t len;
    const char* str = PyUnicode_AsUTF8AndSize(args, &len);
    
    if (!param->Scan(std::string(str, len)))
    {
      PyErr_SetString(PyExc_TypeError, "Invalid format in argument");
      return -1;
    }
    return 0;
  }

  size_t arg_buf_sz = param->GetArgumentBufferSize();
  char* arg_buf[arg_buf_sz];

  if (!PyGridStreamerWriteArguments(args, arg_buf, arg_buf_sz, traits))
    return -1;

  return param->CallUnsafe(NULL, 0, arg_buf, arg_buf_sz)? 0 : -1;
}


//
// PyParameterValueGet returns the parameters as a dictionary
//
static PyObject* PyParameterValueGet(PyParameter* self)
{
  auto param = self->parameter;
  if (param == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "Invalid argument, not a parameter");
    return NULL;
  }

  size_t arg_buf_sz = param->GetArgumentBufferSize();
  char arg_buf[arg_buf_sz];
  if (!param->GetValues(arg_buf, arg_buf_sz))
  {
    PyErr_SetString(PyExc_TypeError, "Failed to get parameter values");
    return NULL;
  }

  return PyGridStreamerReadArguments(arg_buf, arg_buf_sz, param->GetSignature());
}


//
// Define parameter attributes
//
static PyGetSetDef pyparameter_getsets[] =
{
  {
    "format",
    (getter) PyParameterFormatGet,
    (setter) NULL,
    NULL,
    NULL
  },
  {
    "value",
    (getter) PyParameterValueGet,
    (setter) PyParameterValueSet,
    NULL,
    NULL
  },
  {
    NULL  /* Sentinel */
  }
};


//
// Define the PyParameter type
//
PyTypeObject pyparameter_type =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "gridstreamer.Parameter",
  .tp_basicsize = sizeof(PyParameter),
  .tp_itemsize = 0,
  .tp_dealloc = (destructor) PyParameterDealloc,
  .tp_repr = (reprfunc) PyParameterStr,
  .tp_str = (reprfunc) PyParameterStr,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_doc = PyDoc_STR(
      "Parameter describe a generic parameter for Grid types"),
  .tp_getset = pyparameter_getsets,
  .tp_init = (initproc) PyParameterInit,
  .tp_new = PyType_GenericNew,
};


} // end of extern "C"
