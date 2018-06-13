#ifndef __VDS_EMBEDDED_VDS_API_H_
#define __VDS_EMBEDDED_VDS_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef _WIN32

#define API __attribute__ ((visibility("default")))

#else

#define API __declspec(dllexport)

#endif


#ifdef __cplusplus
extern "C" {
#endif

void * API vds_init();
void API vds_done(void * vds);
const char * API vds_last_error(void * vds);

void API vds_set_root_folder(void * vds, const char * root_folder);

void API vds_server_root (void * vds, const char * login, const char * password);

#ifdef __cplusplus
}
#endif

#endif //__VDS_EMBEDDED_VDS_API_H_
