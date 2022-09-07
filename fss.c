#include "core.h"

void send_success(http_s *h){
    h->status = 200;
    FIOBJ result_obj = fiobj_hash_new();
    put_obj(result_obj,"result",FIOBJ_T_TRUE);
    FIOBJ result_str = fiobj_obj2json(result_obj,PRETTY);
    http_send_body(h,fiobj_obj2cstr(result_str).data,fiobj_obj2cstr(result_str).len);
    fiobj_free(result_obj);
    fiobj_free(result_str);
}

void send_error(http_s *h, int code, char *message){
    h->status = code;
    FIOBJ result_obj = fiobj_hash_new();
    put_obj(result_obj,"error", fiobj_str_new(message, strlen(message)));
    FIOBJ result_str = fiobj_obj2json(result_obj,PRETTY);
    http_send_body(h,fiobj_obj2cstr(result_str).data,fiobj_obj2cstr(result_str).len);
    fiobj_free(result_obj);
    fiobj_free(result_str);
}

void
fss_register(http_s *h) {
    FIOBJ username = get_obj(h->params,"username");

    FIOBJ users = db_read_users_by_username(username);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) != 0) {
        send_error(h,400,"username already exist");
        goto free;
    }
    if (db_create_user(h->params) == FIOBJ_T_FALSE) {
        h->status = 500;
        send_error(h,500,"database error");
        goto free;
    }
    send_success(h);

    free:
    fiobj_free(users);
}

void
fss_login(http_s *h) {
    FIOBJ username = get_obj(h->params, "username");
    FIOBJ password = get_obj(h->params, "password");
    char *token = randomString(TOKEN_LENGTH);

    FIOBJ users = db_read_users_by_username(username);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);

    if (strcmp(fiobj_obj2cstr(password).data, fiobj_obj2cstr(get_obj(user,"password")).data) != 0) {
        send_error(h,400,"password error");h->status = 400;
        goto free;
    }
    put_obj(h->params,"token", fiobj_str_new(token, strlen(token)));
    if (db_create_login(h->params) == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }

    h->status = 200;
    FIOBJ result_obj = fiobj_hash_new();
    put_obj(result_obj,"result",FIOBJ_T_TRUE);
    put_obj(result_obj,"token", fiobj_str_new(token, strlen(token)));
    FIOBJ result_str = fiobj_obj2json(result_obj,PRETTY);
    http_send_body(h,fiobj_obj2cstr(result_str).data,fiobj_obj2cstr(result_str).len);
    fiobj_free(result_obj);
    fiobj_free(result_str);

    free:
    free(token);
    fiobj_free(users);
}

void
fss_reset_password(http_s *h) {
    FIOBJ username = get_obj(h->params,"username");
    FIOBJ password = get_obj(h->params,"password");
    FIOBJ answer = get_obj(h->params, "answer");

    FIOBJ users = db_read_users_by_username(username);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);

    if (strcmp(fiobj_obj2cstr(answer).data, fiobj_obj2cstr(get_obj(user,"answer")).data) != 0) {
        send_error(h,400,"answer error");h->status = 400;
        goto free;
    }
    put_obj(user,"password",password);
    if (db_update_user(user) == FIOBJ_T_FALSE) {
        h->status = 500;
        send_error(h,500,"database error");
        goto free;
    }
    send_success(h);

    free:
    fiobj_free(users);
}

void
fss_get_user_question(http_s *h) {
    FIOBJ username = get_obj(h->params, "username");

    FIOBJ users = db_read_users_by_username(username);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");h->status = 400;
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);

    h->status = 200;
    FIOBJ result_obj = fiobj_hash_new();
    put_obj(result_obj,"result",FIOBJ_T_TRUE);
    put_obj(result_obj,"question", get_obj(user,"question"));
    FIOBJ result_str = fiobj_obj2json(result_obj,PRETTY);
    http_send_body(h,fiobj_obj2cstr(result_str).data,fiobj_obj2cstr(result_str).len);
    fiobj_free(result_obj);
    fiobj_free(result_str);
    free:
    fiobj_free(users);
}

void
fss_create_dir(http_s *h) {
    FIOBJ path = get_obj(h->params,"path");
    FIOBJ name = get_obj(h->params,"name");
    FIOBJ token = get_obj(h->params,"token");

    FIOBJ users = db_read_logins_by_token(token);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);
    FIOBJ dirtrees = db_read_dir_tree_by_username(get_obj(user,"username"));
    if (dirtrees == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(dirtrees) == 0) {
        send_error(h,500,"database error");
        goto free;
    }
    FIOBJ dirtree = fiobj_ary_index(dirtrees,0);
    FIOBJ tree = get_obj(dirtree,"tree");
    FIOBJ root = FIOBJ_INVALID;
    fiobj_json2obj(&root,fiobj_obj2cstr(tree).data,fiobj_obj2cstr(tree).len);
    if (_createDir(root, fiobj_obj2cstr(path).data,fiobj_obj2cstr(name).data)== FIOBJ_T_FALSE) {
        send_error(h,500,"create dir fail");
        goto free;
    }
    put_obj(dirtree,"tree",fiobj_obj2json(root,PRETTY));
    if(db_update_dir_tree(dirtree) == FIOBJ_T_FALSE){
        send_error(h,500,"database error");
        goto free;
    }

    send_success(h);

    free:
    fiobj_free(root);
    fiobj_free(dirtrees);
    fiobj_free(users);
}

void
fss_list_dir(http_s *h) {
    FIOBJ path = get_obj(h->params,"path");
    FIOBJ token = get_obj(h->params,"token");

    FIOBJ users = db_read_logins_by_token(token);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);
    FIOBJ dirtrees = db_read_dir_tree_by_username(get_obj(user,"username"));
    if (dirtrees == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(dirtrees) == 0) {
        send_error(h,500,"database error");
        goto free;
    }
    FIOBJ dirtree = fiobj_ary_index(dirtrees,0);
    FIOBJ tree = get_obj(dirtree,"tree");
    FIOBJ root = FIOBJ_INVALID,files = FIOBJ_INVALID;
    fiobj_json2obj(&root,fiobj_obj2cstr(tree).data,fiobj_obj2cstr(tree).len);
    if ((files =_listDir(root, fiobj_obj2cstr(path).data) )== FIOBJ_T_FALSE) {
        send_error(h,500,"dir not exist");
        goto free;
    }

    h->status = 200;
    FIOBJ result_obj = fiobj_hash_new();
    put_obj(result_obj,"result",FIOBJ_T_TRUE);
    put_obj(result_obj,"files", files);
    FIOBJ result_str = fiobj_obj2json(result_obj,PRETTY);
    http_send_body(h,fiobj_obj2cstr(result_str).data,fiobj_obj2cstr(result_str).len);
    fiobj_free(result_obj);
    fiobj_free(result_str);

    free:
    fiobj_free(root);
    fiobj_free(files);
    fiobj_free(dirtrees);
    fiobj_free(users);
}

void
fss_list_share(http_s *h) {
    FIOBJ token = get_obj(h->params,"token");

    FIOBJ users = db_read_logins_by_token(token);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);
    FIOBJ shares = db_read_share_by_username(get_obj(user,"username"));
    if (shares == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }

    h->status = 200;
    FIOBJ result_obj = fiobj_hash_new();
    put_obj(result_obj,"result",FIOBJ_T_TRUE);
    put_obj(result_obj,"shares", shares);
    FIOBJ result_str = fiobj_obj2json(result_obj,PRETTY);
    http_send_body(h,fiobj_obj2cstr(result_str).data,fiobj_obj2cstr(result_str).len);
    fiobj_free(result_obj);
    fiobj_free(result_str);

    free:
    fiobj_free(shares);
    fiobj_free(users);
}

void
fss_delete_file(http_s *h) {
    FIOBJ path = get_obj(h->params,"path");
    FIOBJ name = get_obj(h->params,"name");
    FIOBJ token = get_obj(h->params,"token");

    FIOBJ users = db_read_logins_by_token(token);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);

    FIOBJ dirtrees = db_read_dir_tree_by_username(get_obj(user,"username"));
    if (dirtrees == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(dirtrees) == 0) {
        send_error(h,500,"database error");
        goto free;
    }
    FIOBJ dirtree = fiobj_ary_index(dirtrees,0);
    FIOBJ tree = get_obj(dirtree,"tree");
    FIOBJ root = FIOBJ_INVALID;
    fiobj_json2obj(&root,fiobj_obj2cstr(tree).data,fiobj_obj2cstr(tree).len);
    if (_delete(root, fiobj_obj2cstr(path).data,fiobj_obj2cstr(name).data)== FIOBJ_T_FALSE) {
        send_error(h,500,"create dir fail");
        goto free;
    }
    put_obj(dirtree,"tree",fiobj_obj2json(root,PRETTY));
    if(db_update_dir_tree(dirtree) == FIOBJ_T_FALSE){
        send_error(h,500,"database error");
        goto free;
    }

    send_success(h);

    free:
    fiobj_free(root);
    fiobj_free(dirtrees);
    fiobj_free(users);
}

void
fss_create_share(http_s *h) {
    FIOBJ path = get_obj(h->params,"path");
    FIOBJ token = get_obj(h->params,"token");
    char *sid = randomString(TOKEN_LENGTH);

    FIOBJ users = db_read_logins_by_token(token);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);

    FIOBJ dirtrees = db_read_dir_tree_by_username(get_obj(user,"username"));
    if (dirtrees == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(dirtrees) == 0) {
        send_error(h,500,"database error");
        goto free;
    }
    FIOBJ dirtree = fiobj_ary_index(dirtrees,0);

    FIOBJ tree = get_obj(dirtree,"tree");
    FIOBJ root = FIOBJ_INVALID,children = FIOBJ_INVALID;
    fiobj_json2obj(&root,fiobj_obj2cstr(tree).data,fiobj_obj2cstr(tree).len);
    if ((children = _getChild(root, fiobj_obj2cstr(path).data))== FIOBJ_T_FALSE) {
        send_error(h,500,"create share fail");
        goto free;
    }
    put_obj(h->params,"username", get_obj(user,"username"));
    put_obj(h->params,"sid", fiobj_str_new(sid, strlen(sid)));
    put_obj(h->params,"tree", fiobj_obj2json(children,PRETTY));
    if (db_create_share(h->params) == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }

    send_success(h);

    free:
    free(sid);
    fiobj_free(root);
    fiobj_free(dirtrees);
    fiobj_free(users);
}

void
fss_delete_share(http_s *h) {
    FIOBJ sid = get_obj(h->params,"sid");
    FIOBJ token = get_obj(h->params,"token");

    FIOBJ users = db_read_logins_by_token(token);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);

    FIOBJ shares = db_read_share_by_sid(sid);
    if (shares == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(shares) == 0) {
        send_error(h,400,"share not exist");
        goto free;
    }
    FIOBJ share = fiobj_ary_index(shares,0);

    if (strcmp(fiobj_obj2cstr(get_obj(user,"username")).data, fiobj_obj2cstr(get_obj(share,"username")).data) != 0) {
        send_error(h,400,"permission deny");
        goto free;
    }
    if (db_delete_share_by_sid(sid) == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    send_success(h);
    free:
    fiobj_free(shares);
    fiobj_free(users);
}

void
fss_upload_file(http_s *h) {
    FIOBJ path = get_obj(h->params,"path");
    FIOBJ name = get_obj(h->params,"name");
    FIOBJ data = get_obj(h->params,"data");
    FIOBJ token = get_obj(h->params,"token");

    FIOBJ users = db_read_logins_by_token(token);
    if (users == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(users) == 0) {
        send_error(h,400,"username not exist");
        goto free;
    }
    FIOBJ user = fiobj_ary_index(users,0);
    FIOBJ dirtrees = db_read_dir_tree_by_username(get_obj(user,"username"));
    if (dirtrees == FIOBJ_T_FALSE) {
        send_error(h,500,"database error");
        goto free;
    }
    if (fiobj_ary_count(dirtrees) == 0) {
        send_error(h,500,"database error");
        goto free;
    }
    FIOBJ dirtree = fiobj_ary_index(dirtrees,0);
    FIOBJ tree = get_obj(dirtree,"tree");
    FIOBJ root = FIOBJ_INVALID;
    fiobj_json2obj(&root,fiobj_obj2cstr(tree).data,fiobj_obj2cstr(tree).len);
    if (_createFile(root, fiobj_obj2cstr(path).data,fiobj_obj2cstr(name).data,fiobj_obj2cstr(data).data)== FIOBJ_T_FALSE) {
        send_error(h,500,"create dir fail");
        goto free;
    }
    put_obj(dirtree,"tree",fiobj_obj2json(root,PRETTY));
    if(db_update_dir_tree(dirtree) == FIOBJ_T_FALSE){
        send_error(h,500,"database error");
        goto free;
    }

    send_success(h);

    free:
    fiobj_free(root);
    fiobj_free(dirtrees);
    fiobj_free(users);
}