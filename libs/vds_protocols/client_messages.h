#ifndef __VDS_PROTOCOLS_CLIENT_MESSAGES_H_
#define __VDS_PROTOCOLS_CLIENT_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

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
        const std::list<guid> & active_records,
        size_t order_num
      );

      certificate_and_key_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      const guid & id() const { return this->id_; }
      const std::string & certificate_body() const { return this->certificate_body_; }
      const std::string & private_key_body() const { return this->private_key_body_; }
      const std::list<guid> & active_records() const { return this->active_records_; }
      size_t order_num() const { return this->order_num_; }

    private:
      guid id_;
      std::string certificate_body_;
      std::string private_key_body_;
      std::list<guid> active_records_;
      size_t order_num_;
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
    class put_object_message
    {
    public:
      static const char message_type[];

      put_object_message(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      put_object_message(
        const guid & principal_id,
        const std::shared_ptr<json_value> & principal_msg,
        const const_data_buffer & signature,
        const filename & tmp_file);

      const guid & principal_id() const { return this->principal_id_; }
      const std::shared_ptr<json_value> & principal_msg() const { return this->principal_msg_; }
      const const_data_buffer & signature() const { return this->signature_; }
      const filename & tmp_file() const { return this->tmp_file_; }

    private:
      guid principal_id_;
      std::shared_ptr<json_value> principal_msg_;
      const_data_buffer signature_;
      filename tmp_file_;
    };

    class put_object_message_response
    {
    public:
      static const char message_type[];

      put_object_message_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      put_object_message_response();
    };
    
    /*
    class get_file_message_request
    {
    public:
      static const char message_type[];

      get_file_message_request(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      get_file_message_request(
        const std::string & user_login,
        const std::string & name);

      const std::string & user_login() const { return this->user_login_; }
      const std::string & name() const { return this->name_; }

    private:
      std::string user_login_;
      std::string name_;
    };

    class get_file_message_response
    {
    public:
      static const char message_type[];

      get_file_message_response(const std::shared_ptr<json_value> &);
      std::shared_ptr<json_value> serialize() const;

      get_file_message_response(
        const const_data_buffer & meta_info,
        const filename & tmp_file);

      const const_data_buffer & meta_info() const { return this->meta_info_; }
      const filename & tmp_file() const { return this->tmp_file_; }

    private:
      const_data_buffer meta_info_;
      filename tmp_file_;
    };*/
  };
}

#endif // __VDS_PROTOCOLS_CLIENT_MESSAGES_H_
