#include "rpc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Please enter '-h' to learn the details.\n");
        return 0;
    }

    if (strcmp(argv[1], "-h") == 0) {
        printf("Please enter the data in format as follow.\n-l (Register center IP address) -p (Register center Port)\n");
        return 0;
    }

    if (strcmp(argv[1], "-l") != 0) {
        printf("Please enter '-h' to learn the details.\n");
        return 0;
    }

    rpc_register_center* state;
    char* rc_ip = (char*)RC_IP;
    int rc_port = -1;

    for (int i = 1; i < argc && i < 5; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            if(strcmp(argv[i + 1], "-p") != 0) {
                i++;
                rc_ip = (char*)malloc(strlen(argv[i]) + 1);
                strcpy(rc_ip, argv[i]);
            }
        }
        if (strcmp(argv[i], "-p") == 0) {
            if(i + 1 < argc && i + 1 < 5) {
                i++;
                rc_port = atoi(argv[i]);
            }
        }
    }

    if (rc_port == -1) {
        printf("Please enter port.\n");
        return 0;
    }

    state = rpc_init_register(rc_port, rc_ip);
    if (state == NULL) {
        fprintf(stderr, "Failed to init\n");
        exit(EXIT_FAILURE);
    }

    // Init register center.
    register_center* RegisterCenter = (register_center*)malloc(sizeof(register_center));
    RegisterCenter->capacity = MAXIMUM_REGISTER_NUM;
    RegisterCenter->size = 0;

    rpc_serve_both(state, RegisterCenter);

    free(state);
    state = NULL;
    return 0;
}
