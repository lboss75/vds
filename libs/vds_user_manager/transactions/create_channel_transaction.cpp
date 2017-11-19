//
// Created by vadim on 19.11.17.
//

#include "create_channel_transaction.h"

vds::create_channel_transaction::create_channel_transaction(
    const vds::guid &id,
    const vds::guid &owner_id)
: id_(id), owner_id_(owner_id){

}
