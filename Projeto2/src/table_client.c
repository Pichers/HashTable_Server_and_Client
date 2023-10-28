#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/client_stub.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <server_address:port>\n", argv[0]);
        exit(1);
    }

    // Conecta ao servidor
    struct rtable_t *rtable = rtable_connect(argv[1]);
    if (rtable == NULL) {
        fprintf(stderr, "Falha ao conectar ao servidor\n");
        exit(1);
    }

    char input[256];
    char *token;

    while (1) {
        printf("Comandos disponíveis:\n");
        printf("put <key> <data>\n");
        printf("get <key>\n");
        printf("del <key>\n");
        printf("size\n");
        printf("getkeys\n");
        printf("gettable\n");
        printf("quit\n");
        printf("Digite um comando: ");

        fgets(input, sizeof(input), stdin);
        token = strtok(input, " \n");

        if (token == NULL) {
            printf("Comando inválido. Tente novamente.\n");
            continue;
        }

        if (strcmp(token, "put") == 0) {
            // Processar comando put
            char *key = strtok(NULL, " \n");
            char *data = strtok(NULL, "\n");

            if (key == NULL || data == NULL) {
                printf("Comando put requer <key> e <data>\n");
            } else {
                // Chamar a função rtable_put
                // Implemente o código para chamar rtable_put aqui
            }
        } else if (strcmp(token, "get") == 0) {
            // Processar comando get
            char *key = strtok(NULL, " \n");

            if (key == NULL) {
                printf("Comando get requer <key>\n");
            } else {
                // Chamar a função rtable_get
                // Implemente o código para chamar rtable_get aqui
            }
        } else if (strcmp(token, "del") == 0) {
            // Processar comando del
            char *key = strtok(NULL, " \n");

            if (key == NULL) {
                printf("Comando del requer <key>\n");
            } else {
                // Chamar a função rtable_del
                // Implemente o código para chamar rtable_del aqui
            }
        } else if (strcmp(token, "size") == 0) {
            // Processar comando size
            // Chamar a função rtable_size
            // Implemente o código para chamar rtable_size aqui
        } else if (strcmp(token, "getkeys") == 0) {
            // Processar comando getkeys
            // Chamar a função rtable_get_keys
            // Implemente o código para chamar rtable_get_keys aqui
        } else if (strcmp(token, "gettable") == 0) {
            // Processar comando gettable
            // Chamar a função rtable_get_table
            // Implemente o código para chamar rtable_get_table aqui
        } else if (strcmp(token, "quit") == 0) {
            // Encerra o programa cliente
            break;
        } else {
            printf("Comando desconhecido. Tente novamente.\n");
        }
    }

    // Desconecta do servidor
    if (rtable_disconnect(rtable) == -1) {
        fprintf(stderr, "Erro ao desconectar do servidor\n");
    }

    return 0;
}
