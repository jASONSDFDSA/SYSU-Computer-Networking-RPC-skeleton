#include "rpc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

rpc_data* add2_i8(rpc_data* in);
rpc_data* multiply(rpc_data* in);
rpc_data* subtract(rpc_data* in);
rpc_data* divide(rpc_data* in);
rpc_data* square(rpc_data* in);
rpc_data* cube(rpc_data* in);

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Please enter '-h' to learn the details.\n");
        return 0;
    }

    if (strcmp(argv[1], "-h") == 0) {
        printf("Please enter the data in format as follow.\n-l (Server IP address) -p (Server Port) -l (Register center IP address) -p (Register center port)\n");
        return 0;
    }

    if (strcmp(argv[1], "-l") != 0) {
        printf("Please enter '-h' to learn the details.\n");
        return 0;
    }

    rpc_server* state;
    char* server_ip = NULL;
    int server_port = -1;
    char* reg_ip = NULL;
    int reg_port = -1;

    bool isServerIP = true;
    bool isServerPort = true;

    for (int i = 1; i < argc && i < 9; i++) {
        if (strcmp(argv[i], "-l") == 0 && isServerIP) {
            if(strcmp(argv[i + 1], "-p") != 0 && i + 1 < argc) {
                i++;
                server_ip = (char*)malloc(strlen(argv[i]) + 1);
                strcpy(server_ip, argv[i]);
            }
            isServerIP = !isServerIP;
        }
        if (strcmp(argv[i], "-p") == 0 && isServerPort) {
            if(i + 1 < argc) {
                i++;
                server_port = atoi(argv[i]);
            }
            isServerPort = !isServerPort;
        }
        if (strcmp(argv[i], "-l") == 0 && !isServerIP) {
            if(strcmp(argv[i + 1], "-p") != 0 && i + 1 < argc) {
                i++;
                reg_ip = (char*)malloc(strlen(argv[i]) + 1);
                strcpy(reg_ip, argv[i]);
            }
        }
        if (strcmp(argv[i], "-p") == 0 && !isServerPort) {
            if(i + 1 < argc) {
                i++;
                reg_port = atoi(argv[i]);
            }
        }
    }

    if (server_port == -1 || server_ip == NULL) {
        printf("Please enter server port and ip.\n");
        return 0;
    }

    if (reg_port == -1 || reg_ip == NULL) {
        printf("Please enter register center port and ip.\n");
        return 0;
    }

    state = rpc_init_server(server_port, server_ip, reg_port, reg_ip);
    if (state == NULL) {
        fprintf(stderr, "Failed to init\n");
        exit(EXIT_FAILURE);
    }

    if (rpc_register(state, "add2", add2_i8) == -1) {
        fprintf(stderr, "Failed to register add2\n");
        exit(EXIT_FAILURE);
    }

    if (rpc_register(state, "multiply", multiply) == -1) {
        fprintf(stderr, "Failed to register multiply\n");
        exit(EXIT_FAILURE);
    }

    if (rpc_register(state, "subtract", subtract) == -1) {
        fprintf(stderr, "Failed to register subtract\n");
        exit(EXIT_FAILURE);
    }

    if (rpc_register(state, "divide", divide) == -1) {
        fprintf(stderr, "Failed to register divide\n");
        exit(EXIT_FAILURE);
    }

    if (rpc_register(state, "square", square) == -1) {
        fprintf(stderr, "Failed to register square\n");
        exit(EXIT_FAILURE);
    }

    if (rpc_register(state, "cube", cube) == -1) {
        fprintf(stderr, "Failed to register cube\n");
        exit(EXIT_FAILURE);
    }

    rpc_serve_all(state);

    free(state);
    state = NULL;
    return 0;
}

/* Adds 2 signed 8 bit numbers */
/* Uses data1 for left operand, data2 for right operand */
rpc_data* add2_i8(rpc_data* in) {
    /* Check data2 */
    if (in->data2 == NULL || in->data2_len == 0) {
        return NULL;
    }

    /* Parse request */
    int n1 = in->data1;
    int n2 = atoi(((rpc_subdata* )(in->data2))->data2);

    /* Perform calculation */
    printf("add2: arguments %d and %d\n", n1, n2);
    int res = n1 + n2;

    /* Prepare response */
    rpc_data *out = (rpc_data*)malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->send_from_server = true;
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}

rpc_data* multiply(rpc_data* in) {
    /* Check data2 */
    if (in->data2 == NULL || in->data2_len == 0) {
        return NULL;
    }

    /* Parse request */
    int n1 = in->data1;
    int n2 = atoi(((rpc_subdata* )(in->data2))->data2);

    /* Perform calculation */
    printf("multiply: arguments %d and %d\n", n1, n2);
    int res = n1 * n2;

    /* Prepare response */
    rpc_data *out = (rpc_data*)malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->send_from_server = true;
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}

rpc_data* subtract(rpc_data* in) {
    /* Check data2 */
    if (in->data2 == NULL || in->data2_len == 0) {
        return NULL;
    }

    /* Parse request */
    int n1 = in->data1;
    int n2 = atoi(((rpc_subdata* )(in->data2))->data2);

    /* Perform calculation */
    printf("subtract: arguments %d and %d\n", n1, n2);
    int res = n1 - n2;

    /* Prepare response */
    rpc_data *out = (rpc_data*)malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->send_from_server = true;
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}

rpc_data* divide(rpc_data* in) {
    /* Check data2 */
    if (in->data2 == NULL || in->data2_len == 0) {
        return NULL;
    }

    /* Parse request */
    int n1 = in->data1;
    int n2 = atoi(((rpc_subdata* )(in->data2))->data2);

    /* Perform calculation */
    printf("divide: arguments %d and %d\n", n1, n2);
    int res = n1 / n2;

    /* Prepare response */
    rpc_data *out = (rpc_data*)malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->send_from_server = true;
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}

rpc_data* square(rpc_data* in) {

    /* Parse request */
    int n1 = in->data1;

    /* Perform calculation */
    printf("square: arguments %d\n", n1);
    int res = n1 * n1;

    /* Prepare response */
    rpc_data *out = (rpc_data*)malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->send_from_server = true;
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}

rpc_data* cube(rpc_data* in) {

    /* Parse request */
    int n1 = in->data1;

    /* Perform calculation */
    printf("cube: arguments %d\n", n1);
    int res = n1 * n1 * n1;

    /* Prepare response */
    rpc_data *out = (rpc_data*)malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->send_from_server = true;
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}