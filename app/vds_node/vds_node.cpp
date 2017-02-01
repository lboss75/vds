#include <iostream>
#include "vds_http.h"
#include "vds_node_app.h"

int main(int argc, const char ** argv)
{
  vds::vds_node_app app;
  
  return app.run(argc, argv);
}