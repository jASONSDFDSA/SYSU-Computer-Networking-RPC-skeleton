/* Header for RPC system */
/* Please do not modify this file */

#ifndef RPC_H
#define RPC_H

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>
#include <stdbool.h>

#define SOCKET int
#define BACKLOG 20
#define MAXIMUM_REGISTER_NUM 15
#define MAXIMUM_HANDLERS_NUMBER 15
#define RC_IP "0.0.0.0"
#define CLIENT_IP "127.0.0.1"
#define CLIENT_PORT 2000

/* The payload for requests/responses */
typedef struct {
    bool send_from_server;
    int data1;
    size_t data2_len;
    void *data2;
} rpc_data;

/* Handler for remote functions, which takes rpc_data* as input and produces
 * rpc_data* as output */
typedef rpc_data* (*rpc_handler)(rpc_data* );

/* Handler map in server */
typedef struct {
    rpc_handler handlers[MAXIMUM_HANDLERS_NUMBER];
    char* handler_names[MAXIMUM_HANDLERS_NUMBER];
    int size;
} handler_map;

/* Server state */
typedef struct rpc_server {
    /* Add variable(s) for server state */
    SOCKET server_sock;
    struct sockaddr server_addr;
    int backlog;
    char* reg_addr;
    int reg_port;
    handler_map* server_handlers;
    int type;
    int reg_type;
    int port;
} rpc_server;

/* Client state */
typedef struct rpc_client {
    SOCKET client_sock;
    struct sockaddr client_addr;
    char* reg_addr;
    int reg_port;
    int type;
    int port;
} rpc_client;

/* Register center state*/
typedef struct rpc_register_center {
    SOCKET register_sock;
    struct sockaddr register_addr;
    int backlog;
    int type;
} rpc_register_center;

/* The data inside data2 */
typedef struct {
    char* data2;
} rpc_subdata;

/* The format of server_info. */
typedef struct {
    char* ip_addr;
    uint16_t port;
} server_info;

/* Handle for remote function */
typedef struct {
    int num;
    server_info srv;
    char* name;
    int type;
} rpc_handle;

/* Register center, containing functions and its handle */
typedef struct {
    char* handler_names[MAXIMUM_REGISTER_NUM];
    uint16_t handler_server_port[MAXIMUM_REGISTER_NUM];
    size_t size;
    size_t capacity;
    char* handler_server_ip[MAXIMUM_REGISTER_NUM];
} register_center;

/* The data to be passed into the worker. */
typedef struct {
    rpc_client* cl;
    register_center* reg;
} worker_data;

/* The data to be passed into the clerk. */
typedef struct {
    rpc_client* cl;
    handler_map* server_handlers;
} clerk_data;

/* ------------------ */
/* Register functions */
/* ------------------ */

/* Initialises register state */
/* RETURNS: rpc_register_center* on success, NULL on error */
rpc_register_center *rpc_init_register(int port, const char* ip_addr);

/* Serve both server and client */
void rpc_serve_both(rpc_register_center *srv, register_center* reg);

/* Deal with a single request */
void* rpc_worker(void* param);

/* ---------------- */
/* Server functions */
/* ---------------- */

/* Initialises server state */
/* RETURNS: rpc_server* on success, NULL on error */
rpc_server *rpc_init_server(int port, const char* ip_addr, int reg_port, const char* reg_ip);

/* Registers a function (mapping from name to handler) */
/* RETURNS: -1 on failure */
int rpc_register(rpc_server *srv, char *name, rpc_handler handler);

/* Start serving requests */
void rpc_serve_all(rpc_server *srv);

/* Client handler, takes a pointer to function as an input and rpc_data as an output. */
void* rpc_clerk(void* cl);

/* ---------------- */
/* Client functions */
/* ---------------- */

/* Initialises client state */
/* RETURNS: rpc_client* on success, NULL on error */
rpc_client *rpc_init_client(const char *addr, int port);

/* Finds a remote function by name */
/* RETURNS: rpc_handle* on success, NULL on error */
/* rpc_handle* will be freed with a single call to free(3) */
rpc_handle *rpc_find(rpc_client *cl, char *name);

/* Calls remote function using handle */
/* RETURNS: rpc_data* on success, NULL on error */
rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload);

/* Cleans up client state and closes client */
void rpc_close_client(rpc_client *cl);

/* ---------------- */
/* Shared functions */
/* ---------------- */

/* Frees a rpc_data struct */
void rpc_data_free(rpc_data *data);

/* Check if the ip address */
int IsValidIpv4(const char *ip);
int IsValidIpv6(const char *hostname);

#endif
