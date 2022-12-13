//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//

#ifndef GRIDMODULE_H
#define GRIDMODULE_H

#include <Python.h>

#include <grid/fw/grid.h>

extern "C" {

extern PyTypeObject pygrid_type;

// PyGrid describes the Grid class for Python and encapsulates the grid object.
typedef struct
{
  PyObject_HEAD
  PyObject*                         name;
  std::shared_ptr<grid::Grid>       grid;
} PyGrid;


} // end of extern "C"


#endif  // GRIDMODULE_H
