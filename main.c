/**
In this a Hello World example using the bundled HTTP / WebSockets extension.
Compile using:
    NAME=http make
Or test the `poll` engine's performance by compiling with `poll`:
    FIO_POLL=1 NAME=http make
Run with:
    ./tmp/http -t 1
Benchmark with keep-alive:
    ab -c 200 -t 4 -n 1000000 -k http://127.0.0.1:3000/
    wrk -c200 -d4 -t1 http://localhost:3000/
Benchmark with higher load:
    ab -c 4400 -t 4 -n 1000000 -k http://127.0.0.1:3000/
    wrk -c4400 -d4 -t1 http://localhost:3000/
Use a javascript console to connect to the WebSocket chat service... maybe using
the following javascript code:
    // run 1st client app on port 3000.
    ws = new WebSocket("ws://localhost:3000/Mitchel");
    ws.onmessage = function(e) { console.log(e.data); };
    ws.onclose = function(e) { console.log("closed"); };
    ws.onopen = function(e) { e.target.send("Yo!"); };
    // run 2nd client app on port 3030, to test Redis
    ws = new WebSocket("ws://localhost:3030/Johana");
    ws.onmessage = function(e) { console.log(e.data); };
    ws.onclose = function(e) { console.log("closed"); };
    ws.onopen = function(e) { e.target.send("Brut."); };
It's possible to use SSE (Server-Sent-Events / EventSource) for listening in on
the chat:
    var source = new EventSource("/Watcher");
    source.addEventListener('message', (e) => { console.log(e.data); });
    source.addEventListener('open', (e) => {
      console.log("SSE Connection open.");
    }); source.addEventListener('close', (e) => {
      console.log("SSE Connection lost."); });
Remember that published messages will now be printed to the console both by
Mitchel and Johana, which means messages will be delivered twice unless using
two different browser windows.
*/

#include "core.h"

/* *****************************************************************************
The main function
***************************************************************************** */

/* HTTP request handler */
static void on_http_request(http_s *h);
/* HTTP upgrade request handler */
static void on_http_upgrade(http_s *h, char *requested_protocol, size_t len);
/* Command Line Arguments Management */
static void initialize_cli(int argc, char const *argv[]);
/* Initializes Redis, if set by command line arguments */
static void initialize_redis(void);
/* Initializes Database */
ConnectionPool_T pool;
static void initialize_database(void) {
    URL_T url = URL_new("mysql://fss.mysql.database.azure.com/fss?user=fss&password=YuHaiyang!");
    pool = ConnectionPool_new(url);
    ConnectionPool_start(pool);
}

int main(int argc, char const *argv[]) {
    initialize_cli(argc, argv);
    initialize_redis();
    initialize_database();
    srand(time(0));
    /* TLS support */
    fio_tls_s *tls = NULL;
    if (fio_cli_get_bool("-tls")) {
        char local_addr[1024];
        local_addr[fio_local_addr(local_addr, 1023)] = 0;
        tls = fio_tls_new(local_addr, NULL, NULL, NULL);
    }
    /* optimize WebSocket pub/sub for multi-connection broadcasting */
    websocket_optimize4broadcasts(WEBSOCKET_OPTIMIZE_PUBSUB, 1);
    /* listen for inncoming connections */
    if (http_listen(fio_cli_get("-p"), fio_cli_get("-b"),
                    .on_request = on_http_request, .on_upgrade = on_http_upgrade,
                    .max_body_size = (fio_cli_get_i("-maxbd") * 1024 * 1024),
                    .ws_max_msg_size = (fio_cli_get_i("-maxms") * 1024),
                    .public_folder = fio_cli_get("-public"),
                    .log = fio_cli_get_bool("-log"),
                    .timeout = fio_cli_get_i("-keep-alive"), .tls = tls,
                    .ws_timeout = fio_cli_get_i("-ping")) == -1) {
        /* listen failed ?*/
        perror(
                "ERROR: facil.io couldn't initialize HTTP service (already running?)");
        exit(1);
    }
    fio_start(.threads = fio_cli_get_i("-t"), .workers = fio_cli_get_i("-w"));
    fio_cli_end();
    fio_tls_destroy(tls);
    return 0;
}

/* *****************************************************************************
HTTP Request / Response Handling
***************************************************************************** */
/* route handler */
bool http_path_matches(http_s *h, char *__route) {
    // make a mutable copy of the route
    char *route = strdup(__route);

    if (!route) {
        FIO_LOG_ERROR("Failed to allocate memory!");
        return false;
    }

    char *path = fiobj_obj2cstr(h->path).data;

    // truncate the path at the query string delimiter,
    // as we only care about the path itself and the
    // query string is parsed by http_parse_query()
    path = strtok(path, "?");

    // does the route contain any inline path variables?
    if (strchr(route, ':') == NULL) {
        // no - perform direct string comparison
        if (strcmp(route, path) == 0) {
            free(route);
            return true;
        } else {
            free(route);
            return false;
        }
    }

    int route_part_cnt = 0;
    int path_part_cnt  = 0;

    // count the number of parts in the route and the path
    for (int i = 0; route[i]; route[i] == '/' ? route_part_cnt++, i++ : i++);
    for (int i = 0; path[i];  path[i]  == '/' ? path_part_cnt++,  i++ : i++);

    // do we have an equal number of parts?
    if (route_part_cnt != path_part_cnt) {
        return false;
    }

    char *route_parts[route_part_cnt];
    char *path_parts[path_part_cnt];

    char *route_remaining;
    char *path_remaining;

    int matches = 0;

    // loop through each part

    char *route_next_part = strtok_r(route, "/", &route_remaining);
    char *path_next_part  = strtok_r(path,  "/", &path_remaining);

    while (route_next_part && path_next_part) {
        // if the route part is an inline variable, extract the variable name and its value
        if (route_next_part[0] == ':') {
            route_parts[matches]  = route_next_part + 1;
            path_parts[matches++] = path_next_part;
        } else {
            // the route part is literal, does it match the path part?
            if (strcmp(route_next_part, path_next_part)) {
                free(route);
                return false;
            }
        }

        route_next_part = strtok_r(NULL, "/", &route_remaining);
        path_next_part  = strtok_r(NULL, "/", &path_remaining);
    }

    free(route);

    // add the inline path variable names and values to the request params
    for (int i = 0; i < matches; i++) {
        http_add2hash(h->params, route_parts[i], strlen(route_parts[i]), path_parts[i], strlen(path_parts[i]), 1);
    }

    return true;
}

#define http_route_handler(_h, _method, _route, _func) {                                                \
    if (strcmp(fiobj_obj2cstr(h->method).data, _method) == 0 && http_path_matches(_h, _route)) {  \
        FIO_LOG_DEBUG("Matched route %s %s", _method, _route);                                          \
        _func(_h);                                                                                      \
        return;                                                                                         \
    }                                                                                                   \
}
#define http_route_get(_h, _route, _func) \
    http_route_handler(_h, "GET", _route, _func)

#define http_route_post(_h, _route, _func) \
    http_route_handler(_h, "POST", _route, _func)

#define http_route_put(_h, _route, _func) \
    http_route_handler(_h, "PUT", _route, _func)

#define http_route_delete(_h, _route, _func) \
    http_route_handler(_h, "DELETE", _route, _func)

static void on_http_request(http_s *h) {
    http_parse_query(h);
    http_parse_body(h);

    http_route_post(h, "/api/auth/user_register", fss_register);
    http_route_post(h, "/api/auth/user_login", fss_login);
    http_route_post(h, "/api/auth/forget_password", fss_get_user_question);
    http_route_post(h, "/api/auth/reset_password", fss_reset_password);
    http_route_post(h, "/api/fss/create_dir", fss_create_dir);
    http_route_post(h, "/api/fss/list_dir", fss_list_dir);
    http_route_post(h, "/api/fss/delete_file", fss_delete_file);
    http_route_post(h, "/api/fss/list_share", fss_list_share);
    http_route_post(h, "/api/fss/create_share", fss_create_share);
    http_route_post(h, "/api/fss/delete_share", fss_delete_share);
    http_route_post(h, "/api/fss/upload_file", fss_upload_file);

    http_send_error(h,404);
}

/* *****************************************************************************
HTTP Upgrade Handling
***************************************************************************** */

/* Server Sent Event Handlers */
static void sse_on_open(http_sse_s *sse);
static void sse_on_close(http_sse_s *sse);

/* WebSocket Handlers */
static void ws_on_open(ws_s *ws);
static void ws_on_message(ws_s *ws, fio_str_info_s msg, uint8_t is_text);
static void ws_on_shutdown(ws_s *ws);
static void ws_on_close(intptr_t uuid, void *udata);

/* HTTP upgrade callback */
static void on_http_upgrade(http_s *h, char *requested_protocol, size_t len) {
    /* Upgrade to SSE or WebSockets and set the request path as a nickname. */
    FIOBJ nickname;
    if (fiobj_obj2cstr(h->path).len > 1) {
        nickname = fiobj_str_new(fiobj_obj2cstr(h->path).data + 1,
                                 fiobj_obj2cstr(h->path).len - 1);
    } else {
        nickname = fiobj_str_new("Guest", 5);
    }
    /* Test for upgrade protocol (websocket vs. sse) */
    if (len == 3 && requested_protocol[1] == 's') {
        if (fio_cli_get_bool("-v")) {
            fprintf(stderr, "* (%d) new SSE connection: %s.\n", getpid(),
                    fiobj_obj2cstr(nickname).data);
        }
        http_upgrade2sse(h, .on_open = sse_on_open, .on_close = sse_on_close,
                         .udata = (void *)nickname);
    } else if (len == 9 && requested_protocol[1] == 'e') {
        if (fio_cli_get_bool("-v")) {
            fprintf(stderr, "* (%d) new WebSocket connection: %s.\n", getpid(),
                    fiobj_obj2cstr(nickname).data);
        }
        http_upgrade2ws(h, .on_message = ws_on_message, .on_open = ws_on_open,
                        .on_shutdown = ws_on_shutdown, .on_close = ws_on_close,
                        .udata = (void *)nickname);
    } else {
        fprintf(stderr, "WARNING: unrecognized HTTP upgrade request: %s\n",
                requested_protocol);
        http_send_error(h, 400);
        fiobj_free(nickname); // we didn't use this
    }
}

/* *****************************************************************************
Globals
***************************************************************************** */

static fio_str_info_s CHAT_CANNEL = {.data = "chat", .len = 4};

/* *****************************************************************************
HTTP SSE (Server Sent Events) Callbacks
***************************************************************************** */

/**
 * The (optional) on_open callback will be called once the EventSource
 * connection is established.
 */
static void sse_on_open(http_sse_s *sse) {
    http_sse_write(sse, .data = {.data = "Welcome to the SSE chat channel.\r\n"
            "You can only listen, not write.",
                   .len = 65});
    http_sse_subscribe(sse, .channel = CHAT_CANNEL);
    http_sse_set_timout(sse, fio_cli_get_i("-ping"));
    FIOBJ tmp = fiobj_str_copy((FIOBJ)sse->udata);
    fiobj_str_write(tmp, " joind the chat only to listen.", 31);
    fio_publish(.channel = CHAT_CANNEL, .message = fiobj_obj2cstr(tmp));
    fiobj_free(tmp);
}

static void sse_on_close(http_sse_s *sse) {
    /* Let everyone know we left the chat */
    fiobj_str_write((FIOBJ)sse->udata, " left the chat.", 15);
    fio_publish(.channel = CHAT_CANNEL,
                .message = fiobj_obj2cstr((FIOBJ)sse->udata));
    /* free the nickname */
    fiobj_free((FIOBJ)sse->udata);
}

/* *****************************************************************************
WebSockets Callbacks
***************************************************************************** */

static void ws_on_message(ws_s *ws, fio_str_info_s msg, uint8_t is_text) {
    // Add the Nickname to the message
    FIOBJ str = fiobj_str_copy((FIOBJ)websocket_udata_get(ws));
    fiobj_str_write(str, ": ", 2);
    fiobj_str_write(str, msg.data, msg.len);
    // publish
    fio_publish(.channel = CHAT_CANNEL, .message = fiobj_obj2cstr(str));
    // free the string
    fiobj_free(str);
    (void)is_text; // we don't care.
    (void)ws;      // this could be used to send an ACK, but we don't.
}
static void ws_on_open(ws_s *ws) {
    websocket_subscribe(ws, .channel = CHAT_CANNEL);
    websocket_write(
            ws, (fio_str_info_s){.data = "Welcome to the chat-room.", .len = 25}, 1);
    FIOBJ tmp = fiobj_str_copy((FIOBJ)websocket_udata_get(ws));
    fiobj_str_write(tmp, " joind the chat.", 16);
    fio_publish(.channel = CHAT_CANNEL, .message = fiobj_obj2cstr(tmp));
    fiobj_free(tmp);
}
static void ws_on_shutdown(ws_s *ws) {
    websocket_write(
            ws, (fio_str_info_s){.data = "Server shutting down, goodbye.", .len = 30},
            1);
}

static void ws_on_close(intptr_t uuid, void *udata) {
    /* Let everyone know we left the chat */
    fiobj_str_write((FIOBJ)udata, " left the chat.", 15);
    fio_publish(.channel = CHAT_CANNEL, .message = fiobj_obj2cstr((FIOBJ)udata));
    /* free the nickname */
    fiobj_free((FIOBJ)udata);
    (void)uuid; // we don't use the ID
}

/* *****************************************************************************
Redis initialization
***************************************************************************** */
static void initialize_redis(void) {
    if (!fio_cli_get("-redis") || !strlen(fio_cli_get("-redis")))
        return;
    FIO_LOG_STATE("* Initializing Redis connection to %s\n",
                  fio_cli_get("-redis"));
    fio_url_s info =
            fio_url_parse(fio_cli_get("-redis"), strlen(fio_cli_get("-redis")));
    fio_pubsub_engine_s *e =
            redis_engine_create(.address = info.host, .port = info.port,
                                .auth = info.password);
    if (e)
        fio_state_callback_add(FIO_CALL_ON_FINISH,
                               (void (*)(void *))redis_engine_destroy, e);
    FIO_PUBSUB_DEFAULT = e;
}

/* *****************************************************************************
CLI helpers
***************************************************************************** */
static void initialize_cli(int argc, char const *argv[]) {
    /*     ****  Command line arguments ****     */
    fio_cli_start(
            argc, argv, 0, 0, NULL,
    // Address Binding
            FIO_CLI_PRINT_HEADER("Address Binding:"),
            FIO_CLI_INT("-port -p port number to listen to. defaults port 3000"),
            "-bind -b address to listen to. defaults any available.",
            FIO_CLI_BOOL("-tls use a self signed certificate for TLS."),
    // Concurrency
            FIO_CLI_PRINT_HEADER("Concurrency:"),
            FIO_CLI_INT("-workers -w number of processes to use."),
            FIO_CLI_INT("-threads -t number of threads per process."),
    // HTTP Settings
            FIO_CLI_PRINT_HEADER("HTTP Settings:"),
            "-public -www public folder, for static file service.",
            FIO_CLI_INT(
                    "-keep-alive -k HTTP keep-alive timeout (0..255). default: 10s"),
            FIO_CLI_INT(
                    "-max-body -maxbd HTTP upload limit in Mega Bytes. default: 50Mb"),
            FIO_CLI_BOOL("-log -v request verbosity (logging)."),
    // WebSocket Settings
            FIO_CLI_PRINT_HEADER("WebSocket Settings:"),
            FIO_CLI_INT("-ping websocket ping interval (0..255). default: 40s"),
            FIO_CLI_INT("-max-msg -maxms incoming websocket message "
                        "size limit in Kb. default: 250Kb"),
    // Misc Settings
            FIO_CLI_PRINT_HEADER("Misc:"),
            FIO_CLI_STRING("-redis -r an optional Redis URL server address."),
            FIO_CLI_PRINT("\t\ta valid Redis URL would follow the pattern:"),
            FIO_CLI_PRINT("\t\t\tredis://user:password@localhost:6379/"),
            FIO_CLI_INT("-verbosity -V facil.io verbocity 0..5 (logging level)."));

    /* Test and set any default options */
    if (!fio_cli_get("-p")) {
        /* Test environment as well */
        char *tmp = getenv("PORT");
        if (!tmp)
            tmp = "3000";
        /* CLI et functions (unlike fio_cli_start) ignores aliases */
        fio_cli_set("-p", tmp);
        fio_cli_set("-port", tmp);
    }
    if (!fio_cli_get("-b")) {
        char *tmp = getenv("ADDRESS");
        if (tmp) {
            fio_cli_set("-b", tmp);
            fio_cli_set("-bind", tmp);
        }
    }
    if (!fio_cli_get("-public")) {
        char *tmp = getenv("HTTP_PUBLIC_FOLDER");
        if (tmp) {
            fio_cli_set("-public", tmp);
            fio_cli_set("-www", tmp);
        }
    }

    if (!fio_cli_get("-redis")) {
        char *tmp = getenv("REDIS_URL");
        if (tmp) {
            fio_cli_set("-redis", tmp);
            fio_cli_set("-r", tmp);
        }
    }
    if (fio_cli_get("-V")) {
        FIO_LOG_LEVEL = fio_cli_get_i("-V");
    }

    fio_cli_set_default("-ping", "40");

    /* CLI set functions (unlike fio_cli_start) ignores aliases */
    fio_cli_set_default("-k", "10");
    fio_cli_set_default("-keep-alive", "10");

    fio_cli_set_default("-max-body", "50");
    fio_cli_set_default("-maxbd", "50");

    fio_cli_set_default("-max-message", "250");
    fio_cli_set_default("-maxms", "250");
}