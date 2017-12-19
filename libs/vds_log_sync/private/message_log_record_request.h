#ifndef __VDS_LOG_SYNC_MESSAGE_LOG_RECORD_REQUEST_H_
#define __VDS_LOG_SYNC_MESSAGE_LOG_RECORD_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class message_log_record_request {
  public:
    static const uint8_t message_id = (uint8_t)'r';

    message_log_record_request(const std::list<std::string> & record_ids)
        : record_ids_(record_ids){
    }

    const_data_buffer serialize() const{
      binary_serializer s;

      s << message_id << this->record_ids_;

      return const_data_buffer(s.data().data(), s.size());
    }

  private:
    std::list<std::string> record_ids_;
  };
}

#endif //__VDS_LOG_SYNC_MESSAGE_LOG_RECORD_REQUEST_H_
