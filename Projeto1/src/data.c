#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/data.h"

/* Função que cria um novo elemento de dados data_t e que inicializa 
 * os dados de acordo com os argumentos recebidos, sem necessidade de
 * reservar memória para os dados.	
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct data_t *data_create(int size, void *data){
    if(size <= 0 || data == NULL)
        return NULL;
    
    struct data_t* d = malloc(sizeof(struct data_t));
    if(d==NULL)
        return NULL;

    d->data = data;
    d->datasize = size;
    
    return d;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int data_destroy(struct data_t *data){

    if(data != NULL){
        if(data->data != NULL)
            free(data->data);   
        else 
            free(data);
        return 0;
    }
    return -1;
    
}

/* Função que duplica uma estrutura data_t, reservando a memória
 * necessária para a nova estrutura.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct data_t *data_dup(struct data_t *data){
    if(data == NULL)
        return NULL;

    struct data_t* newData;
    newData = data_create(data->datasize, data->data);
    if(newData == NULL)
        return NULL;

    newData->data = malloc(data->datasize);
    if(newData->data == NULL){
        free(newData);
        return NULL;
    }
    memcpy(newData->data, data->data, data->datasize);

    return newData;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
 * Deve assegurar que liberta o espaço ocupado pelo conteúdo antigo.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int data_replace(struct data_t *data, int new_size, void *new_data){
    if(data == NULL || new_size <= 0 || new_data == NULL) {
        return -1;
    }

    free(data->data);

    data->datasize = new_size;
    memcpy(data->data, new_data, new_size);

    return 0;
}
