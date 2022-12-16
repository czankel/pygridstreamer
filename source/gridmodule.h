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

// Helper functions to read and write arguments between Python arguments
// and Grid::Arguments. Reading arguments returns a Python tuple. Writing
// arguments can pass a single or tuple of arguments, or a string formatted
// as supported by Grid::Parameters. The value passed as a single argument
// or part of a tuple can also be a string.
PyObject* PyGridStreamerReadArguments(uintptr_t, size_t, const unsigned long*);
int PyGridStreamerWriteArguments(PyObject*, uintptr_t, size_t, const unsigned long*);

// Helper function to convert camelCase/CamelCase to snake_case
std::string PythonifyName(const std::string& name);

extern "C" {

extern PyTypeObject pygrid_type;
extern PyTypeObject pychannel_type;
extern PyTypeObject pycell_type;
extern PyTypeObject pyparameter_type;

// PyGrid describes the Grid class for Python and encapsulates the grid object.
typedef struct
{
  PyObject_HEAD
  PyObject*                         name;
  std::shared_ptr<grid::Grid>       grid;
} PyGrid;


// PyChannel describes a Channel in Grid.
typedef struct
{
  PyObject_HEAD
  PyObject*                         name;
  PyGrid*                           grid;
  std::shared_ptr<grid::Channel>    channel;
} PyChannel;

// PyChannel exported functions
PyObject* PyChannelCompile(PyChannel* self, PyObject* pylayout);


// PyCell describes a Cell in Grid. The "parent" element can be a PyChannel
// or a PyCell describing a Cluster or Pipeline.
typedef struct
{
  PyObject_HEAD
  PyObject*                         name;
  PyObject*                         type;
  PyObject*                         parent;
  PyObject*                         dict;
  std::shared_ptr<grid::Cell>       cell;
} PyCell;


// PyParameter describes a Parameter in Grid.
typedef struct
{
  PyObject_HEAD
  PyObject*                         name;
  std::shared_ptr<grid::Parameter>  parameter;
} PyParameter;


} // end of extern "C"


#endif  // GRIDMODULE_H
