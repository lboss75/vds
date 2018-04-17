#ifndef __LOG_PARSER_FILTER_PARSER_H_
#define __LOG_PARSER_FILTER_PARSER_H_

#include <iostream>
#include <cstring>
#include "linked_list.h"
#include "filter_statemachine.h"

class filter_parser {
public:

  //Create state machine based on the filter
  static bool parse_filter(const char * filter, filter_statemachine & result){
    linked_list<parser_state_t> states;

    if(!init_states(states, filter)) {
      return false;
    }

    if(!generate_states(states)) {
      return false;
    }

    if(!pack_result(states, result)) {
      return false;
    }

    return true;
  }


private:

  //Current position in the filter
  class filter_state_t {
  public:
    //Filter pattern ?
    static const int ANY_CHAR = 256;

    //Filter pattern *
    static const int ANY_CHAR_MULTIPLE = 257;

    filter_state_t(const char * filter)
    : filter_(filter){
    }

    const char * filter() const {
      return filter_;
    }

    //Next symbol expected by the filter
    int next_char() const {
      switch (*filter_) {
      case '*':
        return ANY_CHAR_MULTIPLE;

      case '?':
        return  ANY_CHAR;

      default:
        return static_cast<unsigned char>(*filter_);
      }
    }

  private:
    const char * filter_;
  };

  //Helper class to create new parser state
  class parser_state_builder_t {
  public:
    parser_state_builder_t() {
    }

    parser_state_builder_t(parser_state_builder_t && origin) = delete;

    //Add filter state if it is not exists
    bool add_state(const char * filter_state) {
      trim_left(filter_state);

      if(!add(filter_state)) {
        return false;
      }

      //Filter that starts with *rest generates two states *rest and rest
      if ('*' == *filter_state) {
        return add(filter_state + 1);
      }
      else {
        return true;
      }
    }

    const linked_list<filter_state_t> & items() const {
      return items_;
    }

    linked_list<filter_state_t> & items() {
      return items_;
    }

    //Trim leads *
    static void trim_left(const char * & filter) {
      while('*' == filter[0] && '*' == filter[1]) {
        ++filter;
      }
    }

    //Generate new states by applying symbol to the filter state
    bool apply_char(int next_char, const filter_state_t & filter_state) {
      switch (filter_state.next_char()) {
        case filter_state_t::ANY_CHAR: {
          if (!add_state(filter_state.filter() + 1)) {
            return false;
          }

          break;
        }

        case filter_state_t::ANY_CHAR_MULTIPLE: {
          if(!add_state(filter_state.filter())){
            return false;
          }

          if (!add_state(filter_state.filter() + 1)) {
            return false;
          }

          break;
        }

        default: {
          if(next_char == filter_state.next_char()){
            if(!add_state(filter_state.filter() + 1)){
              return false;
            }
          }

          break;
        }
      }

      return true;
    }

  private:
    linked_list<filter_state_t> items_;

    //checks the state exists and add if it is not
    bool add(const char * filter_state) {
      auto p = items_.head();
      if (nullptr == p) {
        return items_.add(filter_state_t(filter_state));
      }
      for(;;) {
        if (p->value().filter() == filter_state) {
          return true;//Already exists
        }
        const auto next = p->next();
        if (nullptr == next) {
          return p->add(filter_state_t(filter_state));
        }
        else {
          p = next;
        }
      }
    }
  };

  //Parser state
  class parser_state_t {
  public:
    parser_state_t () {
    }

    parser_state_t(parser_state_builder_t && origin)
    : items_((linked_list<filter_state_t> &&)origin.items()){
    }

    //Calculate transitions by applying possible symbols
    bool build_transactions(linked_list<parser_state_t> & states) {
      //Crear transitions table
      for (size_t i = 0; i < sizeof(transactions_) / sizeof(transactions_[0]); ++i) {
        transactions_[i] = filter_statemachine::INVALID_STATE;
      }

      //transition by 0 string terminator
      if(can_final()) {
        transactions_[0] = filter_statemachine::FINAL_STATE;
      }

      //transitions by specified in the filter symbols
      for (auto p = items_.head(); nullptr != p; p = p->next()) {
        auto next_char = p->value().next_char();
        if(0 == next_char || next_char > 255){
          continue;//Only real symbols
        }

        if(filter_statemachine::INVALID_STATE != transactions_[next_char]){
          continue;//Already calculated
        }

        if(!next_state(states, next_char, transactions_[next_char])){
          return false;
        }
      }

      //apply all other symbols
      int default_state = filter_statemachine::INVALID_STATE;
      if(!next_state(states, filter_state_t::ANY_CHAR, default_state)){
        return false;
      }

      if(filter_statemachine::INVALID_STATE != default_state){
        for (size_t i = 1; i < sizeof(transactions_) / sizeof(transactions_[0]); ++i) {
          if (filter_statemachine::INVALID_STATE == transactions_[i]) {
            transactions_[i] = default_state;
          }
        }
      }

      return true;
    }

    //Compare states
    bool operator == (parser_state_builder_t & state) const {
      for (auto p = items_.head(); nullptr != p; p = p->next()) {
        bool is_exists = false;
        for (auto p1 = state.items().head(); nullptr != p1; p1 = p1->next()) {
          if (p1->value().filter() == p->value().filter()) {
            is_exists = true;
            break;
          }
        }
        if (!is_exists) {
          return false;
        }
      }

      for (auto p = state.items().head(); nullptr != p; p = p->next()) {
        bool is_exists = false;
        for (auto p1 = items_.head(); nullptr != p1; p1 = p1->next()) {
          if (p1->value().filter() == p->value().filter()) {
            is_exists = true;
            break;
          }
        }
        if (!is_exists) {
          return false;
        }
      }

      return true;
    }

    const filter_statemachine::transactions_t & transactions() const {
      return transactions_;
    }

    const linked_list<filter_state_t> & items() const {
      return items_;
    }

    //Contains final state
    bool can_final() const {
      for (auto p = items_.head(); nullptr != p; p = p->next()) {
        if('\0' == *p->value().filter()){
          return true;
        }
      }

      return false;
    }

  private:
    linked_list<filter_state_t> items_;
    filter_statemachine::transactions_t transactions_;

    //Calculate state after applying symbol
    bool next_state(linked_list<parser_state_t> & states, int next_char, int & new_state) const{
      parser_state_builder_t next_state;
      for (auto pstate = items_.head(); nullptr != pstate; pstate = pstate->next()) {
        if (!next_state.apply_char(next_char, pstate->value())) {
          return false;
        }
      }

      if (!next_state.items().empty()) {
        new_state = add_state(
            states,
            static_cast<parser_state_builder_t &&>(next_state));
      }

      return true;
    }
  };

  //Looking to the state and add if it is not exists
  static int add_state(linked_list<parser_state_t> & states, parser_state_builder_t && state){
    auto p = states.head();
    if (nullptr == p) {
      if(!states.add(parser_state_t(static_cast<parser_state_builder_t &&>(state)))) {
        return filter_statemachine::INVALID_STATE;
      }
      return 0;
    }

    for(int result = 0;;++result) {
      if(p->value() == state) {
        return result;
      }
      auto next = p->next();
      if(nullptr == next) {
        if(!p->add(parser_state_t(static_cast<parser_state_builder_t &&>(state)))) {
          return filter_statemachine::INVALID_STATE;
        }

        return result + 1;
      }

      p = next;
    }
  }

  //Start parser state
  static bool init_states(linked_list<parser_state_t> & states, const char * filter) {
    parser_state_builder_t first_state;
    if (!first_state.add_state(filter)) {
      return false;
    }

    if (filter_statemachine::INVALID_STATE == add_state(states, static_cast<parser_state_builder_t &&>(first_state))) {
      return false;
    }

    return true;
  }

  //Generate transitions
  static bool generate_states(linked_list<parser_state_t> & states) {
//    size_t count = 0;
    for (auto p = states.head(); nullptr != p; p = p->next()) {
      if (!p->value().build_transactions(states)) {
        return false;
      }

//      parser_debug("State %d\n", count++);
//      int index = 0;
//      for (auto pstate = p->value().items().head(); nullptr != pstate; pstate = pstate->next()) {
//        parser_debug("%d: %s\n", index++, pstate->value().filter());
//      }
    }

    return true;
  }

  //Allocate result transition table
  static bool pack_result(linked_list<parser_state_t> & states, filter_statemachine & result) {
    auto transactions = reinterpret_cast<filter_statemachine::transactions_t *>(malloc(states.count() * sizeof(filter_statemachine::transactions_t)));
    if (nullptr == transactions) {
      parser_debug("Out of memory");
    }

    size_t index = 0;
    for (auto p = states.head(); nullptr != p; p = p->next()) {
      memcpy(transactions[index++], p->value().transactions(), sizeof(filter_statemachine::transactions_t));
    }

    result.reset(transactions);
    return true;
  }
};


#endif //__LOG_PARSER_FILTER_PARSER_H_
