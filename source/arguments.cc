//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//


#include <grid/util/arguments.h>

#include <Python.h>


// Helper function to read arguments from an argument buffer into a python tuple
PyObject*
PyGridStreamerReadArguments(void* args_buf,
                            size_t args_sz,
                            const unsigned long* traits)
{
  PyObject* tuple = PyTuple_New(traits[0]);
  uintptr_t args_ptr = (uintptr_t) args_buf;

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
      unsigned long t =  (trait & ~grid::kCountMask) | (1 << grid::kCountShift);
      if (t == grid::TypeT<uint8_t>::Sig)
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
static int
WriteSingleArgument(PyObject* item, uintptr_t& args_ptr, unsigned long trait)
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
int PyGridStreamerWriteArguments(PyObject* args,
                                 void* args_buf,
                                 size_t args_sz,
                                 const unsigned long* traits)
{
  uintptr_t args_ptr = (uintptr_t) args_buf;

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
