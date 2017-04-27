#ifndef __VDS_DATA_COMPRESSED_ARCHIVE_H_
#define __VDS_DATA_COMPRESSED_ARCHIVE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _compressed_archive;
  
  class compressed_archive
  {
  public:
    compressed_archive();
    ~compressed_archive();
    
    void add(const std::string & name, const std::string & body);
    void add(const std::string & name, const filename & file);
    
  private:
    _compressed_archive * const impl_;
  };
}

#endif // __VDS_DATA_COMPRESSED_ARCHIVE_H_
