#include "stdafx.h"
#include "db_model.h"

vds::async_task<> vds::db_model::async_transaction(const vds::service_provider &sp,
                                                   const std::function<void(vds::database_transaction &)> &handler) {
  return [this, sp, handler](const async_result<> & result){
    this->db_.async_transaction(sp, [handler, result](database_transaction & t)->bool{
      try {
        handler(t);
      }
      catch (const std::exception & ex){
        result.error(std::make_shared<std::runtime_error>(ex.what()));
        return true;
      }

      result.done();
      return true;
    });
  };
}

void vds::db_model::start(const service_provider & sp)
{
	filename db_filename(foldername(persistence::current_user(sp), ".vds"), "local.db");

	if (!file::exists(db_filename)) {
		this->db_.open(sp, db_filename);

		this->db_.sync_transaction(sp, [this](database_transaction & t) {
			this->migrate(t, 0);
			return true;
		});
	}
	else {
		this->db_.open(sp, db_filename);

		this->db_.sync_transaction(sp, [this](database_transaction & t) {
			auto st = t.parse("SELECT version FROM module WHERE id='kernel'");
			if (!st.execute()) {
				throw std::runtime_error("Database has been corrupted");
			}

			uint64_t db_version;
			st.get_value(0, db_version);

			this->migrate(t, db_version);

			return true;
		});
	}
}

void vds::db_model::stop(const service_provider & sp)
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


		t.execute("CREATE TABLE cert(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			body BLOB NOT NULL,\
			parent VARCHAR(64))");

		t.execute("CREATE TABLE cert_private_key(\
			id VARCHAR(64) NOT NULL,\
      owner_id VARCHAR(64) NOT NULL,\
			body BLOB NOT NULL,\
		  CONSTRAINT pk_cert_private_key PRIMARY KEY(id, owner_id))");

		t.execute("CREATE TABLE channel (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			channel_type INT NOT NULL,\
      name VARCHAR(64) NOT NULL,\
      read_cert VARCHAR(64) NOT NULL,\
      write_cert VARCHAR(64) NOT NULL)");

		t.execute("CREATE TABLE channel_admin(\
			id VARCHAR(64) NOT NULL,\
			member_id VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_channel_admin PRIMARY KEY(id, member_id))");

    t.execute("CREATE TABLE channel_message(\
			id INTEGER PRIMARY KEY AUTOINCREMENT,\
      channel_id VARCHAR(64) NOT NULL,\
      message_id INT NOT NULL,\
			read_cert_id VARCHAR(64) NOT NULL,\
      write_cert_id VARCHAR(64) NOT NULL,\
			message BLOB NOT NULL,\
			signature BLOB NOT NULL)");

		t.execute("CREATE TABLE chunk_data (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			block_key BLOB NOT NULL,\
			block_data BLOB NOT NULL)");

		t.execute("CREATE TABLE user (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			cert_id VARCHAR(64) NOT NULL,\
			private_key BLOB NOT NULL,\
			parent VARCHAR(64),\
			login VARCHAR(64),\
			password_hash BLOB NOT NULL,\
		  CONSTRAINT fk_user_login UNIQUE(login))");

    t.execute("CREATE TABLE run_configuration (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			cert_id VARCHAR(64) NOT NULL,\
      cert_private_key BLOB NOT NULL,\
			port INTEGER,\
			common_channel_id VARCHAR(64) NOT NULL)");

		t.execute("CREATE TABLE well_known_node(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			addresses TEXT NOT NULL)");

		t.execute("CREATE TABLE transaction_log_record(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
      data BLOB NOT NULL,\
			state INTEGER NOT NULL)");

		t.execute("CREATE TABLE transaction_log_unknown_record(\
			id VARCHAR(64) NOT NULL,\
      follower_id VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_transaction_log_unknown_record PRIMARY KEY(id,follower_id))");

    t.execute("CREATE TABLE chunk_replica(\
			id VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
			replica_hash BLOB NOT NULL,\
			CONSTRAINT pk_chunk_replica PRIMARY KEY(id,replica))");

		t.execute("CREATE TABLE chunk_map(\
			id VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
			device VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_chunk_map PRIMARY KEY(id,replica,device))");

    t.execute("CREATE TABLE channel_member(\
			channel_id VARCHAR(64) NOT NULL,\
			user_cert_id VARCHAR(64) NOT NULL,\
			member_type INTEGER NOT NULL,\
			CONSTRAINT pk_channel_member PRIMARY KEY(channel_id,user_cert_id))");

		t.execute("INSERT INTO well_known_node(id, addresses) VALUES(\
									'3940754a-64dd-4491-9777-719315b36a67',\
									'udp://127.0.0.1:8050')");
		t.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
	}
}

vds::async_task<> vds::db_model::prepare_to_stop(const vds::service_provider &sp) {
  return this->db_.prepare_to_stop(sp);
}
