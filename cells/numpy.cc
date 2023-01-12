//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//


#include "numpy.h"

namespace grid {
namespace cells {

NumPyImage::NumPyImage()
{
  RegisterIngressPort("", this, ingress_port_);
  RegisterEgressPort("", this, egress_port_);
  RegisterOrigin("NumPyImageOrigin",
                 BaseTransportHandler(This,
                                      &NumPyImage::OnDiscovery,
                                      &NumPyImage::OnTransport,
                                      &NumPyImage::OnMessage),
                 origin_);
}


bool NumPyImage::OnStateTransition(State state, Transition transition)
{
  return true;
}


// FIXME: differs if Origin or Sink
bool NumPyImage::OnDiscovery(ImageStream& stream,
                             ImageDiscovery& discovery,
                             Port& port)
{
  ImageSurvey survey = discovery.GetImageSurvey();
  // survey.RequireBufferSize(ImageFormat(...))
  // survey.ProvideMetadata(meta_frame_);
  discovery.Forward(egress_port_);

  return true;
}

bool NumPyImage::OnMessage(ImageStream& stream,
                           const Message& message,
                           const Port& port)
{
  return false;
}


// FIXME: differs if Origin or Sink?? can check by port...
Transport::Result NumPyImage::OnTransport(ImageStream& stream,
                                          ImageTransport& transport,
                                          const Port& port)
{
  Image* image = transport.AllocateAttachedImage();

  transport.Forward(egress_port_);
  return Transport::kOk;
}


} // end of namespace cells
} // end of namespace grid


// https://numpy.org/doc/stable/reference/c-api/index.html
//

typedef struct PyArrayObject {
  PyObject_HEAD
    char *data;               // images: data buffer
  int nd;                   // images: 2 dimensions
  npy_intp *dimensions;     // images: width and height
  npy_intp *strides;        // images: strides
  PyObject *base;           // images: ?? 
  PyArray_Descr *descr;     //
  int flags;
  PyObject *weakreflist;
  /* version dependent private members */
} PyArrayObject;


static PyObject *
trace(PyObject *self, PyObject *args)
{
  PyArrayObject *array;
  double sum;
  int i, n;

  if (!PyArg_ParseTuple(args, "O!",
        &PyArray_Type, &array))
    return NULL;
  if (array->nd != 2 || array->descr->type_num != PyArray_DOUBLE) {
    PyErr_SetString(PyExc_ValueError,
        "array must be two-dimensional and of type float");
    return NULL;
  }

  n = array->dimensions[0];
  if (n > array->dimensions[1])
    n = array->dimensions[1];
  sum = 0.;
  for (i = 0; i < n; i++)
    sum += *(double *)(array->data + i*array->strides[0] + i*array->strides[1]);

  return PyFloat_FromDouble(sum);
}

static PyObject *
matrix_vector(PyObject *self, PyObject *args)
{
  PyObject *input1, *input2;
  PyArrayObject *matrix, *vector, *result;

  int dimensions[1];
  double factor[1];
  double real_zero[1] = {0.};
  long int_one[1] = {1};
  long dim0[1], dim1[1];

  extern dgemv_(char *trans, long *m, long *n,
      double *alpha, double *a, long *lda,
      double *x, long *incx,
      double *beta, double *Y, long *incy);

  if (!PyArg_ParseTuple(args, "dOO", factor, &input1, &input2))
    return NULL;
  matrix = (PyArrayObject *)
    PyArray_ContiguousFromObject(input1, PyArray_DOUBLE, 2, 2);
  if (matrix == NULL)
    return NULL;
  vector = (PyArrayObject *)
    PyArray_ContiguousFromObject(input2, PyArray_DOUBLE, 1, 1);
  if (vector == NULL)
    return NULL;
  if (matrix->dimensions[1] != vector->dimensions[0]) {
    PyErr_SetString(PyExc_ValueError,
        "array dimensions are not compatible");
    return NULL;
  }

  dimensions[0] = matrix->dimensions[0];
  result = (PyArrayObject *)PyArray_FromDims(1, dimensions, PyArray_DOUBLE);
  if (result == NULL)
    return NULL;

  dim0[0] = (long)matrix->dimensions[0];
  dim1[0] = (long)matrix->dimensions[1];
  dgemv_("T", dim1, dim0, factor, (double *)matrix->data, dim1,
      (double *)vector->data, int_one,
      real_zero, (double *)result->data, int_one);

  return PyArray_Return(result);
}


