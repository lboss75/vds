/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vds_embedded.h"

class vds_configuration {
public:

};

void * vds_create_configuration() {
  return new vds_configuration();
}

void vds_delete_configuration(void * config) {
  delete reinterpret_cast<vds_configuration *>(config);
}
