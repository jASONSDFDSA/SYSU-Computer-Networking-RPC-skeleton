#include "rpc.h"
#include "cJSON.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

extern rpc_data* add2_i8(rpc_data* in);
extern rpc_data* multiply(rpc_data* in);
extern rpc_data* subtract(rpc_data* in);
extern rpc_data* divide(rpc_data* in);
extern rpc_data* square(rpc_data* in);
extern rpc_data* cube(rpc_data* in);

int IsValidIpv6(const char *hostname)
{
	struct sockaddr_in6 addr;
	
	if (!hostname) return -1;
	if (strchr(hostname, '.')) return -1;
	if (inet_pton(AF_INET6, hostname, &addr.sin6_addr) != 1) return -1;
    	
	return 0;
}

int IsValidIpv4(const char *ip)
{
    if (ip == NULL) return -1;

    struct in_addr s;
    if(inet_pton(AF_INET, (char *)ip, (void *)&s) != 1) return -1;
    
    return 0;
}


rpc_register_center *rpc_init_register(int port, const char* ip_addr) {

    if (IsValidIpv6(ip_addr) == 0 || IsValidIpv4(ip_addr) == 0) {

        struct sockaddr_storage reg_addr;
        socklen_t addr_len;

        // Check format
        if (IsValidIpv6(ip_addr) == 0) {
            reg_addr.ss_family = AF_INET6;
            addr_len = sizeof(struct sockaddr_in6);
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&reg_addr;
            inet_pton(AF_INET6, ip_addr, &(addr6->sin6_addr));
            addr6->sin6_port = htons(port);
        } else {
            reg_addr.ss_family = AF_INET;
            addr_len = sizeof(struct sockaddr_in);
            struct sockaddr_in* addr4 = (struct sockaddr_in*)&reg_addr;
            addr4->sin_addr.s_addr = inet_addr(ip_addr);
            addr4->sin_port = htons(port);
        }

        // Create socket.
        int family = reg_addr.ss_family;
        int reg_sock = socket(family, SOCK_STREAM, 0);
        if (reg_sock < 0) {
            perror("Cannot create socket.");
            exit(EXIT_FAILURE);
        }

        // Bind socket
        if (bind(reg_sock, (struct sockaddr*)&reg_addr, addr_len) < 0) {
            perror("Bind failed.\n");
            exit(EXIT_FAILURE);
        }

        // Print the info.
        char ip_string[INET6_ADDRSTRLEN];
        if (family == AF_INET6) {
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&reg_addr;
            printf("IPv6 Bind complete, port number: %d, ip address: %s\n", ntohs(addr6->sin6_port), inet_ntop(AF_INET6, &(addr6->sin6_addr), ip_string, INET6_ADDRSTRLEN));
        } else {
            struct sockaddr_in* addr4 = (struct sockaddr_in*)&reg_addr;
            printf("IPv4 Bind complete, port number: %d, ip address: %s\n", ntohs(addr4->sin_port), inet_ntoa(addr4->sin_addr));
        }
        // Start listening.
        if(listen(reg_sock, BACKLOG) < 0) {
            perror("Listen failed.\n");
            exit(EXIT_FAILURE);
        }
        printf("Listening...\n");

        // Create an output register center state.
        rpc_register_center* new_reg = NULL;
        new_reg = (rpc_register_center*)malloc(sizeof(rpc_register_center));
        new_reg->register_addr = *(struct sockaddr*)&reg_addr;
        new_reg->register_sock = reg_sock;
        new_reg->backlog = BACKLOG;
        new_reg->type = family;

        return new_reg;

    } else {

        // IP address invalid.
        printf("Invalid IP address.");
        return NULL;

    }

    // Shouldn't reach here.
    return NULL;
}

void rpc_serve_both(rpc_register_center *srv, register_center* reg) {

    // Try to handle requests.
    while (true) { 
        rpc_client* cl = (rpc_client*)malloc(sizeof(rpc_client));

        int nSize = sizeof(cl->client_addr);
        cl->client_sock = accept(srv->register_sock, (struct sockaddr*)&(cl->client_addr), (socklen_t*)&nSize);
        if(cl->client_sock < 0) {
            perror("Accept error!/n");
            continue;
        }

        // Put the data into param.
        worker_data* param = (worker_data*)malloc(sizeof(worker_data));
        param->cl = (rpc_client*)malloc(sizeof(rpc_client));
        *param->cl = *cl;
        param->reg = reg;
        for (int i = 0; i < reg->size; i++) {
            param->reg->handler_server_ip[i] = strdup(reg->handler_server_ip[i]);
        }

        // Create new thread to handle the request.
        pthread_t pid;
        int res = pthread_create(&pid, NULL, rpc_worker, (void* )param);
        while (res != 0) {
            printf("The clerk maybe asleep. Try to wake him up!/n");
            sleep(1);
            res = pthread_create(&pid, NULL, rpc_worker, (void* )param);
        }

        if (res == 0) {
            printf("Worker is at work!\n");
            free((void*)cl);
            pthread_detach(pid);
        }
    }
}

void* rpc_worker(void* param) {
    // Retrieve data from param.

    worker_data parameter = *(worker_data*)param;
    rpc_client client = *(parameter.cl);
    register_center* RegisterCenter = parameter.reg;

    char buffer[1500] = {0};
    int recv_len;

    // Error dectection.
    struct timeval tv_out;
    tv_out.tv_sec = 5;
    tv_out.tv_usec = 0;
    setsockopt(client.client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    setsockopt(client.client_sock, SOL_SOCKET, SO_SNDTIMEO, &tv_out, sizeof(tv_out));


    // Receive data and handle.
    recv_len = recv(client.client_sock, (void*)&buffer, sizeof(buffer), 0);

    if (recv_len <= 0) {
        printf("Receive error or opposite close the connection.\n");
        close(client.client_sock);
        free(param);
        return NULL;
    }

    // Parse received data.
    cJSON* json = NULL;
    json = cJSON_Parse(buffer);

    // Process the data.
    cJSON* send_from_server = NULL;
    cJSON* data1 = NULL;
    cJSON* data2_len = NULL;
    cJSON* data2 = NULL;
    cJSON* d2_data2 = NULL;
    send_from_server = cJSON_GetObjectItem(json, "send_from_server");
    data1 = cJSON_GetObjectItem(json, "data1");
    data2_len = cJSON_GetObjectItem(json, "data2_len");
    data2 = cJSON_GetObjectItem(json, "data2");
    d2_data2 = cJSON_GetObjectItem(data2, "data2");

    // Prepare message to response.
    cJSON* confirm_message = NULL;
    confirm_message = cJSON_CreateObject();

    if (cJSON_IsTrue(send_from_server) == 1) {

        // Process the data from server.
        if (RegisterCenter->size < RegisterCenter->capacity) {

            // Check if the service has been registered.
            bool isRegistered = false;
            for (int i = 0; i < RegisterCenter->size; i++) {
                if (strcmp(d2_data2->valuestring, RegisterCenter->handler_names[i]) == 0)
                {
                    isRegistered = true;
                    break;
                }
            }

            if (!isRegistered) {

                // Register the service.
                int index = RegisterCenter->size;
                RegisterCenter->size++;
                RegisterCenter->handler_names[index] = (char*)malloc(strlen(d2_data2->valuestring) + 1);

                if (client.client_addr.sa_family == AF_INET) {
                    // IPv4 address handle.
                    struct sockaddr_in* addr4 = (struct sockaddr_in*)&(client.client_addr);
                    RegisterCenter->handler_server_ip[index] = (char*)malloc(INET_ADDRSTRLEN);
                    strcpy(RegisterCenter->handler_server_ip[index], inet_ntoa(addr4->sin_addr));
                } else if (client.client_addr.sa_family == AF_INET6) {
                    // IPv6 address handle.
                    struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&(client.client_addr);
                    RegisterCenter->handler_server_ip[index] = (char*)malloc(INET6_ADDRSTRLEN);
                    inet_ntop(AF_INET6, &(addr6->sin6_addr), RegisterCenter->handler_server_ip[index], INET6_ADDRSTRLEN);
                }

                RegisterCenter->handler_server_port[index] = data1->valueint;
                strcpy(RegisterCenter->handler_names[index], d2_data2->valuestring);

                // Fill confirm message.
                cJSON_AddStringToObject(confirm_message, "response", "Success!");
                printf("Register %s success!\n", RegisterCenter->handler_names[index]);

            } else {

                // Duplicated register.
                cJSON_AddStringToObject(confirm_message, "response", "Failed!");
                printf("The service has been registered before.");

            }
        } else {

            // Register number at limit.
            cJSON_AddStringToObject(confirm_message, "response", "Failed!");
            printf("Register Center is full.\n");

        }

        // Send response.
        char* response = cJSON_PrintUnformatted(confirm_message);
        if(send(client.client_sock, response, strlen(response), 0) == -1) {
            perror("Response Failed.\n");
        } else {
            printf("Confirm message sent.\n");
        }
        printf("RIP: %s\n", RegisterCenter->handler_server_ip[0]);

    } else { // Process the data from client.

        if (cJSON_IsTrue(cJSON_GetObjectItem(json, "find_request")) == 1) { // Return handle to the client.

            bool isRegistered = false;
            int index = -1;
            for (int i = 0; i < RegisterCenter->size; i++) {
                if (strcmp(d2_data2->valuestring, RegisterCenter->handler_names[i]) == 0)
                {
                    isRegistered = true;
                    index = i;
                    break;
                }
            }

            cJSON* cResponse = cJSON_CreateObject();

            if (isRegistered) {

                // Prepare response message.
                rpc_handle srv_handle = {
                    .num = 0, 
                    .srv = {
                        .ip_addr = RegisterCenter->handler_server_ip[index], .port = RegisterCenter->handler_server_port[index]
                    }
                };

                // Packing stuff.
                cJSON* handle = cJSON_CreateObject();
                cJSON* srv_info = cJSON_CreateObject();
                cJSON_AddBoolToObject(cResponse, "response", true);
                cJSON_AddNumberToObject(handle, "num", srv_handle.num);
                cJSON_AddStringToObject(handle, "name", d2_data2->valuestring);
                cJSON_AddStringToObject(srv_info, "ip_addr", srv_handle.srv.ip_addr);
                cJSON_AddNumberToObject(srv_info, "port", srv_handle.srv.port);
                cJSON_AddItemToObject(handle, "srv", srv_info);
                cJSON_AddItemToObject(cResponse, "rpc_handle", handle);

            } else {

                // The service hasn't registered.
                cJSON_AddBoolToObject(cResponse, "response", false);

            }

            // Send response.
            char* response = cJSON_PrintUnformatted(cResponse);

            if(send(client.client_sock, response, strlen(response), 0) == -1) {
                perror("Response Failed.\n");
            } else if (isRegistered) {
                printf("Message sent.\n");
            } else {
                printf("Message sent. Service not found.\n");
            }
        }
    }

    // End connection
    printf("End of the connection.\n\n\n");
    close(client.client_sock);

    return NULL;
}

/*    Init server using the ip address and port passed in. Create a socket base on it.  */
rpc_server* rpc_init_server(int port, const char* ip_addr, int reg_port, const char* reg_ip) {

    if (IsValidIpv6(ip_addr) == 0 || IsValidIpv4(ip_addr) == 0) {

        struct sockaddr_storage server_addr;
        socklen_t addr_len;

        // Check format
        if (IsValidIpv6(ip_addr) == 0) {
            addr_len = sizeof(struct sockaddr_in6);
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&server_addr;
            inet_pton(AF_INET6, ip_addr, &(addr6->sin6_addr));
            addr6->sin6_port = htons(port);
            server_addr.ss_family = AF_INET6;
        } else {
            addr_len = sizeof(struct sockaddr_in);
            struct sockaddr_in* addr4 = (struct sockaddr_in*)&server_addr;
            addr4->sin_addr.s_addr = inet_addr(ip_addr);
            addr4->sin_port = htons(port);
            server_addr.ss_family = AF_INET;
            printf("IPv4:%s\n", inet_ntoa(addr4->sin_addr));
        }

        // Create socket.
        int family = server_addr.ss_family;
        int server_sock = socket(family, SOCK_STREAM, 0);
        if (server_sock < 0) {
            perror("Cannot create socket.");
            exit(EXIT_FAILURE);
        }

        // Bind socket
        if (bind(server_sock, (struct sockaddr*)&server_addr, addr_len) < 0) {
            perror("Bind failed.\n");
            exit(EXIT_FAILURE);
        }

        // Print the info.
        char ip_string[INET6_ADDRSTRLEN];
        if (family == AF_INET6) {
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&server_addr;
            printf("IPv6 Bind complete, port number: %d, ip address: %s\n", ntohs(addr6->sin6_port), inet_ntop(AF_INET6, &(addr6->sin6_addr), ip_string, INET6_ADDRSTRLEN));
        } else {
            struct sockaddr_in* addr4 = (struct sockaddr_in*)&server_addr;
            printf("IPv4 Bind complete, port number: %d, ip address: %s\n", ntohs(addr4->sin_port), inet_ntoa(addr4->sin_addr));
        }

        // Start listening.
        if(listen(server_sock, BACKLOG) < 0) {
            perror("Listen failed.\n");
            exit(EXIT_FAILURE);
        }
        printf("Listening...\n");

        // Create handler map.
        handler_map* server_handlers = (handler_map*)malloc(sizeof(handler_map));
        server_handlers->size = 0;

        // Create an output server state.
        rpc_server* new_server = NULL;
        new_server = (rpc_server* )malloc(sizeof(rpc_server));
        new_server->server_addr = *(struct sockaddr*)&server_addr;
        new_server->server_sock = server_sock;
        new_server->server_handlers = server_handlers;
        new_server->reg_addr = (char*)malloc(strlen(reg_ip) + 1);
        strcpy(new_server->reg_addr, reg_ip);
        new_server->reg_port = reg_port;
        new_server->backlog = BACKLOG;
        new_server->type = family;
        new_server->port = 5000;

        if(IsValidIpv6(reg_ip) == 0) {
            new_server->reg_type = AF_INET6;
        } else if (IsValidIpv4(reg_ip) == 0) {
            new_server->reg_type = AF_INET;
        } else {
            printf("Invalid IP address.\n");
            return NULL;
        }

        return new_server;

    } else {

        // IP address invalid.
        printf("Invalid IP address.\n");
        return NULL;

    }

    // Shouldn't reach here.
    return NULL;


}

int rpc_register(rpc_server* srv, char* name, rpc_handler handler) {

    // Create socket.
    int family = srv->type;
    int server_sock = socket(family, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Cannot create socket.");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage server_addr;
    socklen_t addr_len;
    server_addr.ss_family = family;

    if (family == AF_INET6) {
        addr_len = sizeof(struct sockaddr_in6);
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&server_addr;
        addr6->sin6_port = htons(srv->port++);
        inet_pton(AF_INET6, srv->reg_addr, &(addr6->sin6_addr));
    } else if (family == AF_INET) {
        addr_len = sizeof(struct sockaddr_in);
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&server_addr;
        addr4->sin_port = htons(srv->port++);
        addr4->sin_addr.s_addr = inet_addr(srv->reg_addr);
    } else {
        printf("Invalid ip address.\n");
    }

    // Bind socket
    if (bind(server_sock, (struct sockaddr*)&server_addr, addr_len) < 0) {
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }

    // Print the info.
    char ip_string[INET6_ADDRSTRLEN];
    if (family == AF_INET6) {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&server_addr;
        printf("IPv6 Bind complete, port number: %d, ip address: %s\n", ntohs(addr6->sin6_port), inet_ntop(AF_INET6, &(addr6->sin6_addr), ip_string, INET6_ADDRSTRLEN));
    } else {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&server_addr;
        printf("IPv4 Bind complete, port number: %d, ip address: %s\n", ntohs(addr4->sin_port), inet_ntoa(addr4->sin_addr));
    }

    // Error dectection.
    struct timeval tv_out;
    tv_out.tv_sec = 5;
    tv_out.tv_usec = 0;
    setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    setsockopt(server_sock, SOL_SOCKET, SO_SNDTIMEO, &tv_out, sizeof(tv_out));

    cJSON* json = NULL;
    rpc_subdata data2 = {
        .data2 = name
    };

    int portnumber;

    if (srv->type == AF_INET) {
        portnumber = ntohs(((struct sockaddr_in*)&(srv->server_addr))->sin_port);
    } else if (srv->type == AF_INET6) {
        portnumber = ntohs(((struct sockaddr_in6*)&(srv->server_addr))->sin6_port);
    }
    rpc_data message = {
       .send_from_server = true, .data1 = portnumber, .data2_len = sizeof(data2), .data2 = (void*)&data2
    };

    // Packing data.
    json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "send_from_server", message.send_from_server);
    cJSON_AddNumberToObject(json, "data1", message.data1);
    cJSON_AddNumberToObject(json, "data2_len", message.data2_len);

    cJSON* cdata2 = cJSON_CreateObject();
    cJSON_AddStringToObject(cdata2, "data2", data2.data2);
    cJSON_AddItemToObject(json, "data2", cdata2);
    
    char* to_be_sent = cJSON_PrintUnformatted(json);

    // Send data.
    struct sockaddr_storage registerAddr;
    memset(&registerAddr, 0, sizeof(registerAddr));
    if (srv->reg_type == AF_INET) {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&registerAddr;
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(srv->reg_port);
        addr4->sin_addr.s_addr = inet_addr(srv->reg_addr);
    } else {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&registerAddr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(srv->reg_port);
        inet_pton(AF_INET6, srv->reg_addr, &(addr6->sin6_addr));
    }
    
    int res = connect(server_sock, (struct sockaddr*)&registerAddr, sizeof(struct sockaddr));
    if (res == -1) {
        perror("Connect failed.\n");
        exit(EXIT_FAILURE);
    } 

    if (send(server_sock, to_be_sent, strlen(to_be_sent), 0)) {
        printf("Sending\n");
        char buffer[100] = {0};
        if(recv(server_sock, (void*)&buffer, sizeof(buffer), 0)) {

            // Retrieve data.
            cJSON* response = NULL;
            response = cJSON_Parse(buffer);
            printf("%s\n", cJSON_GetObjectItem(response, "response")->valuestring);

            if (strcmp(cJSON_GetObjectItem(response, "response")->valuestring, "Success!") == 0) {

                // Register the function in server.
                int index = srv->server_handlers->size;
                srv->server_handlers->size++;
                srv->server_handlers->handlers[index] = handler;
                srv->server_handlers->handler_names[index] = (char*)malloc(strlen(name) + 1);
                strcpy(srv->server_handlers->handler_names[index], name);
                printf("%s has been registered.\n", srv->server_handlers->handler_names[index]);
                printf("\n\n\n");

            }

        } else {
            printf("The opposite didn't respond.\n");
            close(server_sock);
        }

        return 0;
    }

    // Shouldn't reach here.
    return -1;
}

void rpc_serve_all(rpc_server *srv) {

    // Try to handle requests.
    while (true) { 
        rpc_client* cl = (rpc_client*)malloc(sizeof(rpc_client));

        int nSize = sizeof(cl->client_addr);
        cl->client_sock = accept(srv->server_sock, (struct sockaddr*)&(cl->client_addr), (socklen_t*)&nSize);
        if(cl->client_sock < 0) {
            perror("Accept error!\n");
            continue;
        }

        // Put necessary data into param.
        clerk_data* param = (clerk_data*)malloc(sizeof(clerk_data));
        param->cl = (rpc_client*)malloc(sizeof(rpc_client));
        *param->cl = *cl;
        param->server_handlers = (handler_map*)malloc(sizeof(handler_map));
        *param->server_handlers = *srv->server_handlers;

        // Create new thread to handle the request.
        pthread_t pid;
        int res = pthread_create(&pid, NULL, rpc_clerk, (void* )param);
        while (res != 0) {
            printf("The clerk maybe asleep. Try to wake him up!\n");
            sleep(1);
            res = pthread_create(&pid, NULL, rpc_clerk, (void* )param);
        }

        printf("Clerk is at work!\n");
        pthread_detach(pid);
    }
}

void* rpc_clerk(void* param) {
    // Retrieve data from param.
    clerk_data parameters = *(clerk_data*)param;
    rpc_client cl = *(parameters.cl);
    handler_map* srv_han = parameters.server_handlers;
    
    char buffer[1500] = {0};
    int recv_len;

    struct timeval tv_out;
    tv_out.tv_sec = 5;
    tv_out.tv_usec = 0;
    setsockopt(cl.client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    setsockopt(cl.client_sock, SOL_SOCKET, SO_SNDTIMEO, &tv_out, sizeof(tv_out));


    // Receive data and handle.
    recv_len = recv(cl.client_sock, buffer, sizeof(buffer), 0);
    if (recv_len <= 0) {
        printf("Receive error or opposite close the connection.\n");
        close(cl.client_sock);
        free(param);
        return NULL;
    }

    // Parse received data.
    cJSON* json = NULL;
    json = cJSON_Parse(buffer);

    // Process the data.
    cJSON* send_from_server = NULL;
    cJSON* data1 = NULL;
    cJSON* data2_len = NULL;
    cJSON* data2 = NULL;
    cJSON* d2_data2 = NULL;
    cJSON* handle = NULL;
    send_from_server = cJSON_GetObjectItem(json, "send_from_server");
    data1 = cJSON_GetObjectItem(json, "data1");
    data2_len = cJSON_GetObjectItem(json, "data2_len");
    data2 = cJSON_GetObjectItem(json, "data2");
    d2_data2 = cJSON_GetObjectItem(data2, "data2");
    handle = cJSON_GetObjectItem(json, "handle");
    
    // Find the service.
    int index = -1;
    for(int i = 0; i < srv_han->size; i++) {
        if (strcmp(srv_han->handler_names[i], handle->valuestring) == 0) {
            index = i;
            printf("Service found.\n");
            break;
        }
    }

    // Call the real fuction. 
    rpc_handler vtuber = srv_han->handlers[index];

    // Pack up the data.
    rpc_data* deck = (rpc_data*)malloc(sizeof(rpc_data));
    deck->data1 = data1->valueint;
    deck->data2_len = data2_len->valueint;
    rpc_subdata* deck_data2 = (rpc_subdata*)malloc(sizeof(rpc_subdata));
    deck_data2->data2 = (char*)malloc(strlen(d2_data2->valuestring) + 1);
    strcpy(deck_data2->data2, d2_data2->valuestring);
    deck->data2 = (void*)(deck_data2);

    // Put deck into vtubers.
    rpc_data* response = vtuber(deck);
    if (response == NULL) {
        printf("Response error.\n");
        close(cl.client_sock);
        free(param);
        return NULL;
    }

    // Serialize the data.
    cJSON* response_data = NULL;
    response_data = cJSON_CreateObject();
    cJSON_AddStringToObject(response_data, "response", "Success!");
    cJSON_AddBoolToObject(response_data, "send_from_server", response->send_from_server);
    cJSON_AddNumberToObject(response_data, "data1", response->data1);
    cJSON_AddNumberToObject(response_data, "data2_len", response->data2_len);
    
    cJSON* cdata2 = NULL;
    cdata2 = cJSON_CreateObject();
    if (response->data2 == NULL) {
        cJSON_AddStringToObject(response_data, "data2", "NULL");
    } else {
        cJSON_AddStringToObject(cdata2, "data2", ((rpc_subdata*)response->data2)->data2);
        cJSON_AddItemToObject(response_data, "data2", cdata2);
    }

    char* to_be_sent = cJSON_PrintUnformatted(response_data);

    if(send(cl.client_sock, to_be_sent, strlen(to_be_sent), 0)) {
        printf("Response sent.\n");
    }

    printf("\n\n\n");

    close(cl.client_sock);
    free(param);
    return NULL;
}

rpc_client *rpc_init_client(const char *addr, int port) {
    
    // Create an output client state.
    rpc_client* new_client = (rpc_client*)malloc(sizeof(rpc_client));
    new_client->reg_addr = (char*)malloc(strlen(addr) + 1);
    strcpy(new_client->reg_addr, addr);
    new_client->reg_port = port;
    new_client->port = CLIENT_PORT + 1;
    if (IsValidIpv4(addr) == 0) {
        new_client->type = AF_INET;
    } else if (IsValidIpv6(addr) == 0) {
        new_client->type = AF_INET6;
    } else {
        printf("Invalid IP address.\n");
    }

    return new_client;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    // Create socket fd.
    SOCKET client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(client_sock < 0) {
        perror("Cannot create socket.");
        exit(EXIT_FAILURE);
    }

    // Bind socket to specific address and port passed in.
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    client_addr.sin_port = htons(cl->port++);
    if(bind(client_sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }
    printf("Bind complete, port number: %d, ip address: %s\n", ntohs(client_addr.sin_port), inet_ntoa(client_addr.sin_addr));


    // Initialize the handle to -1.
    rpc_handle* handle = (rpc_handle*)malloc(sizeof(rpc_handle));
    handle->num = -1;

    // Prepare the message.
    rpc_subdata data2 = {
        .data2 = name
    };
    rpc_data message = {
        .send_from_server = false, .data1 = 0, .data2_len = sizeof(data2), .data2 = (void* )&data2
    };

    // Packing data.
    cJSON* json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "send_from_server", message.send_from_server);
    cJSON_AddBoolToObject(json, "find_request", true);
    cJSON_AddNumberToObject(json, "data1", message.data1);
    cJSON_AddNumberToObject(json, "data2_len", message.data2_len);

    cJSON* cdata2 = cJSON_CreateObject();
    cJSON_AddStringToObject(cdata2, "data2", data2.data2);
    cJSON_AddItemToObject(json, "data2", cdata2);
    
    char* to_be_sent = cJSON_PrintUnformatted(json);

    // Send data.
    struct sockaddr_storage registerAddr;
    memset(&registerAddr, 0, sizeof(registerAddr));
    if (cl->type == AF_INET) {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&registerAddr;
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(cl->reg_port);
        addr4->sin_addr.s_addr = inet_addr(cl->reg_addr);
    } else {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&registerAddr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(cl->reg_port);
        inet_pton(AF_INET6, cl->reg_addr, &(addr6->sin6_addr));
    }

    struct timeval tv_out;
    tv_out.tv_sec = 5;
    tv_out.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, &tv_out, sizeof(tv_out));

    
    // Ask register center.
    int res = connect(client_sock, (struct sockaddr*)&registerAddr, sizeof(struct sockaddr));
    if (res == -1) {
        perror("Connect failed.");
        return NULL;
    }

    if (send(client_sock, to_be_sent, strlen(to_be_sent), 0)) {

        char buffer[1500] = {0};

        if(recv(client_sock, (void*)&buffer, sizeof(buffer), 0)) {
            cJSON* response = NULL;
            response = cJSON_Parse(buffer);
            if (cJSON_IsTrue(cJSON_GetObjectItem(response, "response")) == 1) { // Asked service exists.

                // Retrieve data.
                cJSON* cHandle = NULL;
                cJSON* num = NULL;
                cJSON* cname = NULL;
                cJSON* srv = NULL;
                cJSON* ip_addr = NULL;
                cJSON* port = NULL;
                cHandle = cJSON_GetObjectItem(response, "rpc_handle");
                num = cJSON_GetObjectItem(cHandle, "num");
                cname = cJSON_GetObjectItem(cHandle, "name");

                srv = cJSON_GetObjectItem(cHandle, "srv");
                ip_addr = cJSON_GetObjectItem(srv, "ip_addr");
                port = cJSON_GetObjectItem(srv, "port");


                handle->num = num->valueint;
                handle->name = cname->valuestring;
                handle->name = (char*)malloc(strlen(cname->valuestring) + 1);
                strncpy(handle->name, cname->valuestring, strlen(cname->valuestring) + 1);
                handle->srv.ip_addr = (char*)malloc(strlen(ip_addr->valuestring) + 1);
                strcpy(handle->srv.ip_addr, ip_addr->valuestring);
                handle->srv.port = port->valueint;
                if (IsValidIpv4(handle->srv.ip_addr) == 0) {
                    handle->type = AF_INET;
                } else if (IsValidIpv6(handle->srv.ip_addr) == 0) {
                    handle->type = AF_INET6;
                }

            } else { 
                // Asked service doesn't exist.
            }
        } else {
            printf("The opposite didn't respond.\n");
            close(client_sock);
        }
    } else {
        perror("Send error.\n");
    }

    // Return the handle
    if (handle->num != -1) {
        return handle;
    }
    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {

    // Create a new socket for the new connection
    SOCKET client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(client_sock < 0) {
        perror("Cannot create socket.");
        exit(EXIT_FAILURE);
    }

    // Bind socket to specific address and port passed in.
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    client_addr.sin_port = htons(cl->port++);
    if(bind(client_sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }

    // Error dectection.
    struct timeval tv_out;
    tv_out.tv_sec = 5;
    tv_out.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, &tv_out, sizeof(tv_out));

    
    // Serialize the data.
    cJSON* request_data = NULL;
    request_data = cJSON_CreateObject();
    cJSON_AddBoolToObject(request_data, "send_from_server", payload->send_from_server);
    cJSON_AddNumberToObject(request_data, "data1", payload->data1);
    cJSON_AddNumberToObject(request_data, "data2_len", payload->data2_len);
    
    cJSON* cdata2 = NULL;
    cdata2 = cJSON_CreateObject();
    cJSON_AddStringToObject(cdata2, "data2", ((rpc_subdata* )(payload->data2))->data2);
    cJSON_AddItemToObject(request_data, "data2", cdata2);

    cJSON_AddStringToObject(request_data, "handle", h->name);


    char* to_be_sent = cJSON_PrintUnformatted(request_data);

    // Call the remote function.
    struct sockaddr_storage serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    if (cl->type == AF_INET) {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&serverAddr;
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(h->srv.port);
        addr4->sin_addr.s_addr = inet_addr(h->srv.ip_addr);
    } else {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&serverAddr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(h->srv.port);
        inet_pton(AF_INET6, h->srv.ip_addr, &(addr6->sin6_addr));
    }
    
    int res = connect(client_sock, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr));
    if (res == -1) {
        perror("Connect failed.\n");
        return NULL;
    }

    if (send(client_sock, to_be_sent, strlen(to_be_sent), 0)) {
        char buffer[1500] = {0};
        if(recv(client_sock, (void*)&buffer, sizeof(buffer), 0)) {
            cJSON* response = NULL;
            response = cJSON_Parse(buffer);
            
            // Retrieve data.
            cJSON* csfs = cJSON_GetObjectItem(response, "send_from_server");
            cJSON* cd1 = cJSON_GetObjectItem(response, "data1");
            cJSON* cd2_len = cJSON_GetObjectItem(response, "data2_len");
            cJSON* cd2 = cJSON_GetObjectItem(response, "data2");

            rpc_data* rpc_res = (rpc_data*)malloc(sizeof(rpc_data));
            rpc_res->send_from_server = csfs->valuestring;
            rpc_res->data1 = cd1->valueint;
            rpc_res->data2_len = cd2_len->valueint;
            if (rpc_res->data2_len == 0) {
                rpc_res->data2 = NULL;
            } else {
                cJSON* cd2_d2 = cJSON_GetObjectItem(cd2, "data2");
                rpc_subdata* temp = (rpc_subdata*)malloc(sizeof(rpc_subdata));
                temp->data2 = (char*)malloc(strlen(cd2_d2->valuestring) + 1);
                strcpy(temp->data2, cd2_d2->valuestring);
                rpc_res->data2 = (void*)temp;
            }

            return rpc_res;
        } else {
            printf("The opposite didn't respond.\n");
            close(client_sock);
        }
    } else {
        perror("Send error.\n");
    }

    return NULL;
}

void rpc_close_client(rpc_client *cl) {
    free(cl);
}

void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
