#ifndef __VDS_PROTOCOLS_CLIENT_MESSAGES_H_
#define __VDS_PROTOCOLS_CLIENT_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "log_records.h"
#include "server_task_manager.h"

namespace vds {

  namespace client_messages
  {
    class certificate_and_key_request
    {
    public:
      static const char message_type[];
      
      certificate_and_key_request(
        const std::string & object_name,
        const const_data_buffer & password_hash
      );
      
      certificate_and_key_request(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      const std::string & object_name() const { return this->object_name_; }
      const const_data_buffer & password_hash() const { return this->password_hash_; }

    private:
      std::string object_name_;
      const_data_buffer password_hash_;
    };

    class certificate_and_key_response
    {
    public:
      static const char message_type[];

      certificate_and_key_response(
        const guid & id,
        const std::string & certificate_body,
        const std::string & private_key_body,
        size_t order_num,
        const std::list<guid> & parents);

      certificate_and_key_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      const guid & id() const { return this->id_; }
      const std::string & certificate_body() const { return this->certificate_body_; }
      const std::string & private_key_body() const { return this->private_key_body_; }
      size_t order_num() const { return this->order_num_; }
      const std::list<guid> & parents() const { return this->parents_; }

    private:
      guid id_;
      std::string certificate_body_;
      std::string private_key_body_;
      size_t order_num_;
      std::list<guid> parents_;
    };

    class register_server_request
    {
    public:
      static const char message_type[];

      register_server_request(
        const guid & id,
        const guid & parent_id,
        const std::string & server_certificate,
        const std::string & server_private_key,
        const const_data_buffer & password_hash);

      register_server_request(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      const guid & id() const { return this->id_; }
      const guid & parent_id() const { return this->parent_id_; }
      const std::string & server_certificate() const { return this->server_certificate_; }
      const std::string & server_private_key() const { return this->server_private_key_; }
      const const_data_buffer & password_hash() const { return this->password_hash_; }

    private:
      guid id_;
      guid parent_id_;
      std::string server_certificate_;
      std::string server_private_key_;
      const_data_buffer password_hash_;
    };

    class register_server_response
    {
    public:
      static const char message_type[];

      register_server_response();
      register_server_response(const std::shared_ptr<json_value> &);

      std::shared_ptr<json_value> serialize() const;
    };
    //////////////////////////////////////////////////////////////
    class server_log_state_request
    {
    public:
      static const char message_type[];

      server_log_state_request();

      server_log_state_request(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;
    };

    class server_log_state_response
    {
    public:
      static const char message_type[];

      server_log_state_response(
        size_t order_num,
        const std::list<guid> & parents);

      server_log_state_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      size_t order_num() const { return this->order_num_; }
      const std::list<guid> & parents() const { return this->parents_; }

    private:
      size_t order_num_;
      std::list<guid> parents_;
    };
    //////////////////////////////////////////////////////////////
    class principal_log_members_request
    {
    public:
      static const char message_type[];

      principal_log_members_request(
        const guid & principal_id)
      : principal_id_(principal_id)
      {
      }

      principal_log_members_request(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;
      
    private:
      guid principal_id_;
    };
    
    class principal_log_members_response
    {
    public:
      static const char message_type[];

      principal_log_members_response(
        const std::list<std::string> & members);

      principal_log_members_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      const std::list<std::string> & members() const { return this->members_; }

    private:
      std::list<std::string> members_;
    };

    //////////////////////////////////////////////////////////////
    class principal_log_add_record_request
    {
    public:
      static const char message_type[];

      principal_log_add_record_request(
        const std::string & message,
        const const_data_buffer & message_sign)
      : message_(message),
        message_sign_(message_sign)
      {
      }

      principal_log_add_record_request(const std::shared_ptr<json_value> & source)
      {
        auto s = std::dynamic_pointer_cast<json_object>(source);
        if (s) {
          s->get_property("m", this->message_);
          s->get_property("s", this->message_sign_);
        }
      }

      std::shared_ptr<json_value> serialize(bool add_type) const
      {
        auto result = std::make_shared<json_object>();
        if (add_type) {
          result->add_property("$t", message_type);
        }

        result->add_property("m", this->message_);
        result->add_property("s", this->message_sign_);
        return result;
      }

      const std::string & message() const { return this->message_; }
      const const_data_buffer & message_sign() const { return this->message_sign_; }

    private:
      std::string message_;
      const_data_buffer message_sign_;
    };

    class principal_log_add_record_response
    {
    public:
      static const char message_type[];

      principal_log_add_record_response(const std::shared_ptr<json_value> & source)
      {
      }

      std::shared_ptr<json_value> serialize(bool add_type) const
      {
        auto result = std::make_shared<json_object>();
        if (add_type) {
          result->add_property("$t", message_type);
        }
      }
    };
    //////////////////////////////////////////////////////////////
    class put_object_message
    {
    public:
      static const char message_type[];

      put_object_message(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      put_object_message(
        const guid & principal_id,
        const principal_log_record & principal_msg,
        const const_data_buffer & signature,
        const guid & version_id,
        const filename & tmp_file,
        const const_data_buffer & file_hash);

      const guid & principal_id() const { return this->principal_id_; }
      const principal_log_record & principal_msg() const { return this->principal_msg_; }
      const const_data_buffer & signature() const { return this->signature_; }
      const guid & version_id() const { return this->version_id_; }
      const filename & tmp_file() const { return this->tmp_file_; }
      const const_data_buffer & file_hash() const { return this->file_hash_; }

    private:
      guid principal_id_;
      principal_log_record principal_msg_;
      const_data_buffer signature_;
      guid version_id_;
      filename tmp_file_;
      const_data_buffer file_hash_;
    };

    class put_object_message_response
    {
    public:
      static const char message_type[];

      put_object_message_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      put_object_message_response();
    };
    //////////////////////////////////////////////////
    class principal_log_request
    {
    public:
      static const char message_type[];

      principal_log_request(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      principal_log_request(
        const guid & principal_id,
        size_t last_order_num)
      : principal_id_(principal_id),
        last_order_num_(last_order_num)
      {
      }
      

      const guid & principal_id() const { return this->principal_id_; }
      const size_t last_order_num() const { return this->last_order_num_; }

    private:
      guid principal_id_;
      size_t last_order_num_;
    };

    class principal_log_response
    {
    public:
      static const char message_type[];
      principal_log_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      principal_log_response(
        const guid & principal_id,
        size_t last_order_num,
        const std::list<principal_log_record> & records)
      : principal_id_(principal_id),
        last_order_num_(last_order_num),
        records_(records)
      {
      }
      
      const guid & principal_id() const { return this->principal_id_; }
      const size_t last_order_num() const { return this->last_order_num_; }
      const std::list<principal_log_record> & records() const { return this->records_; }
      
    private:
      guid principal_id_;
      size_t last_order_num_;
      std::list<principal_log_record> records_;
    };
    
    //////////////////////////////////////////////////
    class get_object_request
    {
    public:
      static const char message_type[];

      get_object_request(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      get_object_request(
        const guid & version_id,
        const filename & tmp_file)
      : version_id_(version_id),
        tmp_file_(tmp_file)
      {
      }

      const guid & version_id() const { return this->version_id_; }
      const filename & tmp_file() const { return this->tmp_file_; }

    private:
      guid version_id_;
      filename tmp_file_;
    };

    class get_object_response
    {
    public:
      static const char message_type[];

      get_object_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      get_object_response(const server_task_manager::task_state & state)
      : state_(state)
      {
      }

      const server_task_manager::task_state & state() const { return this->state_; }
    private:
      server_task_manager::task_state state_;
    };
  };
}

#endif // __VDS_PROTOCOLS_CLIENT_MESSAGES_H_
