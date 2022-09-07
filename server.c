#include "core.h"

void parseRequest(http_s *h){
    char *result_message = (char *)malloc(DEFAULT_MESSAGE_LENGTH);
    bool result = false;

    // Parsing the JSON
    FIOBJ req = FIOBJ_INVALID,key = FIOBJ_INVALID;
    FIOBJ res = fiobj_hash_new();
    size_t consumed = fiobj_json2obj(&req, fiobj_obj2cstr(h->body).data, fiobj_obj2cstr(h->body).len);
    // test for errors
    if (!consumed || !req) {
        sprintf(result_message, "\nERROR, couldn't parse data.\n");
        goto end;
    }

#define GetString(x) {key = fiobj_str_new(#x, strlen(#x)); \
if (FIOBJ_TYPE_IS(req, FIOBJ_T_HASH) && fiobj_hash_get(req, key)) {    \
x = fiobj_obj2cstr(fiobj_hash_get(req, key)); \
} else{ \
sprintf(result_message, "\nERROR, can not get "#x"\n"); \
goto end;\
} \
fiobj_free(key);}                                           \

#define Put(k,v) {key = fiobj_str_new(k, strlen(k)); \
    fiobj_hash_set(res,key, v); \
    fiobj_free(key);}                                      \

#define DEFAULT_PATH {
#define BEGIN_PATH(x) if(strcmp(fiobj_obj2cstr(h->method).data,"POST") == 0 && strncmp(fiobj_obj2cstr(h->path).data,x,strlen(x)) == 0){
#define END_PATH goto end;}

    BEGIN_PATH("/api/auth/login")
        fio_str_info_s username,password;
        GetString(username);GetString(password);
        result = fss_login(username.data, password.data, &result_message);
        if(result) Put("session",fiobj_str_new(result_message,strlen(result_message)));
    END_PATH
    BEGIN_PATH("/api/auth/register")
        fio_str_info_s username,password,question,answer;
        GetString(username);GetString(password);GetString(question);GetString(answer);
        result = fss_register(username.data, password.data, question.data,answer.data,&result_message);
    END_PATH
    BEGIN_PATH("/api/auth/user")
        fio_str_info_s username;
        GetString(username);
        result = fss_getQuestion(username.data, &result_message);
        if(result) Put("question", fiobj_str_new(result_message,strlen(result_message)));
    END_PATH
    BEGIN_PATH("/api/auth/resetPW")
        fio_str_info_s username,password,answer;
        GetString(username);GetString(password);GetString(answer);
        result = fss_resetPassword(username.data, password.data,answer.data,&result_message);
    END_PATH
    BEGIN_PATH("/api/fss/listDir")
        fio_str_info_s path,token;
        GetString(path);GetString(token);
        result = fss_listDir(path.data,token.data,&result_message);
        if(result) Put("files", fiobj_str_new(result_message,strlen(result_message)));
    END_PATH
    BEGIN_PATH("/api/fss/listShare")
        fss_listShare(h);
    END_PATH
    BEGIN_PATH("/api/fss/createDir")
        fio_str_info_s path,name,token;
        GetString(path);GetString(name);GetString(token);
        result = fss_createDir(path.data,name.data,token.data,&result_message);
    END_PATH
    BEGIN_PATH("/api/fss/deleteDir")
        fio_str_info_s path,name,token;
        GetString(path);GetString(name);GetString(token);
        result = fss_deleteDir(path.data,name.data,token.data,&result_message);
    END_PATH
    BEGIN_PATH("/api/fss/deleteFile")
        fio_str_info_s path,name,token;
        GetString(path);GetString(name);GetString(token);
        result = fss_deleteFile(path.data,name.data,token.data,&result_message);
    END_PATH
    BEGIN_PATH("/api/fss/share")
        fio_str_info_s path,name,token;
        GetString(path);GetString(name);GetString(token);
        result = fss_share(path.data,name.data,token.data,&result_message);
    END_PATH
    BEGIN_PATH("/api/fss/unShare")
        fio_str_info_s token,sid;
        GetString(token);GetString(sid);
        result = fss_unShare(token.data,sid.data,&result_message);
    END_PATH
    DEFAULT_PATH
        sprintf(result_message, "bad url path or http method");
    END_PATH

    end:
    // Formatting the JSON back to a String object and printing it up
    if(!result){
        h->status = 400;
        http_send_body(h,result_message,strlen(result_message));
        return;
    } else{
        FIOBJ str = fiobj_obj2json(res, PRETTY);
        http_send_body(h,fiobj_obj2cstr(str).data,(size_t)fiobj_obj2cstr(str).len);
        fiobj_free(str);
    }

    // cleanup
    fiobj_free(req);
    fiobj_free(res);
}