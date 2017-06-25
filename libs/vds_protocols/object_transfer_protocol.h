#ifndef __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_
#define __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _object_transfer_protocol;
  
  class object_transfer_protocol
  {
  public:
    object_transfer_protocol();
    ~object_transfer_protocol();

  private:
    _object_transfer_protocol * const impl_;
  };
  
  class route_hop
  {
  public:
    route_hop(
      const guid & server_id,
      const std::string & return_address);
    
    const guid & server_id() const { return this->server_id_; }
    const std::string & return_address() const { return this->return_address_; }
    
  private:
    guid server_id_;
    std::string return_address_;
  };
  
  class object_request
  {
  public:
    object_request();
    
    const guid & server_id() const { return this->server_id_; }
    uint64_t index() const { return this->index_; }
    
  private:
    guid server_id_;
    uint64_t index_;
    
    std::vector<uint16_t> collected_replicas_;
    std::list<route_hop> hops_;
  };
  
}

#endif // __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_
