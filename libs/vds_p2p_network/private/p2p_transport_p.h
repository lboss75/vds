#ifndef __VDS_P2P_NETWORK_P2P_TRANSPORT_P_H_
#define __VDS_P2P_NETWORK_P2P_TRANSPORT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class _p2p_transport : public std::enable_shared_from_this<_p2p_transport> {
  public:
    _p2p_transport();
    ~_p2p_transport();

    continuous_buffer<const_data_buffer> & incoming();
    continuous_buffer<const_data_buffer> & outgoing();

  private:
    enum class send_state{
      bof = 0,
    };

/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |1|type |           ACK Seq. No.                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    ~                 Control Information Field                     ~
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
    enum class control_type : public uint32_t {
      Handshake =       1000b,//seq: version, info: mtu size
      Keep_alive =      1001b,//
      Acknowledgement = 1010b,//seq: last package
      Failed          = 1011b,//seq: last package, info: failed bits
    };

    uint32_t output_sequence_number_;
    uint32_t input_sequence_number_;

    timer timeout_timer_;
    uint16_t mtu_;
    send_state current_state_;


    void send(const const_data_buffer & data);
    void on_timeout();

    const_data_buffer next_message();
  };
}

#endif //__VDS_P2P_NETWORK_P2P_TRANSPORT_P_H_
