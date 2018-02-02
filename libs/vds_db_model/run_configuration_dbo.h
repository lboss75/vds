#ifndef __VDS_DB_MODEL_RUN_CONFIGURATION_DBO_H_
#define __VDS_DB_MODEL_RUN_CONFIGURATION_DBO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "guid.h"
#include "const_data_buffer.h"

namespace vds {
  namespace dbo {
    class run_configuration : public database_table {
    public:
      run_configuration()
          : database_table("run_configuration"),
            id(this, "id"),
            cert(this, "cert"),
            cert_private_key(this, "cert_private_key"),
            port(this, "port"),
            common_channel_id(this, "common_channel_id"),
		  common_channel_read_cert(this, "common_channel_read_cert"),
		  common_channel_pkey(this, "common_channel_pkey") {
      }

      database_column <guid> id;
      database_column <const_data_buffer> cert;
      database_column <const_data_buffer> cert_private_key;
      database_column<int> port;
      database_column <guid> common_channel_id;
	  database_column <const_data_buffer> common_channel_read_cert;
	  database_column <const_data_buffer> common_channel_pkey;
    };
  }

  class current_run_configuration : public service_provider::property_holder {
  public:
    current_run_configuration(
      const guid& id,
      const certificate& device_cert,
      const asymmetric_private_key& device_private_key,
      const uint16_t port,
      const guid& common_channel_id,
      const certificate& common_channel_read_cert,
      const asymmetric_private_key& common_channel_pkey)
      : id_(id),
        device_cert_(device_cert),
        device_private_key_(device_private_key),
        port_(port),
        common_channel_id_(common_channel_id),
        common_channel_read_cert_(common_channel_read_cert),
        common_channel_pkey_(common_channel_pkey) {
    }

    const guid & id() const {
      return id_;
    }

    const certificate & device_cert() const {
      return device_cert_;
    }

    const asymmetric_private_key & device_private_key() const {
      return device_private_key_;
    }

    uint16_t port() const {
      return port_;
    }

    const guid & common_channel_id() const {
      return common_channel_id_;
    }

    const certificate & common_channel_read_cert() const {
      return common_channel_read_cert_;
    }

    const asymmetric_private_key & common_channel_pkey() const {
      return common_channel_pkey_;
    }

  private:
    guid id_;
    certificate device_cert_;
    asymmetric_private_key device_private_key_;
    uint16_t port_;

    guid common_channel_id_;
    certificate common_channel_read_cert_;
    asymmetric_private_key common_channel_pkey_;
  };
}


#endif //__VDS_DB_MODEL_RUN_CONFIGURATION_DBO_H_
