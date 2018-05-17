#ifndef __VDS_TRANSACTIONS_CHANNEL_MESSAGE_H_
#define __VDS_TRANSACTIONS_CHANNEL_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <cert_control.h>
#include "const_data_buffer.h"
#include "asymmetriccrypto.h"
#include "transaction_id.h"
#include "symmetriccrypto.h"
#include "guid.h"

namespace vds {
  namespace transactions {
    class channel_message {
    public:
      static const transaction_id message_id = transaction_id::channel_message;

      channel_message(
          const const_data_buffer &channel_id,
          const std::string &channel_read_cert_subject,
          const std::string &write_cert_subject,
          const const_data_buffer &crypted_key,
          const const_data_buffer &crypted_data,
          const const_data_buffer &signature)
          : channel_id_(channel_id),
            channel_read_cert_subject_(channel_read_cert_subject),
            write_cert_subject_(write_cert_subject),
            crypted_key_(crypted_key),
            crypted_data_(crypted_data),
            signature_(signature)
      {}

      channel_message(binary_deserializer & s){
        s
            >> this->channel_id_
            >> this->channel_read_cert_subject_
            >> this->write_cert_subject_
            >> this->crypted_key_
            >> this->crypted_data_
            >> this->signature_;
      }

      void serialize(binary_serializer & s) const {
        s
            << (uint8_t)message_id
            << this->channel_id_
            << this->channel_read_cert_subject_
            << this->write_cert_subject_
            << this->crypted_key_
            << this->crypted_data_
            << this->signature_;

      }

      const const_data_buffer & channel_id() const {
        return this->channel_id_;
      }

      const std::string & channel_read_cert_subject() const {
        return this->channel_read_cert_subject_;
      }

      const std::string & write_cert_subject() const {
        return this->write_cert_subject_;
      }

      const const_data_buffer & crypted_key() const {
        return this->crypted_key_;
      }

      const const_data_buffer & crypted_data() const {
        return this->crypted_data_;
      }

      const const_data_buffer & signature() const {
        return this->signature_;
      }

    private:
      const_data_buffer channel_id_;
      std::string channel_read_cert_subject_;
      std::string write_cert_subject_;
      const_data_buffer crypted_key_;
      const_data_buffer crypted_data_;
      const_data_buffer signature_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_MESSAGE_H_
