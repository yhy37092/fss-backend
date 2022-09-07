#include "core.h"

extern ConnectionPool_T pool;

FIOBJ
db_create_user(FIOBJ user) {
    FIOBJ username = get_obj(user,"username");
    FIOBJ password = get_obj(user,"password");
    FIOBJ question = get_obj(user,"question");
    FIOBJ answer = get_obj(user,"answer");

    char sql_create_user[] = "INSERT INTO fss.user (username, password, question, answer) VALUES ('%s', '%s', '%s', '%s');";
    char sql_create_dirtree[] = "INSERT INTO fss.dirtree (username, tree) VALUES ('%s','{\"name\": \"/\", \"type\": \"dir\", \"children\": []}');";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                Connection_beginTransaction(con);
                Connection_execute(con,
                                   sql_create_user,
                                   fiobj_obj2cstr(username).data,
                                   fiobj_obj2cstr(password).data,
                                   fiobj_obj2cstr(question).data,
                                   fiobj_obj2cstr(answer).data);
                Connection_execute(con,
                                   sql_create_dirtree,
                                   fiobj_obj2cstr(username).data);
                Connection_commit(con);
            }
        CATCH(SQLException)
            {
                Connection_rollback(con);
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
    return FIOBJ_T_TRUE;
}

FIOBJ
db_read_users_by_username(FIOBJ username) {
    FIOBJ result = fiobj_ary_new();

    char sql_read_users_by_username[] = "SELECT password,question,answer FROM fss.user WHERE username LIKE '%s';";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                ResultSet_T sql_result = Connection_executeQuery(con,
                                                             sql_read_users_by_username,
                                                             fiobj_obj2cstr(username).data);
                while (ResultSet_next(sql_result)) {

                    FIOBJ obj_result = fiobj_hash_new();

                    put_obj(obj_result,"username",
                            username);
                    put_obj(obj_result,"password",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 1),
                                    strlen(ResultSet_getString(sql_result, 1))));
                    put_obj(obj_result,"question",
                            fiobj_str_new(
                            ResultSet_getString(sql_result, 2),
                            strlen(ResultSet_getString(sql_result, 2))));
                    put_obj(obj_result,"answer",
                            fiobj_str_new(
                            ResultSet_getString(sql_result, 3),
                            strlen(ResultSet_getString(sql_result, 3))));

                    fiobj_ary_push(result,obj_result);
                }
            }
        CATCH(SQLException)
            {
                fiobj_free(result);
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
    return result;
}

FIOBJ
db_update_user(FIOBJ user) {
    FIOBJ username = get_obj(user,"username");
    FIOBJ password = get_obj(user,"password");
    FIOBJ question = get_obj(user,"question");
    FIOBJ answer = get_obj(user,"answer");
    char sql_update_user[] = "UPDATE fss.user SET password = '%s', question = '%s', answer = '%s' WHERE username LIKE '%s'";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                Connection_execute(con,
                                   sql_update_user,
                                   fiobj_obj2cstr(password).data,
                                   fiobj_obj2cstr(question).data,
                                   fiobj_obj2cstr(answer).data,
                                   fiobj_obj2cstr(username).data);
            }
        CATCH(SQLException)
            {
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
}

FIOBJ
db_create_login(FIOBJ login) {
    FIOBJ username = get_obj(login,"username");
    FIOBJ token = get_obj(login,"token");

    char sql_create_login[] = "INSERT INTO fss.login (token,username) VALUES ('%s', '%s');";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                Connection_execute(con,
                                   sql_create_login,
                                   fiobj_obj2cstr(token).data,
                                   fiobj_obj2cstr(username).data);
            }
        CATCH(SQLException)
            {
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
    return FIOBJ_T_TRUE;
}

FIOBJ
db_read_logins_by_token(FIOBJ token) {
    FIOBJ result = fiobj_ary_new();

    char sql_read_logins_by_token[] = "SELECT username FROM fss.login WHERE token LIKE '%s'";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                ResultSet_T sql_result = Connection_executeQuery(con,
                                                                 sql_read_logins_by_token,
                                                                 fiobj_obj2cstr(token).data);
                while (ResultSet_next(sql_result)) {

                    FIOBJ obj_result = fiobj_hash_new();

                    put_obj(obj_result,"username",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 1),
                                    strlen(ResultSet_getString(sql_result, 1))));
                    put_obj(obj_result,"token",
                            token);

                    fiobj_ary_push(result,obj_result);
                }
            }
        CATCH(SQLException)
            {
                fiobj_free(result);
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
    return result;
}

FIOBJ
db_read_dir_tree_by_username(FIOBJ username){
    FIOBJ result = fiobj_ary_new();

    char sql_read_dir_tree_by_username[] = "SELECT tree FROM fss.dirtree WHERE username LIKE '%s'";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                ResultSet_T sql_result = Connection_executeQuery(con,
                                                                 sql_read_dir_tree_by_username,
                                                                 fiobj_obj2cstr(username).data);
                while (ResultSet_next(sql_result)) {

                    FIOBJ obj_result = fiobj_hash_new();

                    put_obj(obj_result,"tree",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 1),
                                    strlen(ResultSet_getString(sql_result, 1))));
                    put_obj(obj_result,"username",
                            username);

                    fiobj_ary_push(result,obj_result);
                }
            }
        CATCH(SQLException)
            {
                fiobj_free(result);
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
    return result;
}


FIOBJ
db_update_dir_tree(FIOBJ dirtree){
    FIOBJ username = get_obj(dirtree,"username");
    FIOBJ tree = get_obj(dirtree,"tree");
    char sql_update_dir_tree[] = "UPDATE fss.dirtree SET tree = '%s' WHERE username LIKE '%s'";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                Connection_execute(con,
                                   sql_update_dir_tree,
                                   fiobj_obj2cstr(tree).data,
                                   fiobj_obj2cstr(username).data);
            }
        CATCH(SQLException)
            {
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
}

FIOBJ
db_create_share(FIOBJ share){
    FIOBJ sid = get_obj(share,"sid");
    FIOBJ path = get_obj(share,"path");
    FIOBJ username = get_obj(share,"username");
    FIOBJ tree = get_obj(share,"tree");

    char sql_create_share[] = "INSERT INTO fss.share (sid,username,path,tree) VALUES ('%s', '%s', '%s', '%s');";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                Connection_execute(con,
                                   sql_create_share,
                                   fiobj_obj2cstr(sid).data,
                                   fiobj_obj2cstr(username).data,
                                   fiobj_obj2cstr(path).data,
                                   fiobj_obj2cstr(tree).data);
            }
        CATCH(SQLException)
            {
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
    return FIOBJ_T_TRUE;
}

FIOBJ
db_read_share_by_sid(FIOBJ sid){
    FIOBJ result = fiobj_ary_new();

    char sql_read_share_by_sid[] = "SELECT username,tree,status,path FROM fss.share where sid = '%s'";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                ResultSet_T sql_result = Connection_executeQuery(con,
                                                                 sql_read_share_by_sid,
                                                                 fiobj_obj2cstr(sid).data);
                while (ResultSet_next(sql_result)) {

                    FIOBJ obj_result = fiobj_hash_new();

                    put_obj(obj_result,"username",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 1),
                                    strlen(ResultSet_getString(sql_result, 1))));
                    put_obj(obj_result,"tree",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 2),
                                    strlen(ResultSet_getString(sql_result, 2))));
                    put_obj(obj_result,"status",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 3),
                                    strlen(ResultSet_getString(sql_result, 3))));
                    put_obj(obj_result,"path",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 4),
                                    strlen(ResultSet_getString(sql_result, 4))));
                    put_obj(obj_result,"sid",
                            sid);

                    fiobj_ary_push(result,obj_result);
                }
            }
        CATCH(SQLException)
            {
                fiobj_free(result);
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
    return result;
}

FIOBJ
db_read_share_by_username(FIOBJ username){
    FIOBJ result = fiobj_ary_new();

    char sql_read_share_by_username[] = "SELECT sid,tree,status,path FROM fss.share where username = '%s'";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                ResultSet_T sql_result = Connection_executeQuery(con,
                                                                 sql_read_share_by_username,
                                                                 fiobj_obj2cstr(username).data);
                while (ResultSet_next(sql_result)) {

                    FIOBJ obj_result = fiobj_hash_new();

                    put_obj(obj_result,"sid",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 1),
                                    strlen(ResultSet_getString(sql_result, 1))));
                    put_obj(obj_result,"tree",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 2),
                                    strlen(ResultSet_getString(sql_result, 2))));
                    put_obj(obj_result,"status",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 3),
                                    strlen(ResultSet_getString(sql_result, 3))));
                    put_obj(obj_result,"path",
                            fiobj_str_new(
                                    ResultSet_getString(sql_result, 4),
                                    strlen(ResultSet_getString(sql_result, 4))));
                    put_obj(obj_result,"username",
                            username);

                    fiobj_ary_push(result,obj_result);
                }
            }
        CATCH(SQLException)
            {
                fiobj_free(result);
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
    return result;
}

FIOBJ
db_delete_share_by_sid(FIOBJ sid){

    char sql_delete_share_by_sid[] = "DELETE FROM fss.share WHERE sid LIKE '%s';";

    Connection_T con;
    TRY
            {
                con = ConnectionPool_getConnection(pool);
                Connection_execute(con,
                                   sql_delete_share_by_sid,
                                   fiobj_obj2cstr(sid).data);
            }
        CATCH(SQLException)
            {
                return FIOBJ_T_FALSE;
            }
        FINALLY
            {
                Connection_close(con);
            }
    END_TRY;
}