//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//

#include "gridmodule.h"

#include <Python.h>

#include <grid/fw/cell.h>


//
// PyGridStreamerCellTypes returns a list of cell types.
//
static PyObject* PyGridStreamerCellTypes(PyObject *self, PyObject *args)
{
  PyObject* cell_list = PyList_New(0);

  for (auto fac_it = grid::CellDirectory::Begin();
       fac_it != grid::CellDirectory::End();
       ++fac_it)
  {
    PyList_Append(cell_list, PyUnicode_FromString(fac_it->GetType().c_str()));
  }

  return cell_list;
}


static PyMethodDef GridStreamerMethods[] =
{
  {
    "celltypes",
    PyGridStreamerCellTypes,
    METH_NOARGS,
    "Return a list of registered cell types"
  },
  {
    NULL
  }
};


static struct PyModuleDef GridStreamerModule = {
    PyModuleDef_HEAD_INIT,
    "gridstreamer",
    "Python interface for the GridStreamer project",
    -1,
    GridStreamerMethods
};


PyMODINIT_FUNC
PyInit_pygridstreamer(void)
{
  PyObject* module = PyModule_Create(&GridStreamerModule);
  if (module == NULL)
    return NULL;

  if (PyType_Ready(&pygrid_type) < 0)
    return NULL;

  Py_INCREF(module);
  if (PyModule_AddObject(module, "Grid", (PyObject *) &pygrid_type) < 0) {
    Py_DECREF(&pygrid_type);
    Py_DECREF(module);
    return NULL;
  }

  if (PyType_Ready(&pychannel_type) < 0)
    return NULL;

  Py_INCREF(module);
  if (PyModule_AddObject(module, "Channel", (PyObject *) &pychannel_type) < 0) {
    Py_DECREF(&pychannel_type);
    Py_DECREF(module);
    return NULL;
  }

  if (PyType_Ready(&pycell_type) < 0)
    return NULL;

  Py_INCREF(module);
  if (PyModule_AddObject(module, "Cell", (PyObject *) &pycell_type) < 0) {
    Py_DECREF(&pycell_type);
    Py_DECREF(module);
    return NULL;
  }

  if (PyType_Ready(&pyparameter_type) < 0)
    return NULL;

  Py_INCREF(module);
  if (PyModule_AddObject(module, "Cell", (PyObject *) &pyparameter_type) < 0) {
    Py_DECREF(&pyparameter_type);
    Py_DECREF(module);
    return NULL;
  }


  return module;
}
