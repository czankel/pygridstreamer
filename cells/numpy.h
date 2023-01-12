//
// Copyright (C) Chris Zankel. All rights reserved.
// This code is subject to U.S. and other copyright laws and
// intellectual property protections.
//
// The contents of this file are confidential and proprietary to Chris Zankel.
//

#ifndef GRID_CELLS_PYTHON_NUMPY_H
#define GRID_CELLS_PYTHON_NUMPY_H

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio/legacy/constants_c.h>

#include <grid/transport/image.h>
#include <grid/base/basecell.h>
#include <grid/fw/parameter.h>
#include <grid/fw/message.h>

#include <grid/library/metadata.h>

extern "C" {
#include <libswscale/swscale.h>
}

#include "opencv.h"

namespace grid {
namespace cells {

class NumPyImage
{
 public:
  NumPyImage();

  bool OnStateTransition(State state, Transition transition);
  bool OnDiscovery(ImageStream&, ImageDiscovery&, Port&);
  bool OnMessage(ImageStream&, const Message&, const Port&);
  Transport::Result OnTransport(ImageStream&, ImageTransport&, const Port&);


 private:

  // ParameterT<>

  Port        input_port_;
  Port        egress_port_;
  ImageOrigin origin_;

  Image::Format input_format_;
  Image::Format output_format_;
};


} // end of namespace cells
} // end of namespace grid

#endif  // GRID_CELLS_PYTHON_NUMPY_H
