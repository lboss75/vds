#include "stdafx.h"
#include "db_model.h"

vds::async_task<vds::expected<void>> vds::db_model::async_transaction(lambda_holder_t<expected<void>, class database_transaction &> handler) {
  return this->db_.async_transaction([h = std::move(handler)](database_transaction & t)->expected<bool> {
    CHECK_EXPECTED(h(t));
    return true;
  });
}

vds::async_task<vds::expected<void>> vds::db_model::async_read_transaction(
  lambda_holder_t<expected<void>, class database_read_transaction &> handler) {
  return this->db_.async_read_transaction(std::move(handler));
}

vds::expected<void> vds::db_model::start(const service_provider * sp)
{
  this->sp_ = sp;
  GET_EXPECTED(folder, persistence::current_user(sp));
  CHECK_EXPECTED(folder.create());

	filename db_filename(folder, "local.db");

	if (!file::exists(db_filename)) {
    CHECK_EXPECTED(this->db_.open(sp, db_filename));

    CHECK_EXPECTED(this->db_.async_transaction([this](database_transaction & t) -> expected<bool>{
      CHECK_EXPECTED(this->migrate(t, 0));
			return true;
		}).get());
	}
	else {
    CHECK_EXPECTED(this->db_.open(sp, db_filename));

    CHECK_EXPECTED(this->db_.async_transaction([this](database_transaction & t) -> expected<bool> {
      GET_EXPECTED(st, t.parse("SELECT version FROM module WHERE id='kernel'"));
			if (!st.execute()) {
				return vds::make_unexpected<std::runtime_error>("Database has been corrupted");
			}

			int64_t db_version;
			st.get_value(0, db_version);

      CHECK_EXPECTED(this->migrate(t, db_version));

			return true;
		}).get());
	}

  return expected<void>();
}

vds::expected<void> vds::db_model::stop()
{
	return this->db_.close();
}

vds::expected<void> vds::db_model::migrate(
	database_transaction & t,
	int64_t db_version)
{
	if (1 > db_version) {

    static const char * commands[] = {

      "CREATE TABLE module(\
		  id VARCHAR(64) PRIMARY KEY NOT NULL,\
		  version INTEGER NOT NULL,\
		  installed DATETIME NOT NULL)",

      "CREATE TABLE chunk_data (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
      data_hash VARCHAR(64) NOT NULL,\
			last_access INTEGER NOT NULL)",

      "CREATE TABLE well_known_node(\
			address VARCHAR(64) PRIMARY KEY NOT NULL,\
			last_connect DATETIME NOT NULL)",

      "CREATE TABLE current_config (\
			node_id VARCHAR(64) PRIMARY KEY NOT NULL,\
			public_key BLOB NOT NULL,\
      private_key BLOB NOT NULL)",
      
      "CREATE TABLE node_storage_dbo (\
			storage_id VARCHAR(64) PRIMARY KEY NOT NULL,\
			local_path VARCHAR(512) NOT NULL,\
      owner_id VARCHAR(64) NOT NULL,\
      reserved_size INTEGER NOT NULL)",

      "CREATE TABLE chunk_tmp_data (\
			object_id VARCHAR(64) PRIMARY KEY NOT NULL,\
			last_sync INTEGER NOT NULL)",


      "CREATE TABLE channel_local_cache(\
      channel_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      last_sync INTEGER NOT NULL)",

      "CREATE TABLE transaction_log_hierarchy(\
			id VARCHAR(64) NOT NULL,\
      follower_id VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_transaction_log_hierarchy PRIMARY KEY(id,follower_id))",

      "CREATE TABLE transaction_log_record(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
      data BLOB NOT NULL,\
			state INTEGER NOT NULL,\
      consensus BIT NOT NULL,\
			order_no INTEGER NOT NULL,\
      time_point INTEGER NOT NULL)",

      "CREATE TABLE transaction_log_balance(\
			id VARCHAR(64) NOT NULL,\
      owner VARCHAR(64) NOT NULL,\
      source VARCHAR(64) NOT NULL,\
      balance INTEGER NOT NULL,\
			CONSTRAINT pk_transaction_log_balance PRIMARY KEY(id,owner,source))",

      "CREATE TABLE transaction_log_vote_request(\
			id VARCHAR(64) NOT NULL,\
      owner VARCHAR(64) NOT NULL,\
      approved BIT NOT NULL,\
      new_member BIT NOT NULL,\
			CONSTRAINT pk_transaction_log_vote_request PRIMARY KEY(id,owner))",

      "CREATE TABLE channel_message_dbo(\
			id INTEGER PRIMARY KEY AUTOINCREMENT,\
      block_id VARCHAR(64) NOT NULL,\
      channel_id VARCHAR(64) NOT NULL,\
      read_id VARCHAR(64) NOT NULL,\
      write_id VARCHAR(64) NOT NULL,\
      crypted_key VARCHAR(64) NOT NULL,\
      crypted_data BLOB NOT NULL,\
      signature VARCHAR(64) NOT NULL)",

      "CREATE TABLE chunk (\
			object_id VARCHAR(64) PRIMARY KEY NOT NULL,\
			last_sync INTEGER NOT NULL)",

      "CREATE TABLE chunk_replica_data(\
			object_hash VARCHAR(64) NOT NULL,\
      replica INTEGER NOT NULL,\
			replica_hash VARCHAR(64) NOT NULL,\
      replica_size INTEGER NOT NULL,\
      CONSTRAINT pk_chunk_replica_data PRIMARY KEY(object_hash,replica),\
      CONSTRAINT idx_chunk_replica_data_replica_hash UNIQUE(replica_hash))",

      "CREATE TABLE local_data_dbo(\
			storage_id VARCHAR(64) NOT NULL,\
			replica_hash VARCHAR(64) NOT NULL,\
      replica_size INTEGER NOT NULL,\
			owner VARCHAR(64) NOT NULL,\
			storage_path VARCHAR(256) NOT NULL,\
      last_access INTEGER NOT NULL,\
      CONSTRAINT pk_local_data_dbo PRIMARY KEY(replica_hash,owner))",

      "CREATE TABLE member_user_dbo(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			public_key VARCHAR(64) NOT NULL)",

      "CREATE TABLE chunk_map(\
			id VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
      distance INTEGER NOT NULL,\
			device VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_chunk_map PRIMARY KEY(id,replica,device))",


      "CREATE TABLE device_record(\
			node_id VARCHAR(64) NOT NULL,\
      storage_path VARCHAR(254) NOT NULL,\
			data_hash VARCHAR(64) NOT NULL,\
			data_size INTEGER NOT NULL,\
      CONSTRAINT pk_device_record PRIMARY KEY(node_id,storage_path,data_hash))",

      "CREATE TABLE sync_local_queue(\
			local_index INTEGER PRIMARY KEY AUTOINCREMENT,\
      object_id VARCHAR(64) NOT NULL,\
      message_type INTEGER NOT NULL,\
      member_node VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
      last_send INTEGER NOT NULL)",

      "CREATE TABLE sync_member(\
			object_id VARCHAR(64) NOT NULL,\
      member_node VARCHAR(64) NOT NULL,\
      voted_for VARCHAR(64) NOT NULL,\
      generation INTEGER NOT NULL,\
			current_term INTEGER NOT NULL,\
			commit_index INTEGER NOT NULL,\
			last_applied INTEGER NOT NULL,\
      delete_index INTEGER NOT NULL,\
      last_activity INTEGER NOT NULL,\
      CONSTRAINT pk_sync_member PRIMARY KEY(object_id,member_node))",

      "CREATE TABLE sync_message(\
			object_id VARCHAR(64) NOT NULL,\
      generation INTEGER NOT NULL,\
			current_term INTEGER NOT NULL,\
			message_index INTEGER NOT NULL,\
			message_type INTEGER NOT NULL,\
      member_node VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
      source_node VARCHAR(64) NOT NULL,\
			source_index INTEGER NOT NULL,\
      CONSTRAINT pk_sync_message PRIMARY KEY(object_id,generation,current_term,message_index))",

      "CREATE UNIQUE INDEX fk_sync_message ON sync_message(object_id,generation,current_term,source_node,source_index)",

      "CREATE TABLE chunk_replica_map(\
			replica_hash VARCHAR(64) NOT NULL,\
			node VARCHAR(64) NOT NULL,\
			last_access INTEGER NOT NULL,\
      CONSTRAINT pk_chunk_replica_map PRIMARY KEY(replica_hash,node))",

      "CREATE TABLE sync_state(\
			object_id VARCHAR(64) PRIMARY KEY NOT NULL,\
			object_size INTEGER NOT NULL,\
			state INTEGER NOT NULL,\
			next_sync INTEGER NOT NULL)",

      "CREATE TABLE node_info_dbo(\
			node_id VARCHAR(64) PRIMARY KEY NOT NULL,\
			public_key VARCHAR(1853) NOT NULL,\
      last_activity INTEGER NOT NULL)",

      "CREATE TABLE wallet_dbo(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			public_key VARCHAR(1853) NOT NULL)",

      "CREATE TABLE datacoin_balance_dbo(\
			owner VARCHAR(64) NOT NULL,\
			issuer VARCHAR(64) NOT NULL,\
			currency VARCHAR(64) NOT NULL,\
			source_transaction VARCHAR(64) NOT NULL,\
			confirmed_balance INTEGER NOT NULL,\
			proposed_balance INTEGER NOT NULL,\
      CONSTRAINT pk_datacoin_balance_dbo PRIMARY KEY(owner,issuer,currency,source_transaction))",


      "INSERT INTO well_known_node(address, last_connect) VALUES(\
									'udp://localhost:8050',\
									datetime('now'))",

      "INSERT INTO well_known_node(address, last_connect) VALUES(\
									'udp://vds.iv-soft.ru:8050',\
									datetime('now'))",

      "INSERT INTO well_known_node(address, last_connect) VALUES(\
									'udp://homeserver.iv-soft.ru:8050',\
									datetime('now'))",

        "INSERT INTO well_known_node(address, last_connect) VALUES(\
									'udp://46.21.68.42:8050',\
									datetime('now'))",

       "INSERT INTO well_known_node(address, last_connect) VALUES(\
				'udp6://::ffff:192.168.0.132:8050',\
				datetime('now'))",

      "INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))"
    };

    for(size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
      CHECK_EXPECTED(t.execute(commands[i]));
    }
	}

  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::db_model::prepare_to_stop() {
  return this->db_.prepare_to_stop();
}
