
#ifndef VDS_FILTER_PARSER_H
#define VDS_FILTER_PARSER_H

#include <iostream>
#include "parser_vector.h"

class filter_parser {
public:
  bool parse_filter(const char * filter){
    states_.clear();

    parser_state_builder_t state;
    state.add(filter);
    if(0 > add_state(static_cast<parser_state_builder_t &&>(state))){
      return false;
    }

//    for(auto item : states_){
//      item.clean();
//    }

    return true;
  }

  bool is_match(const char * data){
    int state = 0;
    for(;;){
      int new_state = states_[state].transactions[*data];
      if('\0' == *data){
        return states_[state].can_be_finish;
      }

      ++data;
      state = new_state;
    }
  }

private:
  struct filter_state_t {
    const char * filter;

    char allowed_char() const {
      if('*' == *filter) {
        return '?';
      }
      else {
          return *filter;
      }
    }
  };

  struct parser_state_builder_t {
    parser_vector<filter_state_t> items;
    bool can_be_finish;
    int transactions[256];

    parser_state_builder_t()
        : can_be_finish(false){
      memset(transactions, 0, sizeof(transactions));
    }

    parser_state_builder_t(parser_state_builder_t && origin)
    : items(static_cast<parser_vector<filter_state_t>&&>(origin.items)),
      can_be_finish(origin.can_be_finish){

      memset(transactions, 0, sizeof(transactions));

    }

    bool add(const char * filter_state){
      if('\0' == filter_state){
        can_be_finish = true;
      }

      for(auto p : items){
        if(filter_state == p.filter){
          return true;//Already exists
        }
      }

      return items.add(filter_state_t{ filter_state });
    }
    void next(filter_parser * parser) {
      std::cout << "State:" << "\n";
        for(auto p : items) {
          std::cout << p.filter << "\n";
        }

        bool result = false;

      for(auto p : items) {
        auto allowed_char = p.allowed_char();
        if('?' == allowed_char){
          continue;
        }
        if('\0' == allowed_char){
          can_be_finish = true;
          break;
        }

        if(0 == transactions[allowed_char]){
          parser_state_builder_t next_state;
          for(auto state : items){
            if('*' == *state.filter){
              next_state.add(state.filter);
              next_state.add(state.filter + 1);
            }
            else if('?' == *state.filter
                    || allowed_char == *state.filter){
              next_state.add(state.filter + 1);
            }
          }

          transactions[allowed_char] = parser->add_state(
              static_cast<parser_state_builder_t &&>(next_state));
        }
      }

      for(auto p : items) {
        if('?' == p.allowed_char()){
          parser_state_builder_t next_state;
          for(auto state : items){
            if('*' == *state.filter){
              next_state.add(state.filter);
            }

            if('\0' != *state.filter){
              next_state.add(state.filter + 1);
            }
          }

          int next_step_index = parser->add_state(
              static_cast<parser_state_builder_t &&>(next_state));

          for(int ch = 0; ch < 256; ++ch) {
            if(transactions[ch] == 0) {
              transactions[ch] = next_step_index;
            }
          }
        }
      }
    }

    bool operator == (parser_state_builder_t & state) const {
      if(can_be_finish != state.can_be_finish){
        return false;
      }

      for(auto p : items){
        bool is_exists = false;
        for(auto p1 : state.items){
          if(p1.filter == p.filter){
            is_exists = true;
            break;
          }
        }
        if(!is_exists){
          return false;
        }
      }

      for(auto p : state.items){
        bool is_exists = false;
        for(auto p1 : items){
          if(p1.filter == p.filter){
            is_exists = true;
            break;
          }
        }
        if(!is_exists){
          return false;
        }
      }

      return true;
    }

    void clean() {
      this->items.clear();
    }

    void operator =(parser_state_builder_t && origin){
      items = static_cast<parser_vector<filter_state_t>&&>(origin.items);
      can_be_finish = origin.can_be_finish;
    }
  };

  int add_state(parser_state_builder_t && state){
    for(int i = 0; i < states_.count(); ++i){
      if(states_[i] == state){
        return i;
      }
    }

    auto result = states_.count();
    states_.add(static_cast<parser_state_builder_t &&>(state));
    states_[result].next(this);
    return result;
  }

  parser_vector<parser_state_builder_t> states_;
};


#endif //VDS_FILTER_PARSER_H
