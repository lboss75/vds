#include "payment_transaction.h"

vds::expected<vds::const_data_buffer>
vds::transactions::payment_transaction::signature_data(
  const const_data_buffer & issuer,
  const std::string & currency,
  const const_data_buffer & source_transaction,
  const const_data_buffer & source_wallet,
  const const_data_buffer & target_wallet,
  uint64_t value,
  const std::string & payment_type,
  const std::string & notes)
{
  binary_serializer s;
  CHECK_EXPECTED(s << (uint8_t)message_id);
  CHECK_EXPECTED(s << issuer);
  CHECK_EXPECTED(s << currency);
  CHECK_EXPECTED(s << source_transaction);
  CHECK_EXPECTED(s << source_wallet);
  CHECK_EXPECTED(s << target_wallet);
  CHECK_EXPECTED(s << value);
  CHECK_EXPECTED(s << payment_type);
  CHECK_EXPECTED(s << notes);

  return s.move_data();
}

vds::expected<vds::const_data_buffer> vds::transactions::asset_issue_transaction::signature_data(
  const const_data_buffer & issuer,
  const const_data_buffer & wallet_id,
  const std::string & currency,
  uint64_t value)
{
  binary_serializer s;
  CHECK_EXPECTED(s << (uint8_t)message_id);
  CHECK_EXPECTED(s << issuer);
  CHECK_EXPECTED(s << wallet_id);
  CHECK_EXPECTED(s << currency);
  CHECK_EXPECTED(s << value);

  return s.move_data();
}
