#ifndef __VDS_DATA_COMPRESSED_ARCHIVE_P_H_
#define __VDS_DATA_COMPRESSED_ARCHIVE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _compressed_archive
  {
  public:
    _compressed_archive(compressed_archive* owner);
      virtual ~_compressed_archive();



  private:
      compressed_archive * const owner_;
  };
}
#endif // __VDS_DATA_COMPRESSED_ARCHIVE_P_H_
