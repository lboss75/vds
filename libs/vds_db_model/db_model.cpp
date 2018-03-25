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

		t.execute("CREATE TABLE chunk_data (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			block_key BLOB NOT NULL)");

    t.execute("CREATE TABLE current_config (\
			id INTEGER PRIMARY KEY AUTOINCREMENT,\
			cert BLOB NOT NULL,\
      cert_key BLOB NOT NULL)");

		t.execute("CREATE TABLE well_known_node(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			addresses TEXT NOT NULL)");

		t.execute("CREATE TABLE transaction_log_record(\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
      channel_id VARCHAR(64) NOT NULL,\
      data BLOB NOT NULL,\
			state INTEGER NOT NULL,\
			order_no INTEGER NOT NULL)");

		t.execute("CREATE TABLE transaction_log_unknown_record(\
			id VARCHAR(64) NOT NULL,\
      channel_id VARCHAR(64) NOT NULL,\
      follower_id VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_transaction_log_unknown_record PRIMARY KEY(id,follower_id))");

    t.execute("CREATE TABLE chunk_replica_data(\
			id VARCHAR(64) NOT NULL,\
			replica INTEGER NOT NULL,\
			replica_data BLOB NOT NULL,\
			replica_hash VARCHAR(100) NOT NULL,\
			CONSTRAINT pk_chunk_replica_data PRIMARY KEY(id,replica))");

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

		t.execute("CREATE TABLE certificate_unknown(\
			id VARCHAR(64) PRIMARY KEY NOT NULL)");

		t.execute("INSERT INTO well_known_node(id, addresses) VALUES(\
									'3940754a-64dd-4491-9777-719315b36a67',\
									'udp://127.0.0.1:8050')");
		t.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
	}
}

vds::async_task<> vds::db_model::prepare_to_stop(const vds::service_provider &sp) {
  return this->db_.prepare_to_stop(sp);
}
