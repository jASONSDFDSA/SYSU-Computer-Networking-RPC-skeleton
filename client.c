#include "rpc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {
    int exit_code = 0;
    if (argc < 2) {
        printf("Please enter '-h' to learn the details.\n");
        return 0;
    }

    if (strcmp(argv[1], "-h") == 0) {
        printf("Please enter the data in format as follow.\n-l (Register center IP address) -p (Register center Port)\n");
        return 0;
    }

    if (argc < 2 || strcmp(argv[1], "-l") != 0) {
        printf("Please enter '-h' to learn the details.\n");
        return 0;
    }

    char* client_ip = NULL;
    int client_port = -1;

    for (int i = 1; i < argc && i < 5; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            if(strcmp(argv[i + 1], "-p") != 0) {
                i++;
                client_ip = (char*)malloc(strlen(argv[i]) + 1);
                strcpy(client_ip, argv[i]);
            }
        }
        if (strcmp(argv[i], "-p") == 0) {
            if(i + 1 < argc && i + 1 < 5) {
                i++;
                client_port = atoi(argv[i]);
            }
        }
    }

    if (client_port == -1 || client_ip == NULL) {
        printf("Please enter port and ip.\n");
        return 0;
    }

    rpc_client *state = rpc_init_client(client_ip, client_port);
    if (state == NULL) {
        exit(EXIT_FAILURE);
    }

    rpc_handle *handle_add2 = rpc_find(state, "add2");
    rpc_handle *handle_multiply = rpc_find(state, "multiply");
    rpc_handle *handle_cube = rpc_find(state, "cube");
    if (handle_add2 == NULL) {
        fprintf(stderr, "ERROR: Function add2 does not exist\n");
        exit_code = 1;
        goto cleanup;
    }

    if (handle_multiply == NULL) {
        fprintf(stderr, "ERROR: Function multiply does not exist\n");
        exit_code = 1;
        goto cleanup;
    }

    if (handle_cube == NULL) {
        fprintf(stderr, "ERROR: Function cube does not exist\n");
        exit_code = 1;
        goto cleanup;
    }

    for (int i = 0; i < 2; i++) {
        /* Prepare request */
        int left_operand = i;
        char* right_operand = "100";
        rpc_subdata right_operand_s = {
            .data2 = right_operand
        };
        rpc_data request_data = {
           .send_from_server = false, .data1 = left_operand, .data2_len = sizeof(right_operand_s), .data2 = (void*)&right_operand_s
        };

        /* Call and receive response */
        rpc_data *response_data = rpc_call(state, handle_add2, &request_data);
        rpc_data *mult = rpc_call(state, handle_multiply, &request_data);
        rpc_data *cub = rpc_call(state, handle_cube, &request_data);
        if (response_data == NULL) {
            fprintf(stderr, "Function call of add2 failed\n");
            exit_code = 1;
            goto cleanup;
        }
        if (mult == NULL) {
            fprintf(stderr, "Function call of multiply failed\n");
            exit_code = 1;
            goto cleanup;
        }
        if (cub == NULL) {
            fprintf(stderr, "Function call of cube failed\n");
            exit_code = 1;
            goto cleanup;
        }

        /* Interpret response */
        assert(response_data->data2_len == 0);
        assert(response_data->data2 == NULL);
        printf("Result of adding %d and %s: %d\n", left_operand, right_operand,
               response_data->data1);
        printf("Result of multiplying %d and %s: %d\n", left_operand, right_operand,
               mult->data1);
        printf("Result of cubing %d: %d\n", left_operand, cub->data1);
        rpc_data_free(response_data);
        rpc_data_free(cub);
        rpc_data_free(mult);
    }

cleanup:
    if (handle_add2 != NULL) {
        free(handle_add2);
    }

    rpc_close_client(state);
    state = NULL;

    return exit_code;
}
