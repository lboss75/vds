#include "stdafx.h"
#include "db_model.h"

vds::async_task<void> vds::db_model::async_transaction(const std::function<void(vds::database_transaction &)> &handler) {
  return this->db_.async_transaction([handler](database_transaction & t)->bool{
      handler(t);
      return true;
  });
}

vds::async_task<void> vds::db_model::async_read_transaction(
    
    const std::function<void(vds::database_read_transaction &)> &handler) {
  return this->db_.async_read_transaction(handler);
}

void vds::db_model::start(const service_provider * sp)
{
	filename db_filename(foldername(persistence::current_user(sp), ".vds"), "local.db");

	if (!file::exists(db_filename)) {
		this->db_.open(sp, db_filename);

		this->db_.async_transaction([this](database_transaction & t) {
			this->migrate(t, 0);
			return true;
		}).get();
	}
	else {
		this->db_.open(sp, db_filename);

		this->db_.async_transaction([this](database_transaction & t) {
			auto st = t.parse("SELECT version FROM module WHERE id='kernel'");
			if (!st.execute()) {
				throw std::runtime_error("Database has been corrupted");
			}

			uint64_t db_version;
			st.get_value(0, db_version);

			this->migrate(t, db_version);

			return true;
		}).get();
	}
}

void vds::db_model::stop()
{
	this->db_.close();
}

void vds::db_model::migrate(
	database_transaction & t,
	uint64_t db_version)
{
	if (1 > db_version) {

		t.execute("CREATE TABLE module(\
		  id VARCHAR(64) PRIMARY KEY NOT NULL,\
		  version INTEGER NOT NULL,\
		  installed DATETIME NOT NULL)");

		t.execute("CREATE TABLE chunk_data (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
      data_hash VARCHAR(64) NOT NULL,\
			last_access INTEGER NOT NULL)");

		t.execute("CREATE TABLE well_known_node(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			addresses TEXT NOT NULL)");

    t.execute("CREATE TABLE current_config (\
			id INTEGER PRIMARY KEY AUTOINCREMENT,\
			cert BLOB NOT NULL,\
      cert_key BLOB NOT NULL)");

		t.execute("CREATE TABLE transaction_log_record(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
      data BLOB NOT NULL,\
			state INTEGER NOT NULL,\
			order_no INTEGER NOT NULL,\
      time_point INTEGER NOT NULL,\
      state_data BLOB NULL)");

    t.execute("CREATE TABLE channel_local_cache(\
      channel_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      last_sync INTEGER NOT NULL)");

		t.execute("CREATE TABLE transaction_log_unknown_record(\
			id VARCHAR(64) NOT NULL,\
      follower_id VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_transaction_log_unknown_record PRIMARY KEY(id,follower_id))");

    t.execute("CREATE TABLE chunk (\
			object_id VARCHAR(64) PRIMARY KEY NOT NULL,\
			last_sync INTEGER NOT NULL)");

    t.execute("CREATE TABLE chunk_replica_data(\
			object_id VARCHAR(64) NOT NULL,\
      replica INTEGER NOT NULL,\
			replica_hash VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_chunk_replica_data PRIMARY KEY(object_id,replica))");

    t.execute("CREATE TABLE local_data_dbo(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			data BLOB NOT NULL,\
			create_time VARCHAR(64) NOT NULL,\
      is_new INTEGER NOT NULL)");

		t.execute("CREATE TABLE chunk_map(\
			id VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
      distance INTEGER NOT NULL,\
			device VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_chunk_map PRIMARY KEY(id,replica,device))");

    t.execute("CREATE TABLE certificate_chain(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			cert BLOB NOT NULL,\
			parent VARCHAR(64) NOT NULL)");

		t.execute("CREATE TABLE device_config(\
			node_id VARCHAR(64) NOT NULL,\
			local_path VARCHAR(254) NOT NULL,\
			owner_id VARCHAR(64) NOT NULL,\
			name VARCHAR(64) NOT NULL,\
			reserved_size INTEGER NOT NULL,\
      CONSTRAINT pk_device_config PRIMARY KEY(node_id,local_path))");

    t.execute("CREATE TABLE device_record(\
			node_id VARCHAR(64) NOT NULL,\
      storage_path VARCHAR(254) NOT NULL,\
			local_path VARCHAR(254) NOT NULL,\
			data_hash VARCHAR(64) NOT NULL,\
			data_size INTEGER NOT NULL,\
      CONSTRAINT pk_device_record PRIMARY KEY(node_id,storage_path,data_hash))");

		t.execute("CREATE TABLE certificate_unknown(\
			id VARCHAR(64) PRIMARY KEY NOT NULL)");

		t.execute("CREATE TABLE register_request(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
      name VARCHAR(64) NOT NULL,\
      email VARCHAR(64) NOT NULL,\
			data BLOB NOT NULL,\
      create_time INTEGER NOT NULL)");

		t.execute("CREATE TABLE incoming_request_dbo(\
			id INTEGER PRIMARY KEY AUTOINCREMENT,\
      owner VARCHAR(64) NOT NULL,\
      name VARCHAR(64) NOT NULL,\
      email VARCHAR(64) NOT NULL,\
			data BLOB NOT NULL,\
      create_time INTEGER NOT NULL)");

    t.execute("CREATE TABLE sync_local_queue(\
			local_index INTEGER PRIMARY KEY AUTOINCREMENT,\
      object_id VARCHAR(64) NOT NULL,\
      message_type INTEGER NOT NULL,\
      member_node VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
      last_send INTEGER NOT NULL)");

    t.execute("CREATE TABLE sync_member(\
			object_id VARCHAR(64) NOT NULL,\
      member_node VARCHAR(64) NOT NULL,\
      voted_for VARCHAR(64) NOT NULL,\
      generation INTEGER NOT NULL,\
			current_term INTEGER NOT NULL,\
			commit_index INTEGER NOT NULL,\
			last_applied INTEGER NOT NULL,\
      delete_index INTEGER NOT NULL,\
      last_activity INTEGER NOT NULL,\
      CONSTRAINT pk_sync_member PRIMARY KEY(object_id,member_node))");

    t.execute("CREATE TABLE sync_message(\
			object_id VARCHAR(64) NOT NULL,\
      generation INTEGER NOT NULL,\
			current_term INTEGER NOT NULL,\
			message_index INTEGER NOT NULL,\
			message_type INTEGER NOT NULL,\
      member_node VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
      source_node VARCHAR(64) NOT NULL,\
			source_index INTEGER NOT NULL,\
      CONSTRAINT pk_sync_message PRIMARY KEY(object_id,generation,current_term,message_index))");

    t.execute("CREATE UNIQUE INDEX fk_sync_message ON sync_message(source_node,source_index)");

    t.execute("CREATE TABLE chunk_replica_map(\
			object_id VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
			node VARCHAR(64) NOT NULL,\
			last_access INTEGER NOT NULL,\
      CONSTRAINT pk_chunk_replica_map PRIMARY KEY(object_id,replica,replica,node))");

    t.execute("CREATE TABLE sync_state(\
			object_id VARCHAR(64) PRIMARY KEY NOT NULL,\
			object_size INTEGER NOT NULL,\
			state INTEGER NOT NULL,\
			next_sync INTEGER NOT NULL)");

    t.execute("INSERT INTO well_known_node(id, addresses) VALUES(\
									'local',\
									'udp://localhost:8050')");
    t.execute("INSERT INTO well_known_node(id, addresses) VALUES(\
									'vds.iv-soft.ru',\
									'udp://178.207.91.252:8050')");
    t.execute("INSERT INTO well_known_node(id, addresses) VALUES(\
									'server',\
									'udp://46.21.68.42:8050')");
    t.execute("INSERT INTO well_known_node(id, addresses) VALUES(\
									'192.168.0.171',\
									'udp://192.168.0.171:8050')");
    t.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
	}
}

vds::async_task<void> vds::db_model::prepare_to_stop() {
  return this->db_.prepare_to_stop();
}
