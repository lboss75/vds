#ifndef __VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_
#define __VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <string>
#include "binary_serialize.h"
#include "const_data_buffer.h"
#include "transaction_id.h"

namespace vds {
  namespace transactions {
    class payment_transaction {
    public:
      static const transaction_id message_id = transaction_id::payment_transaction;

      const_data_buffer issuer;
      std::string currency;
      const_data_buffer source_transaction;
      const_data_buffer source_wallet;
      const_data_buffer target_wallet;
      uint64_t value;
      std::string payment_type;
      std::string notes;
      const_data_buffer signature;

      template <typename visitor_t>
      void visit(visitor_t & v) {
        v(
          issuer,
          currency,
          source_transaction,
          source_wallet,
          target_wallet,
          value,
          payment_type,
          notes,
          signature);
      }

      static expected<const_data_buffer> signature_data(
        const const_data_buffer & issuer,
        const std::string & currency,
        const const_data_buffer & source_transaction,
        const const_data_buffer & source_wallet,
        const const_data_buffer & target_wallet,
        uint64_t value,
        const std::string & payment_type,
        const std::string & notes);
      
      expected<const_data_buffer> signature_data() const {
        return signature_data(
          this->issuer,
          this->currency,
          this->source_transaction,
          this->source_wallet,
          this->target_wallet,
          this->value,
          this->payment_type,
          this->notes);
      }
    };

    class asset_issue_transaction {
    public:
      static const transaction_id message_id = transaction_id::asset_issue_transaction;

      const_data_buffer issuer;
      const_data_buffer wallet_id;
      std::string currency;
      uint64_t value;
      const_data_buffer signature;

      template <typename visitor_t>
      void visit(visitor_t & v) {
        v(
          issuer,
          wallet_id,
          currency,
          value,
          signature);
      }
      static expected<const_data_buffer> signature_data(
        const const_data_buffer & issuer,
        const const_data_buffer & wallet_id,
        const std::string & currency,
        uint64_t value);

      expected<const_data_buffer> signature_data() const {
        return signature_data(
          this->issuer,
          this->wallet_id,
          this->currency,
          this->value);
      }
    };

    class create_wallet_transaction {
    public:
      static const transaction_id message_id = transaction_id::create_wallet_transaction;

      const_data_buffer id;
      const_data_buffer public_key;

      template <typename visitor_t>
      void visit(visitor_t & v) {
        v(
          id,
          public_key);
      }

    };

    class payment_request_transaction {
    public:
      static const transaction_id message_id = transaction_id::payment_request_transaction;

      const_data_buffer issuer;
      std::string currency;
      const_data_buffer target_wallet;
      uint64_t value;
      std::string payment_type;
      std::string notes;
      const_data_buffer signature;

      template <typename visitor_t>
      void visit(visitor_t & v) {
        v(
          issuer,
          currency,
          target_wallet,
          value,
          payment_type,
          notes,
          signature);
      }

      static expected<const_data_buffer> signature_data(
        const const_data_buffer & issuer,
        const std::string & currency,
        const const_data_buffer & target_wallet,
        uint64_t value,
        const std::string & payment_type,
        const std::string & notes);

      expected<const_data_buffer> signature_data() const {
        return signature_data(
          this->issuer,
          this->currency,
          this->target_wallet,
          this->value,
          this->payment_type,
          this->notes);
      }
    };


  }
}

#endif //__VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_
