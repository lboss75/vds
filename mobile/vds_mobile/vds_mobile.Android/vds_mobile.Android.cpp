#include "vds_mobile.h"
#include "openssl/ssl.h"

char * AndroidInfo()
{
  SSL_library_init();
	return vds_mobile::getTemplateInfo();
}