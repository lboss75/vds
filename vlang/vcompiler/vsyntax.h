#ifndef __VSYNTAX_H_
#define __VSYNTAX_H_

#include <list>

namespace vds{
  class vlexer;
  class vfile;
  
  class vsyntax
  {
  public:
    vsyntax();
    
    void parse(vlexer & lexer);
    void save(const std::string & filename);
    
    const std::list<std::unique_ptr<vfile>> & files() const
    {
      return this->files_;
    }
    
  private:    
    std::list<std::unique_ptr<vfile>> files_;
  };
}

#endif // __VSYNTAX_H_
