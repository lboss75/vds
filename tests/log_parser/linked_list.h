//
// Created by vadim on 17.04.18.
//

#ifndef VDS_LINKED_LISK_H
#define VDS_LINKED_LISK_H

template <typename item_type>
class linked_list {
public:

private:
  struct node_type {
    item_type value_;
    linked_list *next_;
  };

  node_type * head_;
};

#endif //VDS_LINKED_LISK_H
