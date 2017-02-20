#ifndef __VDS_PROTOCOLS_INSTALL_NODE_MESSAGES_H_
#define __VDS_PROTOCOLS_INSTALL_NODE_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class install_node_prepare
  {
  public:
    static const char message_type[];

    std::string user_id;
    std::string password_hash;
    std::string request_id;

    json_object * serialize();
    void deserialize(const json_object * source);
  };

  class install_node_prepared
  {
  public:
    static const char message_type[];

    std::string user_id;
    std::string user_certificate;
    std::string user_private_key;
    std::string request_id;
    std::string new_certificate_serial;

    json_object * serialize() const;
    void deserialize(const json_object * source);
  };

  class install_node_register
  {
  public:
    static const char message_type[];

    std::string user_id;
    std::string user_certificate;
  };

  class install_node_registered
  {
  public:
    static const char message_type[];

    std::string user_id;
    std::string certificate_serial;
    std::string node_id;
  };

}


#endif // __VDS_PROTOCOLS_INSTALL_NODE_MESSAGES_H_
