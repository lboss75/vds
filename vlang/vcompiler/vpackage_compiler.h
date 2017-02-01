#ifndef __VPACKAGE_COMPILER_H_
#define __VPACKAGE_COMPILER_H_

#include "vsyntax.h"
#include "vrt_package.h"
#include "vfile_syntax.h"
#include "name_resolver.h"

namespace vds {
  class vruntime_machine;
  class vrt_external_method_resolver;
  class vtype_resolver;
  
  class vpackage_compiler
  {
  public:
    static vrt_package * compile(
      vruntime_machine * machine,
      const vsyntax & source,
      const vrt_external_method_resolver & external_method_resolver
    );

  private:
    vruntime_machine * machine_;
    const vds::vpackage * package_;
    const vfile * package_file_;
    name_resolver name_resolver_;

    class compiled_item
    {
    public:

      virtual ~compiled_item();

      virtual void resolve_types() = 0;
      virtual void compile() = 0;
    };

    class compiled_property : public compiled_item
    {
    public:
      compiled_property(
        const std::shared_ptr<vtype_resolver> & resolver,
        vrt_property * compiled,
        vproperty * original);

      void resolve_types() override;
      void compile() override;

    private:
      std::shared_ptr<vtype_resolver> type_resolver_;
      vrt_property * compiled_;
      vproperty * original_;
    };

    class compiled_method : public compiled_item
    {
    public:
      compiled_method(
        const std::shared_ptr<vtype_resolver> & resolver,
        vrt_method * compiled,
        vmethod * original);

      void resolve_types() override;
      void compile() override;

    private:
      std::shared_ptr<vtype_resolver> type_resolver_;
      vrt_method * compiled_;
      vmethod * original_;
    };

    class compiled_external_method : public compiled_item
    {
    public:
      compiled_external_method(
        const std::shared_ptr<vtype_resolver> & resolver,
        vrt_external_method * compiled,
        vmethod * original,
        const vrt_external_method_resolver & external_method_resolver);

      void resolve_types() override;
      void compile() override;

    private:
      const vrt_external_method_resolver & external_method_resolver_;
      std::shared_ptr<vtype_resolver> type_resolver_;
      vrt_external_method * compiled_;
      vmethod * original_;
    };

    class compiled_class : public compiled_item
    {
    public:
      compiled_class(
        const std::shared_ptr<vtype_resolver> & resolver,
        vrt_class * compiled,
        vclass * original);

      void resolve_types() override;
      void compile() override;

    private:
      std::shared_ptr<vtype_resolver> type_resolver_;
      vrt_class * compiled_;
      vclass * original_;
    };
    
    vpackage_compiler(
      vruntime_machine * machine,
      const vsyntax & source);
    void load_package(const vsyntax & source);
    void load_dependencies();

    vrt_package * compile(
      const vds::vsyntax & source,
      const vrt_external_method_resolver & external_method_resolver
    );
   
    static vrt_json_value * compile_json(
      const vds::vrt_source_file * source_file,
      const vjson_value * object);
  };
}

#endif // __VPACKAGE_COMPILER_H_
