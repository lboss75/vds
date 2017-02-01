#include "stdafx.h"
#include "vrt_global_scope.h"
#include "vrt_method_scope.h"

void vds::vrt_global_scope::invoke(const vrt_method * method)
{
  std::shared_ptr<vrt_method_scope> method_scope(new vrt_method_scope());
  method_scope->invoke(method);
}
