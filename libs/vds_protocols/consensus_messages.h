#ifndef __VDS_PROTOCOLS_CONSENSUS_MESSAGES_H_
#define __VDS_PROTOCOLS_CONSENSUS_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace consensus_messages {
    class consensus_message_who_is_leader
    {
    public:
      static const char message_type[];

      consensus_message_who_is_leader(
        const std::string & source_id);

      consensus_message_who_is_leader(const json_value * value);

      std::unique_ptr<json_value> serialize() const;

      const std::string & source_id() const { return this->source_id_; }

    private:
      std::string source_id_;
    };

    class consensus_message_current_leader
    {
    public:
      static const char message_type[];

      consensus_message_current_leader(
        const std::string & leader_id);

      consensus_message_current_leader(const json_value * value);

      std::unique_ptr<json_value> serialize() const;

      const std::string & leader_id() const { return this->leader_id_; }

    private:
      std::string leader_id_;
    };

  }
}


#endif // __VDS_PROTOCOLS_CONSENSUS_MESSAGES_H_
