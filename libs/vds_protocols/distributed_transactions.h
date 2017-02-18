#ifndef __VDS_PROTOCOLS_DISTRIBUTED_TRANSACTIONS_H_
#define __VDS_PROTOCOLS_DISTRIBUTED_TRANSACTIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  /*
  class idistributed_transaction
  {
  public:
  };

  class distributed_transactions
  {
  public:

    struct message_base_data
    {
      //current primary replica
      int current_view_number;
    };

    //From client
    struct request_message : public message_base_data
    {
      //the operation the client wants to run
      std::string op;

      //
      int client_id;

      //the request-number assigned to the request.
      int request_number;
    };

    //to backup
    struct prepare_message : public message_base_data
    {
      std::string client_message;

      int request_number;

      int commit_number;
    };

    //to primary
    struct prepare_ok_message : public message_base_data
    {
      int request_number;

      int view_id;
    };

    //to client
    struct reply_message : public message_base_data
    {
      //the number the client provided in the request
      int request_number;

      //the result
      std::string result;
    };

    //to backup
    struct commit_message : public message_base_data
    {
      int commit_number;
    };

    //to all replicas
    struct start_view_change_message : public message_base_data
    {
      int view_id;
    };

    //to primary
    struct do_view_change_message : public message_base_data
    {
      int log;

      //the view number of the latest view in which its status was normal,
      int last_primary_view;

      int op_number;

      int commit_number;
    };

  private:

    class common_data
    {
    public:

    protected:
      //current primary replica
      int view_number_;
    };

    //Viewstamped Replication Revisited
    class server_code : public common_data
    {
    public:
      server_code();

      void process_request(request_message request);
      void process_prepare(prepare_message prepare);
      void process_prepare_ok(prepare_ok_message prepare_ok);

      void start_view_change();
      void process_start_view_change(start_view_change_message start_view_change);

    private:
      enum Status
      {
        Follower,
        Candidate,
        Leader
      };

      enum State
      {
        primary,
        backup
      };

      struct client_info
      {
        int last_request_number;
        std::string last_respose;
      };

      struct log_entry
      {
        int op_number;
        std::string message;

        int commit_count;
      };

      Status status_;
      State state_;
      int view_id_;
      std::map<int / * client_id * /, client_info> clients_;

      //The most recently received request, initially 0
      int op_number_;
      std::map<int / *op_number* /, log_entry> log_;

      int current_view_number_;
      int last_request_number_;
      int last_commit_number_;

      int last_unbroken_op_number_;

      int min_commit_count_;
    };

    class client_code : public common_data
    {
    public:

    private:
      int clinet_id_;
      int request_number_;
    };

  };

*/
}


#endif // __VDS_PROTOCOLS_DISTRIBUTED_TRANSACTIONS_H_
