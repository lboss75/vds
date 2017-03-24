#ifndef __VDS_STORAGE_SERVER_DATABASE_H_
#define __VDS_STORAGE_SERVER_DATABASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _server_database;
  class cert;

  class server_database
  {
  public:
    server_database(const service_provider & sp);
    ~server_database();

    void start();
    void stop();
    
    void add_cert(const cert & record);
    std::unique_ptr<cert> find_cert(const std::string & object_name) const;

  private:
    _server_database * const impl_;
  };

}

#endif // __VDS_STORAGE_SERVER_DATABASE_H_
