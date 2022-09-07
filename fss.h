#ifndef FSS_H_
#define FSS_H_

#include "core.h"
void fss_register(http_s *h);

void fss_login(http_s *h);

void fss_get_user_question(http_s *h);

void fss_reset_password(http_s *h);

void fss_create_dir(http_s *h);

void fss_delete_file(http_s *h);

void fss_list_dir(http_s *h);

void fss_create_share(http_s *h);

void fss_list_share(http_s *h);

void fss_delete_share(http_s *h);

void fss_upload_file(http_s *h);

#endif