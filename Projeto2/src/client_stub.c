#include "data.h"
#include "entry.h"
#include "client_stub-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Remote table, que deve conter as informações necessárias para comunicar
 * com o servidor. A definir pelo grupo em client_stub-private.h
 */
struct rtable_t;

/* Função para estabelecer uma associação entre o cliente e o servidor, 
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna a estrutura rtable preenchida, ou NULL em caso de erro.
 */
struct rtable_t *rtable_connect(char *address_port){
    if(address_port == NULL)
        return NULL;
    
    struct rtable_t* rtable = malloc(sizeof(struct rtable_t));
    if(rtable == NULL){
        free(rtable);
        return NULL;
    }
    
    rtable->server_port = address_port;
    
    return rtable;
}
/* Termina a associação entre o cliente e o servidor, fechando a 
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem, ou -1 em caso de erro.
 */
int rtable_disconnect(struct rtable_t *rtable){
    if(rtable == NULL)
        return -1;
    
    free(rtable);
    return 0;
}

/* Função para adicionar um elemento na tabela.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Retorna 0 (OK, em adição/substituição), ou -1 (erro).
 */
int rtable_put(struct rtable_t *rtable, struct entry_t *entry){
    if(rtable == NULL || entry == NULL)
        return -1;
    
    return 0;
}

/* Retorna o elemento da tabela com chave key, ou NULL caso não exista
 * ou se ocorrer algum erro.
 */
struct data_t *rtable_get(struct rtable_t *rtable, char *key);

/* Função para remover um elemento da tabela. Vai libertar 
 * toda a memoria alocada na respetiva operação rtable_put().
 * Retorna 0 (OK), ou -1 (chave não encontrada ou erro).
 */
int rtable_del(struct rtable_t *rtable, char *key){
    if(rtable == NULL || key == NULL)
        return -1;
    
    return 0;
}

/* Retorna o número de elementos contidos na tabela ou -1 em caso de erro.
 */
int rtable_size(struct rtable_t *rtable){
    if(rtable == NULL)
        return -1;
    
    return 0;
}

/* Retorna um array de char* com a cópia de todas as keys da tabela,
 * colocando um último elemento do array a NULL.
 * Retorna NULL em caso de erro.
 */
char **rtable_get_keys(struct rtable_t *rtable){
    if(rtable == NULL)
        return NULL;
    
    return NULL;
}

/* Liberta a memória alocada por rtable_get_keys().
 */
void rtable_free_keys(char **keys){
    if(keys == NULL)
        return;
    
    free(keys);
}

/* Retorna um array de entry_t* com todo o conteúdo da tabela, colocando
 * um último elemento do array a NULL. Retorna NULL em caso de erro.
 */
struct entry_t **rtable_get_table(struct rtable_t *rtable){
    if(rtable == NULL)
        return NULL;
    
    return NULL;
}

/* Liberta a memória alocada por rtable_get_table().
 */
void rtable_free_entries(struct entry_t **entries){
    if(entries == NULL)
        return;
    
    free(entries);
}
