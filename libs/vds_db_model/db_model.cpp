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
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			body BLOB NOT NULL)");

		t.execute("CREATE TABLE channel (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			channel_type INT NOT NULL)");

		t.execute("CREATE TABLE channel_admin(\
			id VARCHAR(64) NOT NULL,\
			member_id VARCHAR(64) NOT NULL,\
			CONSTRAINT pk_channel_admin PRIMARY KEY(id, member_id))");

    t.execute("CREATE TABLE channel_message(\
			id INTEGER PRIMARY KEY AUTOINCREMENT,\
      channel_id VARCHAR(64) NOT NULL,\
			cert_id VARCHAR(64) NOT NULL,\
			message BLOB NOT NULL)");

		t.execute("CREATE TABLE chunk_data (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			block_key BLOB NOT NULL,\
			padding INT NOT NULL,\
			block_data BLOB NOT NULL)");

		t.execute("CREATE TABLE user (\
			id VARCHAR(64) PRIMARY KEY NOT NULL,\
			private_key BLOB NOT NULL,\
			parent VARCHAR(64),\
			login VARCHAR(64),\
			password_hash BLOB NOT NULL,\
		  CONSTRAINT pk_user_login UNIQUE(login))");

		t.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
	}
}
