#ifndef CORE_H_
#define CORE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <openssl/evp.h>

#include <zdb.h>
/* Include the server library */
#include <fio.h>
#define FIO_INCLUDE_STR
/* Include the TLS, CLI, FIOBJ and HTTP / WebSockets extensions */
#include <fio_cli.h>
#include <fio_tls.h>
#include <http.h>
#include <redis_engine.h>
// this is passed as an argument to `fiobj_obj2json`
// change this to 1 to prettify.
#define PRETTY 1

#include "type.h"
#include "db.h"
#include "fss.h"
#include "util.h"

#endif