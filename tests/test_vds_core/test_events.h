#ifndef __TEST_VDS_CORE_TEST_EVENTS_H_
#define __TEST_VDS_CORE_TEST_EVENTS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

class source_class
{
public:

  vds::event_source<> source;
};

class target_class
{
public:
  target_class();

  vds::event_handler<> target;
};



#endif // !__TEST_VDS_CORE_TEST_ASYNC_H_

