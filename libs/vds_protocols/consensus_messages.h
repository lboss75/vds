#ifndef __VDS_PROTOCOLS_CONSENSUS_MESSAGES_H_
#define __VDS_PROTOCOLS_CONSENSUS_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class network_serializer;
  
  namespace consensus_messages {
    
    class consensus_message_who_is_leader
    {
    public:
      static const char message_type[];

      consensus_message_who_is_leader(
        const const_data_buffer & source_id);
      
      consensus_message_who_is_leader(
        const_data_buffer && source_id);

      consensus_message_who_is_leader(const json_value * value);

      std::unique_ptr<json_value> serialize() const;
      void serialize(network_serializer & s) const;

      const const_data_buffer & source_id() const { return this->source_id_; }

    private:
      const_data_buffer source_id_;
    };
    
    class consensus_message_leader_candidate
    {
    public:
      static const char message_type[];

      consensus_message_leader_candidate(
        const const_data_buffer & source_id);
      
      consensus_message_leader_candidate(
        const_data_buffer && source_id);

      consensus_message_leader_candidate(const json_value * value);

      std::unique_ptr<json_value> serialize() const;
      void serialize(network_serializer & s) const;

      const const_data_buffer & source_id() const { return this->source_id_; }

    private:
      const_data_buffer source_id_;
    };
    
    class consensus_message_new_leader
    {
    public:
      static const char message_type[];

      consensus_message_new_leader(
        const const_data_buffer & leader_id);

      consensus_message_new_leader(const json_value * value);

      void serialize(network_serializer & s) const;
      std::unique_ptr<json_value> serialize() const;

      const const_data_buffer & leader_id() const { return this->leader_id_; }

    private:
      const_data_buffer leader_id_;
    };

    class consensus_message_current_leader
    {
    public:
      static const char message_type[];

      consensus_message_current_leader(
        const_data_buffer && leader_id);
      
      consensus_message_current_leader(
        const const_data_buffer & leader_id);

      consensus_message_current_leader(const json_value * value);

      std::unique_ptr<json_value> serialize() const;

      const const_data_buffer & leader_id() const { return this->leader_id_; }

    private:
      const_data_buffer leader_id_;
    };

  }
}


#endif // __VDS_PROTOCOLS_CONSENSUS_MESSAGES_H_
