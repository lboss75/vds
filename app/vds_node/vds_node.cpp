/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vds_http.h"
#include "vds_node_app.h"

int main(int argc, const char ** argv)
{
  vds::vds_node_app app;
  
  return app.run(argc, argv);
}