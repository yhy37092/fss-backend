#include "core.h"

FIOBJ get_obj(FIOBJ hash,const char *key){
    FIOBJ _key = fiobj_str_new(key, strlen(key));
    FIOBJ _result = fiobj_hash_get(hash,_key);
    fiobj_free(_key);
    return _result;
}

void put_obj(FIOBJ hash,const char *key,FIOBJ value){
    FIOBJ _key = fiobj_str_new(key, strlen(key));
    fiobj_hash_set(hash,_key,value);
    fiobj_free(_key);
}

char *randomString(size_t length) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *randomString = NULL;

    if (length) {
        randomString = malloc(sizeof(char) * (length +1));

        if (randomString) {
            for (int n = 0;n < length;n++) {
                int key = rand() % (int)(sizeof(charset) -1);
                randomString[n] = charset[key];
            }

            randomString[length] = '\0';
        }
    }

    return randomString;
}

//create a dir OBJ
FIOBJ
__makeDir(char* name) {
    FIOBJ root = fiobj_hash_new();
    put_obj(root,"name",fiobj_str_new(name,strlen(name)));
    put_obj(root,"type",fiobj_str_new("dir",3));
    put_obj(root,"children",fiobj_ary_new());
    return root;
}

// create a file OBJ
FIOBJ
__touch(char* name,char* hash,size_t length) {
    FIOBJ root = fiobj_hash_new(),children = fiobj_ary_new();
    put_obj(root,"name",fiobj_str_new(name,strlen(name)));
    put_obj(root,"hash",fiobj_str_new(hash,strlen(hash)));
    put_obj(root,"type",fiobj_str_new("file",4));
    put_obj(root,"length", fiobj_num_new(length));
    return root;
}

//remove OBJ in current OBJ
FIOBJ
__remove(FIOBJ root, char* name) {
    FIOBJ children = FIOBJ_INVALID;
    children = get_obj(root,"children");
    if(children == FIOBJ_INVALID) return FIOBJ_T_FALSE;

    for (size_t j = 0; j < fiobj_ary_count(children); j++) {
        FIOBJ item = fiobj_ary_index(children,(int64_t)j);
        FIOBJ item_name = get_obj(item,"name");
        if (strcmp(fiobj_obj2cstr(item_name).data,name) == 0) {
            fiobj_ary_remove(children,(int64_t)j);
            return FIOBJ_T_TRUE;
        }
    }
    return FIOBJ_T_FALSE;
}
//find OBJ current OBG
FIOBJ
__find(FIOBJ root, char* name) {
    FIOBJ children = FIOBJ_INVALID;
    children = get_obj(root,"children");
    if(children == FIOBJ_INVALID) return FIOBJ_T_FALSE;

    for (size_t j = 0; j < fiobj_ary_count(children); j++) {
        FIOBJ item = fiobj_ary_index(children,(int64_t)j);
        FIOBJ item_name = get_obj(item,"name");
        if (strcmp(fiobj_obj2cstr(item_name).data,name) == 0) {
            return item;
        }
    }
    return FIOBJ_T_FALSE;
}
//cd a OBJ by path
FIOBJ
__cd(FIOBJ root, char* path) {
    FIOBJ current = root;
    char *_path = strdup(path);

    char *dirname = strtok(path, "/\\");
    while (dirname != NULL) {
        current = __find(current, dirname);
        if(current == FIOBJ_T_FALSE) {free(_path); return FIOBJ_T_FALSE; }
        dirname = strtok(NULL, "/\\");
    }
    free(_path);
    return current;
}
FIOBJ
__list(FIOBJ root,char* path) {
    FIOBJ result = FIOBJ_INVALID,children = FIOBJ_INVALID;
    if((root = __cd(root,path)) == FIOBJ_T_FALSE) return FIOBJ_T_FALSE;
    children = get_obj(root,"children");
    if(children == FIOBJ_INVALID) return FIOBJ_T_FALSE;
    result = fiobj_ary_new();

    for (size_t j = 0; j < fiobj_ary_count(children); j++) {
        FIOBJ item = fiobj_ary_index(children,(int64_t)j);
        FIOBJ tmp_type = get_obj(item,"type");
        FIOBJ tmp_name = get_obj(item,"name");
        if(strcmp(fiobj_obj2cstr(tmp_type).data, "dir") == 0){
            fiobj_ary_push(result, __makeDir(fiobj_obj2cstr(tmp_name).data));
        } else{
            FIOBJ tmp_hash = get_obj(item,"hash");
            FIOBJ tmp_length = get_obj(item,"length");
            fiobj_ary_push(result, __touch(
                    fiobj_obj2cstr(tmp_name).data,
                    fiobj_obj2cstr(tmp_hash).data,
                    fiobj_obj2num(tmp_length)
            ));
        }

    }
    return result;
}

FIOBJ
_create(FIOBJ root, char *path, char *name, char *hash,size_t length,bool dir) {

    FIOBJ current = FIOBJ_INVALID, children = FIOBJ_INVALID;

    if((current = __cd(root,path)) == FIOBJ_T_FALSE) return FIOBJ_T_FALSE;
    if(__find(current,name) != FIOBJ_T_FALSE) return FIOBJ_T_FALSE;
    children = get_obj(current,"children");
    fiobj_ary_push(children,dir?__makeDir(name):__touch(name,hash,length));

    return root;
}
FIOBJ
_createDir(FIOBJ root, char *path, char *name) {
    return _create(root,path,name,NULL,0,true);
}

FIOBJ
_createFile(FIOBJ root, char *path, char *name, char *data){
    int base64_len = (int)strlen(data);
    unsigned char *raw = (unsigned char *)malloc(base64_len/4*3+3);
    int raw_len = EVP_DecodeBlock(raw,data,base64_len);
    fio_sha1_s sha1 = fio_sha1_init();
    char *sha1_result = fio_sha1(&sha1,raw,raw_len);
    char hash[40] = {0};
    for(int i = 0; i < 20; i++) sprintf(&hash[i*2],"%02X", (unsigned char)sha1_result[i]);
    int fd = open(hash, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR, S_IWUSR);
    write(fd,raw,raw_len);
    free(raw);
    return _create(root,path,name,hash,raw_len,false);
}

FIOBJ
_delete(FIOBJ root, char *path,char *name) {
    FIOBJ current = FIOBJ_INVALID;
    if((current = __cd(root,path)) == FIOBJ_T_FALSE) return FIOBJ_T_FALSE;
    if(__remove(current,name) == FIOBJ_T_FALSE) return FIOBJ_T_FALSE;
    return FIOBJ_T_TRUE;
}
bool
_deleteDir(char **json, char *path,char *name) {
    return _delete(json,path,name);
}

bool
_deleteFile(char **json,char *path,char *name) {
    return _delete(json,path,name);
}

FIOBJ
_getChild(FIOBJ root, char *path) {
    FIOBJ current = FIOBJ_INVALID;
    if((current = __cd(root,path)) == FIOBJ_INVALID) return FIOBJ_T_FALSE;
    return current;
}

FIOBJ
_listDir(FIOBJ root, char* path) {
    return __list(root,path);
}