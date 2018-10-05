/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_context.h"

vds::http_context::http_context(certificate && peer_certificate)
  : peer_certificate_(std::move(peer_certificate))
{
}
