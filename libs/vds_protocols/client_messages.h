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
        const data_buffer & password_hash
      );
      
      certificate_and_key_request(const json_value *);
      std::unique_ptr<json_value> serialize() const;

      const std::string & object_name() const { return this->object_name_; }
      const data_buffer & password_hash() const { return this->password_hash_; }

    private:
      std::string object_name_;
      data_buffer password_hash_;
    };

    class certificate_and_key_response
    {
    public:
      static const char message_type[];

      certificate_and_key_response(
        const std::string & certificate_body,
        const std::string & private_key_body
      );

      certificate_and_key_response(const json_value *);
      std::unique_ptr<json_value> serialize() const;

      const std::string & certificate_body() const { return this->certificate_body_; }
      const std::string & private_key_body() const { return this->private_key_body_; }

    private:
      std::string certificate_body_;
      std::string private_key_body_;
    };

    class register_server_request
    {
    public:
      static const char message_type[];

      register_server_request(
        const std::string & request_id,
        const std::string & certificate_body
      );

      register_server_request(const json_value *);
      std::unique_ptr<json_value> serialize() const;

      const std::string & request_id() const { return this->request_id_; }
      const std::string & certificate_body() const { return this->certificate_body_; }

    private:
      std::string request_id_;
      std::string certificate_body_;
    };

    class register_server_response
    {
    public:
      static const char message_type[];

      register_server_response(
        const std::string & request_id,
        const std::string & error
      );

      register_server_response(const json_value *);
      std::unique_ptr<json_value> serialize() const;

      const std::string & request_id() const { return this->request_id_; }
      const std::string & error() const { return this->error_; }

    private:
      std::string request_id_;
      std::string error_;
    };
    //////////////////////////////////////////////////////////////
    class put_file_message
    {
    public:
      static const char message_type[];

      put_file_message(const json_value *);
      std::unique_ptr<json_value> serialize() const;

      put_file_message(
        const std::string & request_id,
        const std::string & user_login,
        const filename & tmp_file);

      const std::string & request_id() const { return this->request_id_; }
      const std::string & user_login() const { return this->user_login_; }
      const filename & tmp_file() const { return this->tmp_file_; }

    private:
      std::string request_id_;
      std::string user_login_;
      filename tmp_file_;
    };

    class put_file_message_response
    {
    public:
      static const char message_type[];

      put_file_message_response(const json_value *);
      std::unique_ptr<json_value> serialize() const;

      put_file_message_response(
        const std::string & request_id,
        const std::string & error);

      const std::string & request_id() const { return this->request_id_; }
      const std::string & error() const { return this->error_; }

    private:
      std::string request_id_;
      std::string error_;
    };

    class get_file_message_request
    {
    public:
      static const char message_type[];

      get_file_message_request(const json_value *);
      std::unique_ptr<json_value> serialize() const;

      get_file_message_request(
        const std::string & request_id,
        const std::string & user_login);

      const std::string & request_id() const { return this->request_id_; }
      const std::string & user_login() const { return this->user_login_; }

    private:
      std::string request_id_;
      std::string user_login_;
    };

    class get_file_message_response
    {
    public:
      static const char message_type[];

      get_file_message_response(const json_value *);
      std::unique_ptr<json_value> serialize() const;

      get_file_message_response(
        const std::string & request_id,
        const std::string & error,
        const filename & tmp_file);

      const std::string & request_id() const { return this->request_id_; }
      const std::string & error() const { return this->error_; }
      const filename & tmp_file() const { return this->tmp_file_; }

    private:
      std::string request_id_;
      std::string error_;
      filename tmp_file_;
    };
  };
}

#endif // __VDS_PROTOCOLS_CLIENT_MESSAGES_H_
