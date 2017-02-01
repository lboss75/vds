#include "stdafx.h"
#include "vsyntax.h"
#include "vlexer.h"
#include "compile_error.h"
#include "vfile_syntax.h"

vds::vsyntax::vsyntax()
{
}

void vds::vsyntax::save(const std::string & filename)
{
  std::ofstream f(filename, std::ios_base::out);

  f << "{}\n";

  f.close();
}

void vds::vsyntax::parse(vds::vlexer& lexer)
{
  this->files_.push_back(vfile_syntax::parse(lexer));
}
