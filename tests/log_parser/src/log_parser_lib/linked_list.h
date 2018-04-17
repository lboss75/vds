//
// Created by vadim on 17.04.18.
//

#ifndef VDS_LINKED_LISK_H
#define VDS_LINKED_LISK_H

#include "parser_alloc.h"
#include "parser_debug.h"

template <typename item_type>
class linked_list {
public:
  linked_list()
  : head_{nullptr} {
  }

  linked_list(linked_list && origin)
    : head_{ origin.head_ } {
    origin.head_ = nullptr;
  }

  ~linked_list() {
    clear();
  }

  void clear() {
    auto head = head_;
    while(nullptr != head) {
      auto p = head->next();
      delete head;
      head = p;
    }
  }

  class node_type {
  public:
    node_type(item_type && value)
      : value_(static_cast<item_type &&>(value)),
        next_(nullptr){      
    }

    item_type & value() {
      return value_;
    }

    node_type * next() const {
      return next_;
    };

    bool add(item_type && value) {
      if(nullptr != next_) {
        parser_debug("Invalid call %s\n", __FUNCTION__);
        return false;
      }

      next_ = new(parser_alloc::no_throw) node_type(static_cast<item_type &&>(value));
      if (nullptr == next_) {
        parser_debug("Out of memory\n");
        return false;
      }

      return true;
    }

  private:
    item_type value_;
    node_type * next_;
  };

  bool add(item_type && value) {
    if (nullptr != head_) {
      parser_debug("Invalid call %s\n", __FUNCTION__);
      return false;
    }

    head_ = new(parser_alloc::no_throw) node_type(static_cast<item_type &&>(value));
    if (nullptr == head_) {
      parser_debug("Out of memory\n");
      return false;
    }

    return true;
  }

  node_type * head() const {
    return head_;
  }

  bool empty() const {
    return (nullptr == head_);
  }

  size_t count() const {
    size_t count = 0;
    for (auto p = head_; nullptr != p; p = p->next()) {
      ++count;
    }

    return count;
  }

private:

  node_type * head_;
};

#endif //VDS_LINKED_LISK_H
