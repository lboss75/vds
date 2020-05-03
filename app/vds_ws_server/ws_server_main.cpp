/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "ws_server_app.h"

int main(int argc, const char **argv)
{
  vds::ws_server_app app;
  return app.run(argc, argv);
}

