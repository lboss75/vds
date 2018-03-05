#ifndef __vds_embedded_vds_embedded_H_
#define __VDS_BACKGROUND_STDAFX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifdef _WIN32
#define SHARED_EXPORT __declspec(dllexport)
#else
#define SHARED_EXPORT
#endif

SHARED_EXPORT void * vds_create_configuration();
SHARED_EXPORT void vds_delete_configuration(void *);


#endif // __VDS_BACKGROUND_STDAFX_H_
