#ifndef __VRT_RESOLVE_DEPENDENCY_H_
#define __VRT_RESOLVE_DEPENDENCY_H_

#include "vrt_statement.h"

namespace vds {
  class vrt_resolve_dependency : public vrt_statement {
  public:
    vrt_resolve_dependency(
      const vrt_source_file * file,
      int line,
      int column,
      const vrt_type * interface_type
    );

    bool execute(vrt_context & context) const override;

  private:
    const vrt_type * interface_type_;
  };
}

#endif // !__VRT_RESOLVE_DEPENDENCY_H_
