#ifndef __VDS_STORAGE_PROCESS_LOG_LINE_H_
#define __VDS_STORAGE_PROCESS_LOG_LINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "log_records.h"

namespace vds {
  template <typename handler_type>
  class process_log_line
  {
  public:
    process_log_line(handler_type * owner)
      : owner_(owner)
    {
    }

    template <typename context_type>
    class handler : public sequence_step<context_type, void(void)>
    {
      using base_class = sequence_step<context_type, void(void)>;
    public:
      handler(
        const context_type & context,
        const process_log_line & args
      ) : base_class(context),
        owner_(args.owner_)
      {
      }

      void operator()(json_value * log_record) {
        server_log_record record;
        record.deserialize(log_record);
        
        if (record.fingerprint_.empty()
          || record.signature_.empty()
          || !record.message_
          || nullptr == dynamic_cast<json_object *>(record.message_.get())
          ) {
          throw new std::runtime_error("Invalid log record");
        }

        std::string message_type;
        if (!dynamic_cast<json_object *>(record.message_.get())->get_property("$type", message_type, false)
          || server_log_batch::message_type != message_type) {
          throw new std::runtime_error("Invalid log record");
        }

        json_writer writer;
        record.message_->str(writer);
        auto message_body = writer.str();

        server_log_batch batch;
        batch.deserialize(record.message_.get());

        certificate * sign_cert = nullptr;
        if (this->owner_->is_empty()) {
          sign_cert = this->owner_->parse_root_cert(batch.messages_->get(0));
        }
        else {
          sign_cert = this->owner_->get_cert(record.fingerprint_);
        }

        if (record.fingerprint_ != sign_cert->fingerprint()) {
          throw new std::runtime_error("Invalid certificate");
        }

        hash h(hash::sha256());
        h.update(message_body.c_str(), message_body.length());
        h.final();

       
        auto cert_public_key = sign_cert->public_key();
        asymmetric_sign_verify verifier(hash::sha256(), cert_public_key);
        verifier.update(h.signature(), h.signature_length());

        std::vector<uint8_t> sig_data;
        base64::to_bytes(record.signature_, sig_data);
        if (!verifier.verify((const unsigned char *)sig_data.data(), sig_data.size())) {
          throw new std::runtime_error("Invalid log record");
        }
        
        for (size_t i = this->owner_->is_empty() ? 1 : 0; i < batch.messages_->size(); ++i) {
          this->owner_->apply_record(batch.messages_->get(i));
        }
      }

    private:
      handler_type * owner_;
    };

  private:
    handler_type * owner_;
  };
}

#endif // __VDS_STORAGE_PROCESS_LOG_LINE_H_
