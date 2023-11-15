#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "client_stub.h"


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

    signal(SIGPIPE, SIG_IGN);

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

            char *key = strtok(NULL, " \n");
            char *data = strtok(NULL, "\n");

            if (key == NULL || data == NULL) {
                printf("Comando put requer <key> e <data>\n\n");
            } else {
                char* tempData = strdup(data);
                struct data_t* datat = data_create(strlen(data), tempData);
                if (datat == NULL) {
                    printf("Erro ao criar data\n\n");
                }
                char* tempKey = strdup(key);

                struct entry_t* entry = entry_create(tempKey, datat);
                if (entry == NULL) {
                    printf("Erro ao criar entry\n\n");
                }

                int a = rtable_put(rtable, entry);
                if(a == -1)
                    printf("Erro ao inserir elemento\n\n");
                
                printf("Inserindo elemento: %s\n\n", entry->key);

                entry_destroy(entry);
            }
        } else if (strcmp(token, "get") == 0) {

            char *key = strtok(NULL, " \n");

            if (key == NULL) {
                printf("Comando get requer <key>\n\n");
            } else {

                struct data_t* data = rtable_get(rtable, key);
                if (data == NULL) {
                    printf("Elemento nao encontrado, ou erro ao obte-lo\n\n");
                }
                else{
                    char* str = malloc(data->datasize + 1);
                    memcpy(str, data->data, data->datasize);
                    str[data->datasize] = '\0';

                    printf("Elemento encontrado: %s\n\n", str);
                    free(str);
                }
                data_destroy(data);
            }
        } else if (strcmp(token, "del") == 0) {

            char *key = strtok(NULL, " \n");

            if (key == NULL) {
                printf("Comando del requer <key>\n\n");
            } else {
                
                int a = rtable_del(rtable, key);
                if(a == -1){
                    printf("Elemento nao encontrado, ou erro ao apaga-lo\n\n");
                }
                else{
                    printf("Elemento com a chave %s apagado\n\n", key);
                }
            }
        } else if (strcmp(token, "size") == 0) {

            int a = rtable_size(rtable);
            if(a == -1){
                printf("Erro ao obter tamanho da tabela\n\n");
            }
            else{
                printf("Tamanho da tabela: %d\n\n", a);
            }

        } else if (strcmp(token, "getkeys") == 0) {

            char** keys = rtable_get_keys(rtable);

            if(keys == NULL){
                printf("Erro ao obter chaves da tabela\n\n");
            }else if(keys[0] == NULL){
                printf("Tabela vazia\n\n");
            }else{
                printf("Chaves da tabela: \n");
                for (int i = 0; keys[i] != NULL; i++){
                    printf("%s \n", keys[i]);
                }
                printf("\n");
                rtable_free_keys(keys);
            }
        } else if (strcmp(token, "gettable") == 0) {

            struct entry_t** entries = rtable_get_table(rtable);

            if(entries == NULL){
                printf("Erro ao obter tabela\n\n");
            }else if(entries[0] == NULL){
                printf("Tabela vazia\n\n");
            }else{
                printf("Tabela: \n");
                for (int i = 0; entries[i] != NULL; i++){
                    struct entry_t* e = entries[i];

                    char* str = malloc(e->value->datasize + 1);
                    if(str == NULL){
                        printf("Erro ao alocar memoria para entrada\n\n");
                    }
                    memcpy(str, e->value->data, e->value->datasize);
                    str[e->value->datasize] = '\0';


                    printf("%s :: %s \n", entries[i]->key,str);
                    free(str);
                }
                printf("\n");
                rtable_free_entries(entries);
            }
            
        } else if (strcmp(token, "quit") == 0) {

            if(rtable_disconnect(rtable) == -1){
                printf("Erro ao desconectar do servidor\n\n");
            }
            printf("Bye bye client\n");
            break;
        } else {
            help();
        }
    }
    rtable_disconnect(rtable);
    exit(0);
    return 0;
}
