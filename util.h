#ifndef UTIL_H_
#define UTIL_H_

#include "core.h"
FIOBJ get_obj(FIOBJ hash,const char *key);

void put_obj(FIOBJ hash,const char *key,FIOBJ value);

char *randomString(size_t length);

char * digest_file(int fd);

FIOBJ _createDir(FIOBJ root, char *path, char *name);

FIOBJ _createFile(FIOBJ root, char *path, char *name, char *data);

FIOBJ _delete(FIOBJ root, char *path,char *name);

FIOBJ _getChild(FIOBJ root, char *path);

FIOBJ _listDir(FIOBJ root, char* path);

#endif
