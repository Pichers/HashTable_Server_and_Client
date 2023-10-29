#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/client_stub.h"


void help() {
        printf("Comandos disponíveis:\n");
        printf("put <key> <data>\n");
        printf("get <key>\n");
        printf("del <key>\n");
        printf("size\n");
        printf("getkeys\n");
        printf("gettable\n");
        printf("quit\n");
}

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
    help();
    while (1) {
        printf("Digite um comando: ");

        fgets(input, sizeof(input), stdin);
        token = strtok(input, " \n");

        if (token == NULL) {
            printf("Comando inválido. Tente novamente ou digite help para obter a lista de comandos\n");
            continue;
        }

        if (strcmp(token, "put") == 0) {
            // Processar comando put
            char *key = strtok(NULL, " \n");
            char *data = strtok(NULL, "\n");

            if (key == NULL || data == NULL) {
                printf("Comando put requer <key> e <data>\n");
            } else {
                struct entry_t* entry = entry_create(key, data);

                int a = rtable_put(rtable, entry);
                if(a == -1)
                    printf("Erro ao inserir elemento\n");
                
                entry_destroy(entry);
            }
        } else if (strcmp(token, "get") == 0) {
            // Processar comando get
            char *key = strtok(NULL, " \n");

            if (key == NULL) {
                printf("Comando get requer <key>\n");
            } else {
                // Chamar a função rtable_get
                // Implemente o código para chamar rtable_get aqui
                struct data_t* data = rtable_get(rtable, key);
                if (data == NULL) {
                    printf("Elemento nao encontrado, ou erro ao obte-lo\n");
                }
                else{
                    printf("Elemento encontrado: %s\n", data->data);
                }
                entry_destroy(data);
            }
        } else if (strcmp(token, "del") == 0) {
            // Processar comando del
            char *key = strtok(NULL, " \n");

            if (key == NULL) {
                printf("Comando del requer <key>\n");
            } else {
                
                // Chamar a função rtable_del
                // Implemente o código para chamar rtable_del aqui
                int a = rtable_del(rtable, key);
                if(a == -1){
                    printf("Elemento nao encontrado, ou erro ao apaga-lo\n");
                }
                else{
                    printf("Elemento com a chave %s apagado", key);
                }
            }
        } else if (strcmp(token, "size") == 0) {
            // Processar comando size
            // Chamar a função rtable_size
            // Implemente o código para chamar rtable_size aqui
            int a = rtable_size(rtable);
            if(a == -1){
                printf("Erro ao obter tamanho da tabela\n");
            }
            else{
                printf("Tamanho da tabela: %d\n", a);
            }

        } else if (strcmp(token, "getkeys") == 0) {
            // Processar comando getkeys
            // Chamar a função rtable_get_keys
            // Implemente o código para chamar rtable_get_keys aqui
            char** keys = rtable_get_keys(rtable);
            if(keys == NULL){
                printf("Erro ao obter chaves da tabela\n");
            }else{
                printf("Chaves da tabela: ");
                for (int i = 0; keys[i] != NULL; i++){
                    printf("%s ", keys[i]);
                }
            }
        } else if (strcmp(token, "gettable") == 0) {
            // Processar comando gettable
            // Chamar a função rtable_get_table
            // Implemente o código para chamar rtable_get_table aqui
            struct entry_t** entries = rtable_get_table(rtable);
            if(entries == NULL){
                printf("Erro ao obter tabela\n");
                //free(entries);
            }
            else{
                printf("Tabela: ");
                for (int i = 0; entries[i] != NULL; i++){
                    printf("%s :: %s", entries[i]->key, entries[i]->value->data);
                    entry_destroy(entries[i]);
                }
                //free(entries);
            }
        } else if (strcmp(token, "quit") == 0) {
            // Encerra o programa cliente
            rtable_free_entries(rtable_get_table);

            if(rtable_disconnect(rtable) == -1){
                printf("Erro ao desconectar do servidor\n");
            }

            break;
        } else {
            help();
        }
    }

    // Desconecta do servidor
    if (rtable_disconnect(rtable) == -1) {
        fprintf(stderr, "Erro ao desconectar do servidor\n");
    }

    return 0;
}
