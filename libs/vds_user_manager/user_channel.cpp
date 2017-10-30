/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "user_channel.h"
#include "private/user_channel_p.h"


vds::user_channel::user_channel(
    const vds::guid &id,
    const vds::certificate &cert)
: impl_(new _user_channel(id, cert))
{
}

/////////////////////////////////////////////////
vds::_user_channel::_user_channel(
    const vds::guid &id,
    const vds::certificate &cert)
: id_(id), cert_(cert)
{
}
