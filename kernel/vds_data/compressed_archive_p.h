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
      compressed_archivePrivate(compressed_archive* q);
      virtual ~compressed_archivePrivate();

  private:
      class compressed_archive* const q;
  };
}
#endif // __VDS_DATA_COMPRESSED_ARCHIVE_P_H_
