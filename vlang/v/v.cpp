// vcompiler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "v.h"

int main(int argc, const char **argv)
{
  try
  {
    vds::vcompiler app;
    return app.run(argc, argv);
  }
  catch(std::exception * ex)
  {
    printf("Error: %s", ex->what());
    delete ex;
    return 1;
  }
}

static bool ends_with(const std::string & str, const char * suffix)
{
  
  if(str.empty() || suffix == nullptr){
    return false;
  }

  size_t str_len = str.size();
  size_t suffix_len = strlen(suffix);

  if(suffix_len > str_len) {
    return false;
  }

  return 0 == strncmp(str.c_str() + str_len - suffix_len, suffix, suffix_len);
}

int vds::vcompiler::run(int argc, const char** argv)
{
  std::unique_ptr<vruntime_machine> machine(new vruntime_machine());
  for(int i = 1; i < argc; ++i){
    vsyntax t;
    
    vfolder::enumerate(
      argv[i],
      [&t](const std::string & filename, bool is_folder) {
        if(!is_folder && ends_with(filename, ".v")) {
          std::cout << "compiling " << filename << "\n";
          text_stream s(filename);
          
          static const char * operators[] =
          {
            "/", "//", "/*",
            ".",
            "{", "}",
            "[", "]",
            "(", ")",
            ":",
            ",",
            ";",
            "=", "==",
            "+", "-",
            nullptr
          };

          vtokenizer p(s, operators);
          
          static const char * keywords[] = {
            "class",
            "else",
            "for",
            "foreach",
            "function",
            "dependency",
            "external",
            "if",
            "in",
            "interface",
            "namespace",
            "new",
            "override",
            "package",
            "private",
            "property",
            "public",
            "return",
            "static",
            "using",
            "var",
            "void",
            "while",
            nullptr
          };
          vlexer l(p, keywords);
          
          t.parse(l);
        }
        
        return true;          
      }
    );
    
    vrt_external_method_resolver resolver;
    auto package = vpackage_compiler::compile(machine.get(), t, resolver);
   
  }

  return 0;
}
