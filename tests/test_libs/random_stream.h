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
  
    void write(const vds::service_provider & sp, const item_type * data, size_t len) override
    {
		if (0 == len) {
			this->target_.write(sp, data, len);
		}
		else {
			while (0 < len) {
				size_t n = (size_t)std::rand() % len;
				if (n < 1) {
					n = 1;
				}
				if (len < n) {
					n = len;
				}

				this->target_.write(sp, data, len);

				data += n;
				len -= n;
			}
		}
    }
    
  private:
    vds::stream<item_type> & target_;
  };

template<typename item_type>
class random_stream_async : public vds::stream_async<item_type>
{
public:
  random_stream_async(vds::stream_async<item_type> & target)
  : target_(target)
  {
  }
  
    vds::async_task<> write_async(const vds::service_provider & sp, const item_type * data, size_t len) override
    {
      if (0 == len) {
        return this->target_.write_async(sp, data, len);
      }
      else {
        size_t n = (size_t)std::rand() % len;
        if (n < 1) {
          n = 1;
        }
        if (len < n) {
          n = len;
        }

        if(n == len){
          return this->target_.write(sp, data, n);
        }
        
        return 
          this->target_.write(sp, data, n)
          .then([this, sp, p = data + n, l = len - n](){
            return this->write_async(sp, p, l);
          });
      }
    }
    
  private:
    vds::stream_async<item_type> & target_;
  };

#endif // __TEST_VDS_LIBS__RANDOM_READER_H_
