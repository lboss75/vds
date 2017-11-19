/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "user_channel.h"
#include "private/user_channel_p.h"


vds::user_channel::user_channel(
    const vds::guid &id,
    const vds::certificate & read_cert,
    const vds::certificate & write_cert)
: impl_(new _user_channel(id, read_cert, write_cert))
{
}

/////////////////////////////////////////////////
vds::_user_channel::_user_channel(
    const vds::guid &id,
    const vds::certificate & read_cert,
    const vds::certificate & write_cert)
: id_(id), read_cert_(read_cert), write_cert_(write_cert)
{
}
