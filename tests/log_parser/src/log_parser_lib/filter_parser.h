
#ifndef VDS_FILTER_PARSER_H
#define VDS_FILTER_PARSER_H

#include <iostream>
#include <cstring>
#include "linked_list.h"
#include "filter_statemachine.h"

class filter_parser {
public:
  
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
  class filter_state_t {
  public:
    static const int ANY_CHAR = 256;
    static const int ANY_CHAR_MULTIPLE = 257;


    filter_state_t(const char * filter)
    : filter_(filter){
      
    }

    const char * filter() const {
      return filter_;
    }

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

  class parser_state_builder_t {
  public:
    parser_state_builder_t() {
    }

    parser_state_builder_t(parser_state_builder_t && origin) = delete;

    bool add_state(const char * filter_state) {
      trim_left(filter_state);

      if(!add(filter_state)) {
        return false;
      }

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
    static void trim_left(const char * & filter) {
      while('*' == filter[0] && '*' == filter[1]) {
        ++filter;
      }
    }
  private:
    linked_list<filter_state_t> items_;

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

  class parser_state_t {
  public:
    parser_state_t () {
    }

    parser_state_t(parser_state_builder_t && origin)
    : items_((linked_list<filter_state_t> &&)origin.items()){
    }

    bool build_transactions(linked_list<parser_state_t> & states) {
      for (int i = 0; i < sizeof(transactions_) / sizeof(transactions_[0]); ++i) {
        transactions_[i] = filter_statemachine::INVALID_STATE;
      }

      for (auto p = items_.head(); nullptr != p; p = p->next()) {
        auto next_char = p->value().next_char();
        if (filter_state_t::ANY_CHAR == next_char || filter_state_t::ANY_CHAR_MULTIPLE == next_char) {
          continue;
        }
        if (0 == next_char) {
          transactions_[next_char] = filter_statemachine::FINAL_STATE;
          break;
        }

        if (filter_statemachine::INVALID_STATE == transactions_[next_char]) {
          parser_state_builder_t next_state;
          for (auto pstate = items_.head(); nullptr != pstate; pstate = pstate->next()) {
            if ('*' == *pstate->value().filter()) {
              next_state.add_state(pstate->value().filter());
              next_state.add_state(pstate->value().filter() + 1);
            }
            else if ('?' == *pstate->value().filter()
              || next_char == *pstate->value().filter()) {
              next_state.add_state(pstate->value().filter() + 1);
            }
          }

          transactions_[next_char] = add_state(
            states,
            static_cast<parser_state_builder_t &&>(next_state));
        }
      }

      for (auto p = items_.head(); nullptr != p; p = p->next()) {
        if (filter_state_t::ANY_CHAR == p->value().next_char() || filter_state_t::ANY_CHAR_MULTIPLE == p->value().next_char()) {
          parser_state_builder_t next_state;
          for (auto pstate = items_.head(); nullptr != pstate; pstate = pstate->next()) {
            if ('*' == *pstate->value().filter()) {
              next_state.add_state(pstate->value().filter());
            }

            if ('\0' != *pstate->value().filter()) {
              next_state.add_state(pstate->value().filter() + 1);
            }
          }

          if (!next_state.items().empty()) {
            int next_step_index = add_state(
              states,
              static_cast<parser_state_builder_t &&>(next_state));

            for (int ch = 1; ch < 256; ++ch) {
              if (filter_statemachine::INVALID_STATE == transactions_[ch]) {
                transactions_[ch] = next_step_index;
              }
            }
          }
        }
      }
      return true;
    }

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

    void clean() {
      this->items_.clear();
    }

    const filter_statemachine::transactions_t & transactions() const {
      return transactions_;
    }

    const linked_list<filter_state_t> & items() const {
      return items_;
    }

  private:
    linked_list<filter_state_t> items_;
    filter_statemachine::transactions_t transactions_;
  };

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

  static bool generate_states(linked_list<parser_state_t> & states) {
    for (auto p = states.head(); nullptr != p; p = p->next()) {
      if (!p->value().build_transactions(states)) {
        return false;
      }

#ifdef _DEBUG_PARSER_STATES
      parser_debug("State %d\n", count);
      int index = 0;
      for (auto pstate = p->value().items().head(); nullptr != pstate; pstate = pstate->next()) {
        parser_debug("%d: %s\n", index++, pstate->value().filter());
      }
#endif
    }

    return true;
  }

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


#endif //VDS_FILTER_PARSER_H
