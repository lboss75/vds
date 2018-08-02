#include "stdafx.h"
#include "vds_get_root.h"
#include "get_root_app.h"

int main(int argc, const char **argv)
{
  vds::get_root_app app;
  return app.run(argc, argv);
}
