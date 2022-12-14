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


// Helper function to read arguments from an argument buffer into a python tuple
static PyObject*
ReadArguments(uintptr_t args_ptr, size_t args_sz, const unsigned long* traits)
{
  PyObject* tuple = PyTuple_New(traits[0]);

  for (size_t i = 1; i <= traits[0]; i++)
  {
    PyObject* item = NULL;

    unsigned long trait = traits[i];
    unsigned int count = trait >> grid::kCountShift;
    size_t size = 1 << (trait & grid::kSizeMask);
    size_t align = 1 << ((trait & grid::kAlignMask) >> grid::kAlignShift);
    args_ptr = (args_ptr + align - 1) & -align;

    // support only char arrays
    if (count > 1)
    {
      if (((trait & ~grid::kCountMask) | (1 << grid::kCountShift))
            == grid::TypeT<uint8_t>::Sig)
        item = PyUnicode_FromStringAndSize((const char*)args_ptr, size);
      else
        PyErr_SetString(PyExc_TypeError,
            "Genereric arrays not supported as parameters.");
    }
    else
    {
      switch (traits[i])
      {
        case grid::TypeT<uint8_t>::Sig:
          item = PyLong_FromUnsignedLong(*(uint8_t*)args_ptr); break;
        case grid::TypeT<uint16_t>::Sig:
          item = PyLong_FromUnsignedLong(*(uint16_t*)args_ptr); break;
        case grid::TypeT<uint32_t>::Sig:
          item = PyLong_FromUnsignedLong(*(uint32_t*)args_ptr); break;
        case grid::TypeT<uint64_t>::Sig:
          item = PyLong_FromUnsignedLongLong(*(uint64_t*)args_ptr); break;
        case grid::TypeT<int8_t>::Sig:
          item = PyLong_FromLong(*(int8_t*)args_ptr); break;
        case grid::TypeT<int16_t>::Sig:
          item = PyLong_FromLong(*(int16_t*)args_ptr); break;
        case grid::TypeT<int32_t>::Sig:
          item = PyLong_FromLong(*(int32_t*)args_ptr); break;
        case grid::TypeT<int64_t>::Sig:
          item = PyLong_FromLongLong(*(int64_t*)args_ptr); break;
        case grid::TypeT<bool>::Sig:
          item = PyBool_FromLong(*(bool*)args_ptr != 0); break;
        case grid::TypeT<float>::Sig:
          item = PyFloat_FromDouble(*(float*)args_ptr); break;
        case grid::TypeT<double>::Sig:
          item = PyFloat_FromDouble(*(double*)args_ptr); break;
        case grid::TypeT<long double>::Sig:
          item = PyFloat_FromDouble(*(long double*)args_ptr); break;
        case grid::TypeT<std::string&>::Sig:
          item = PyUnicode_FromString(((std::string&)args_ptr).c_str()); break;
        default: break;
      }
    }

    if (item == NULL)
    {
      Py_DECREF(tuple);
      PyErr_SetString(PyExc_TypeError, "Failed to get parameter");
      return NULL;
    }

    PyTuple_SET_ITEM(tuple, i - 1, item);
    args_ptr += count * size;
  }
  return tuple;
}


// Helper function to write a python object to the current argument entry
int WriteSingleArgument(PyObject* item, uintptr_t& args_ptr, unsigned long trait)
{
  unsigned int count = trait >> grid::kCountShift;
  size_t align = 1 << ((trait & grid::kAlignMask) >> grid::kAlignShift);
  size_t size = 1 << (trait & grid::kSizeMask);
  int type = (trait & grid::kTypeMask) >> grid::kTypeShift;
  args_ptr = (args_ptr + align - 1) & -align;

  // TODO: support passing a string as argument
  // if PyUnicode_Check(item) && type != grid::kStdString
  //   convert to type, i.e.  PyLong_FromString( ... )
  //   PyLong_FromUnicodeObject(item, 0);

  // ensure the python object and expected argument type match
  if ((type == grid::kInteger &&   PyLong_Check(item) == 0) ||
      (type == grid::kBoolean &&   PyBool_Check(item) == 0) ||
      (type == grid::kNumber &&    PyNumber_Check(item) == 0) ||
      (type == grid::kStdString && PyUnicode_Check(item) == 0))
    return PyErr_BadArgument();

  int ovfl = 0;
  switch (trait)
  {
   case grid::TypeT<uint8_t>::Sig:
      *(uint8_t*)args_ptr = PyLong_AsUnsignedLong(item); break;
    case grid::TypeT<uint16_t>::Sig:
      *(uint16_t*)args_ptr = PyLong_AsUnsignedLong(item); break;
    case grid::TypeT<uint32_t>::Sig:
      *(uint32_t*)args_ptr = PyLong_AsUnsignedLong(item); break;
    case grid::TypeT<uint64_t>::Sig:
      *(uint64_t*)args_ptr = PyLong_AsLongAndOverflow(item, &ovfl);
    case grid::TypeT<int8_t>::Sig:
      *(int8_t*)args_ptr = PyLong_AsLong(item); break;
    case grid::TypeT<int16_t>::Sig:
      *(int16_t*)args_ptr = PyLong_AsLong(item); break;
    case grid::TypeT<int32_t>::Sig:
      *(int32_t*)args_ptr = PyLong_AsLong(item); break;
    case grid::TypeT<int64_t>::Sig:
      *(int64_t*)args_ptr = PyLong_AsLongLong(item); break;
    case grid::TypeT<bool>::Sig:
      *(bool*)args_ptr = (item == Py_True); break;
    case grid::TypeT<float>::Sig:
      *(float*)args_ptr = PyFloat_AsDouble(item); break;
    case grid::TypeT<double>::Sig:
      *(double*)args_ptr = PyFloat_AsDouble(item); break;
    case grid::TypeT<long double>::Sig:
      *(long double*)args_ptr = PyFloat_AsDouble(item); break;
    case grid::TypeT<std::string>::Sig:
      {
        Py_ssize_t size;
        const char* str = PyUnicode_AsUTF8AndSize(item, &size);
        *(std::string*)args_ptr = std::string(str, size);
        break;
      }
    default: break;
  }

  // FIXME: do something with ovfl
  // FIXME: check if error occured during conversion

  // update pointer (note, it's a reference)
  args_ptr += count * size;
  return 1;
}


// Helper function to write python arguments (tupe, list, string, object) to
// an argument buffer.
static int WriteArguments(PyObject* args,
                          uintptr_t args_ptr,
                          size_t args_sz,
                          const unsigned long* traits)
{
  if (PyTuple_Check(args))
  {
    if (PyTuple_Size(args) != (Py_ssize_t)traits[0])
      return PyErr_BadArgument();

    for (size_t i = 0; i < traits[0]; i++)
      if (WriteSingleArgument(
            PyTuple_GetItem(args, i), args_ptr, traits[i + 1]) != 1)
        return -1;
  }
  else if (PyList_Check(args))
  {
    if (PyList_Size(args) != (Py_ssize_t)traits[0])
      return PyErr_BadArgument();

    for (size_t i = 0; i < traits[0]; i++)
      if (WriteSingleArgument(
            PyList_GetItem(args, i), args_ptr, traits[i + 1]) != 1)
        return -1;
  }
  else if (traits[0] == 1)
  {
    return WriteSingleArgument(args, args_ptr, 1);
  }
  else
    return PyErr_BadArgument();

  return 1;
}


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

  if (!WriteArguments(args, (uintptr_t)arg_buf, arg_buf_sz, traits))
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

  return ReadArguments((uintptr_t)arg_buf, arg_buf_sz, param->GetSignature());
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
