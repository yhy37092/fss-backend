#ifndef DB_H_
#define DB_H_

#include "core.h"

FIOBJ db_create_user(FIOBJ user);

FIOBJ db_update_user(FIOBJ user);

FIOBJ db_read_users_by_username(FIOBJ username);

FIOBJ db_create_login(FIOBJ login);

FIOBJ db_read_logins_by_token(FIOBJ token);

FIOBJ db_read_dir_tree_by_username(FIOBJ username);

FIOBJ db_update_dir_tree(FIOBJ dirtree);

FIOBJ db_create_share(FIOBJ share);

FIOBJ db_read_share_by_sid(FIOBJ sid);

FIOBJ db_read_share_by_username(FIOBJ username);

FIOBJ db_delete_share_by_sid(FIOBJ share);

#endif
