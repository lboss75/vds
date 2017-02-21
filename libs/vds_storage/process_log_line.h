#ifndef __VDS_STORAGE_PROCESS_LOG_LINE_H_
#define __VDS_STORAGE_PROCESS_LOG_LINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


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
        auto log_obj = dynamic_cast<json_object *>(log_record);
        if (nullptr == log_obj) {
          throw new std::runtime_error("Invalid log record");
        }

        auto message = log_obj->get_property("m");
        std::string fingerprint;
        if (!log_obj->get_property_string("f", fingerprint)) {
          throw new std::runtime_error("Invalid log record");
        }

        std::string signature;
        if (!log_obj->get_property_string("s", signature)) {
          throw new std::runtime_error("Invalid log record");
        }

        certificate * sign_cert = nullptr;
        if (this->owner_->is_empty()) {
          sign_cert = this->owner_->parse_root_cert(message);
        }
        else {
          sign_cert = this->owner_->get_cert(fingerprint);
        }

        if (fingerprint != sign_cert->fingerprint()) {
          throw new std::runtime_error("Invalid certificate");
        }

        json_writer writer;
        message->str(writer);

        auto message_body = writer.str();

        hash h(hash::sha256());
        h.update(message_body.c_str(), message_body.length());
        h.final();

       
        auto cert_public_key = sign_cert->public_key();
        asymmetric_sign_verify verifier(hash::sha256(), cert_public_key);
        verifier.update(h.signature(), h.signature_length());

        std::vector<uint8_t> sig_data;
        base64::to_bytes(signature, sig_data);
        if (!verifier.verify((const unsigned char *)sig_data.data(), sig_data.size())) {
          throw new std::runtime_error("Invalid log record");
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
