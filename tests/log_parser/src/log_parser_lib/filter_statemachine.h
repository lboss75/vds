#pragma once
#include "parser_alloc.h"
#include "parser_debug.h"

class filter_statemachine
{
public:
  typedef  int transactions_t[256];

  static constexpr int START_STATE = 0;
  static constexpr int INVALID_STATE = -1;
  static constexpr int FINAL_STATE = -2;

  filter_statemachine()
    : states_(nullptr)
  {
  }

  filter_statemachine(transactions_t * states)
    : states_(states)
  {
  }

  filter_statemachine(filter_statemachine && origin)
    : states_(origin.states_) {
    origin.states_ = nullptr;
  }

  ~filter_statemachine()
  {
    if(nullptr != states_) {
      free(states_);
    }
  }

  bool is_match(const char * data) const {
    auto state = START_STATE;
    for (;;) {
      state = next_state(state, *data);
      switch (state) {
        case INVALID_STATE: {
          return false;
        }
        case FINAL_STATE: {
          return  ('\0' == *data);
        }
        default: {
          ++data;
          break;
        }
      }
    }
  }

  int next_state(int current_state, char next_char) const {
    return states_[current_state][(unsigned char)next_char];
  }

  void reset(transactions_t * states){
    if (nullptr != states_) {
      free(states_);
    }
    states_ = states;
  }

private:
  transactions_t * states_;
};

