/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__RANDOM_READER_H_
#define __TEST_VDS_LIBS__RANDOM_READER_H_

#include "targetver.h"
#include "types.h"
#include "stream.h"

template<typename item_type>
class random_stream : public vds::stream<item_type>
{
public:
  random_stream(vds::stream<item_type> & target)
  : target_(target)
  {
  }
  
    void write(const item_type * data, size_t len)
    {
      while (0 < len) {
        size_t n = (size_t)std::rand() % len;
        if (n < 1) {
          n = 1;
        }
        if(len < n) {
          n = len;
        }
        
        this->target_.write(data, len);

        data += n;
        len -= n;
      }
    }
    
    void final()
    {
      this->target_.final();
    }

  private:
    vds::stream<item_type> & target_;
  };


#endif // __TEST_VDS_LIBS__RANDOM_READER_H_
