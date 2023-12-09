/**
 * Grupo: SD21
 * Miguel Costa - fc54431
 * André Mendes - fc54453
 * Fábio Estanqueiro - fc54469
 */
#include "table_skel.h"
#include "sdmessage.pb-c.h"
#include "table.h"
#include "table-private.h"
#include "data.h"
#include "stats-private.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <zookeeper/zookeeper.h>

#include "client_stub-private.h"
#include "client_stub.h"

#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

struct table_t *table;
struct statistics *stats;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

zhandle_t *zh;
int is_connected;
char *zoo_path = "/kvstore";
typedef struct String_vector zoo_string;
static char *watcher_ctx = "ZooKeeper Data Watcher";

struct rtable_t *backup = NULL;
char *IPB;
char *IPP;
int server;

void my_watcher_func(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            is_connected = 1;
        }
        else
        {
            is_connected = 0;
        }
    }
}

static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
{
    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            /* Get the updated children and reset the watch */
            if (ZOK != zoo_wget_children(zh, zoo_path, child_watcher, watcher_ctx, children_list))
            {
                fprintf(stderr, "Error setting watch at %s!\n", zoo_path);
            }
            // TODO
            switch (children_list->count)
            {
            case 1:
                if (ZNONODE != zoo_exists(zh, "/kvstore/primary", 0, NULL))
                {
                    if (backup != NULL)
                    {
                        if ((rtable_disconnect(backup)) < 0)
                        {
                            exit(-1);
                        }
                        backup = NULL;
                    }
                    IPB = NULL;
                    // meter a null caso antes houvesse um backup;
                }
                else
                {
                    if (backup != NULL)
                    {
                        if ((rtable_disconnect(backup)) < 0)
                        {
                            exit(-1);
                        }
                        backup = NULL;
                    }
                    IPB = NULL;
                    int backupIPLen = 256;
                    char backupIP[256] = "";
                    if (ZOK != zoo_get(zh, "/kvstore/backup", 0, backupIP, &backupIPLen, NULL))
                    {
                        printf("ERRO A IR BUSCAR DATA BACKUP 1 FILHO\n");
                        exit(-1);
                    }
                    IPP = NULL;
                    if (ZOK != zoo_delete(zh, "/kvstore/backup", -1))
                    {
                        printf("DELETE DO BACKUP\n");
                        exit(-1);
                    }
                    char *watch_prim_path = "/kvstore/primary";
                    if (ZOK != zoo_create(zh, watch_prim_path, backupIP, strlen(backupIP), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                    {
                        fprintf(stderr, "Error creating znode from path %s!\n", watch_prim_path);
                        exit(-1);
                    }
                    server = 1;
                }
                break;
            case 2:
                if (server == 1)
                {
                    if (backup == NULL)
                    {
                        int backupIPLen = 256;
                        char backupIP[256] = "";
                        if (ZOK != zoo_get(zh, "/kvstore/backup", 0, backupIP, &backupIPLen, NULL))
                        {
                            printf("ERRO A IR BUSCAR DATA BACKUP 2 FILHOS\n");
                            exit(-1);
                        }
                        if ((backup = rtable_connect(backupIP)) == NULL)
                        {
                            exit(-1);
                        }
                        char **keys = table_get_keys(table);

                        int i = 0;
                        while (keys[i] != NULL)
                        {
                            struct data_t *data = table_get(table, keys[i]);
                            rtable_put(backup, entry_create(keys[i], data));
                            i++;
                        }
                        IPB = backupIP;
                    }
                }
                else
                {
                    int primaryIPLen = 256;
                    char primaryIP[256] = "";
                    if (ZOK != zoo_get(zh, "/kvstore/primary", 0, primaryIP, &primaryIPLen, NULL))
                    {
                        printf("ERRO A IR BUSCAR DATA BACKUP 2 FILHOS\n");
                        exit(-1);
                    }
                    IPP = primaryIP;
                }
                break;
            default:
                // DO NOTHING
                break;
                free(children_list);
            }
        }
    }
}

/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int table_skel_init(int n_lists, char *port, char *address)
{
    if (n_lists < 0)
    {
        return -1;
    }

    zh = zookeeper_init(address, my_watcher_func, 2000, 0, NULL, 0); // Ligação ao servidor do ZooKeeper
    if (zh == NULL)
    {
        perror("Erro a ligar ao servidor do ZooKeeper");
        return -1;
    }
    sleep(3);
    if (is_connected)
    {
        zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));

        if (ZNONODE == zoo_exists(zh, zoo_path, 0, NULL))
        {
            if (ZOK == zoo_create(zh, zoo_path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0))
            {
                fprintf(stderr, "%s created!\n", zoo_path);
            }
            else
            {
                fprintf(stderr, "Error Creating %s!\n", zoo_path);
                exit(EXIT_FAILURE);
            }
        }
        if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher, watcher_ctx, children_list))
        {
            fprintf(stderr, "Error setting watch at %s!\n", zoo_path);
        }

        if (children_list->count == 0)
        { // Caso seja o primeiro filho
            /********************************************/
            // Criar prefixo para o primary
            char node_path[50] = "";
            strcat(node_path, zoo_path);
            strcat(node_path, "/primary");
            /********************************************/
            char *IPBuffer = malloc(sizeof(char *) * 50);

            struct ifaddrs *ifaddr;
            struct sockaddr_in *address = malloc(sizeof(struct socksockaddr_in *));

            if (getifaddrs(&ifaddr) == -1)
            {
                perror("getifaddrs");
                exit(EXIT_FAILURE);
            }
            for (struct ifaddrs *ifa = ifaddr; ifa != NULL;
                 ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr->sa_family == AF_INET)
                {
                    address = (struct sockaddr_in *)ifa->ifa_addr;
                    break;
                }
            }

            // To convert an Internet network address into ASCII string
            IPBuffer = inet_ntoa(address->sin_addr);

            /*---------------concatenar ip com port do servidor---------------*/
            char *ip_port = IPBuffer;
            strcat(ip_port, ":");
            strcat(ip_port, port);
            /*----------------------------------------------------------------*/
            if (ZOK != zoo_create(zh, node_path, ip_port, strlen(ip_port), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
            {
                fprintf(stderr, "Error creating znode from path %s!\n", node_path);
                return -1;
            }
            IPP = ip_port;
            IPB = NULL;
            server = 1;
        }
        else if (children_list->count == 1)
        { // Caso seja o 2 filho

            char node_path[50] = "";
            strcat(node_path, zoo_path);
            strcat(node_path, "/primary");
            if (ZNONODE != zoo_exists(zh, node_path, 0, NULL))
            {
                // Criar prefixo para o backup
                char backup_path[50] = "";
                strcat(backup_path, zoo_path);
                strcat(backup_path, "/backup");
                /********************************************/
                char *IPBuffer;

                struct ifaddrs *ifaddr;
                struct sockaddr_in *address = malloc(sizeof(struct socksockaddr_in *));

                if (getifaddrs(&ifaddr) == -1)
                {
                    perror("getifaddrs");
                    exit(EXIT_FAILURE);
                }
                for (struct ifaddrs *ifa = ifaddr; ifa != NULL;
                     ifa = ifa->ifa_next)
                {
                    if (ifa->ifa_addr->sa_family == AF_INET)
                    {
                        address = (struct sockaddr_in *)ifa->ifa_addr;
                        break;
                    }
                }

                // To convert an Internet network address into ASCII string
                IPBuffer = inet_ntoa(address->sin_addr);

                /*---------------concatenar ip com port do servidor---------------*/
                char *ip_port = IPBuffer;
                strcat(ip_port, ":");
                strcat(ip_port, port);
                /*----------------------------------------------------------------*/
                if (ZOK != zoo_create(zh, backup_path, ip_port, strlen(ip_port), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                {
                    fprintf(stderr, "Error creating znode from path %s!\n", backup_path);
                    return -1;
                }

                int primaryIPLen = 256;
                char primaryIP[256] = "";
                if (ZOK != zoo_get(zh, "/kvstore/primary", 0, primaryIP, &primaryIPLen, NULL))
                {
                    printf("ERRO A IR BUSCAR DATA BACKUP 2 FILHOS\n");
                    exit(-1);
                }
                IPP = primaryIP;
                IPB = ip_port;
                server = 2;
            }
            else
            {
                while (ZNONODE != zoo_exists(zh, node_path, 0, NULL))
                {
                    printf("Não existe servidor Primário\n");
                    sleep(5);
                }
            }
        }
        else
        { // Caso seja o 3 filho
            printf("Demasiados servers");
            return -1;
        }
        free(children_list->data);
        free(children_list);
    }

    table = table_create(n_lists);
    stats = malloc(sizeof(struct statistics));
    stats->n_size = 0;
    stats->n_del = 0;
    stats->n_get = 0;
    stats->n_put = 0;
    stats->n_getkeys = 0;
    stats->n_print = 0;
    stats->n_stats = 0;
    stats->time = 0;

    return 0;
}

/* Liberta toda a memória e recursos alocados pela função table_skel_init.
 */
void table_skel_destroy()
{
    pthread_mutex_lock(&mut);
    free(stats);
    table_destroy(table);
    zookeeper_close(zh);
    if (backup != NULL)
    {
        rtable_disconnect(backup);
    }
    pthread_mutex_unlock(&mut);
}

/* Executa uma operação na tabela (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, tabela nao incializada)
 */
int invoke(MessageT *msg)
{
    if (msg == NULL || table == NULL)
    {
        return -1;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_PUT && msg->c_type == MESSAGE_T__C_TYPE__CT_ENTRY)
    {
        struct data_t *data = data_create2(msg->data_size, msg->data);
        pthread_mutex_lock(&mut);
        if (backup != NULL)
        {
            struct entry_t *e = entry_create(strdup(msg->key), data_dup(data));
            if (rtable_put(backup, e) < 0)
            {
                return -1;
            }
        }
        else
        {
            if (server == 1)
            {
                return -1;
            }
        }
        int i = table_put(table, msg->key, data);
        stats->n_put += 1;
        pthread_mutex_unlock(&mut);
        free(data);
        if (i == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        else
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_GET && msg->c_type == MESSAGE_T__C_TYPE__CT_KEY)
    {
        pthread_mutex_lock(&mut);
        struct data_t *data_aux = table_get(table, msg->key);
        stats->n_get += 1;
        pthread_mutex_unlock(&mut);
        if (data_aux == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
            msg->data = NULL;
            msg->data_size = 0;
            data_destroy(data_aux);
            return 0;
        }
        else if (data_aux != NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
            msg->data = data_aux->data;
            msg->data_size = data_aux->datasize;
            free(data_aux);
            return 0;
        }
        else
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            data_destroy(data_aux);
            return -1;
        }
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_DEL && msg->c_type == MESSAGE_T__C_TYPE__CT_KEY)
    {
        pthread_mutex_lock(&mut);
        if (backup != NULL)
        {
            if (rtable_del(backup, strdup(msg->key)) < 0)
            {
                return -1;
            }
        }
        else
        {
            if (server == 1)
            {
                return -1;
            }
        }

        int i = table_del(table, msg->key);
        stats->n_del += 1;
        pthread_mutex_unlock(&mut);
        if (i == -1)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        else
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_SIZE && msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        pthread_mutex_lock(&mut);
        int i = table_size(table);
        stats->n_size += 1;
        pthread_mutex_unlock(&mut);

        msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->table_elems = i;
        return 0;
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_GETKEYS && msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        pthread_mutex_lock(&mut);
        int i = table_size(table);
        msg->keys = table_get_keys(table);
        stats->n_getkeys += 1;
        pthread_mutex_unlock(&mut);

        msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
        msg->n_keys = i;
        msg->table_elems = i;

        return 0;
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_PRINT && msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        pthread_mutex_lock(&mut);
        int table_size = table_space(table);
        char *copy = malloc(table_size);
        copy = table_string(table);
        stats->n_print += 1;

        msg->opcode = MESSAGE_T__OPCODE__OP_PRINT + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_TABLE;
        table_print(table);
        pthread_mutex_unlock(&mut);

        msg->table_string = copy;

        return 0;
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_STATS && msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        pthread_mutex_lock(&mut);
        msg->n_size = stats->n_size;
        msg->n_del = stats->n_del;
        msg->n_get = stats->n_get;
        msg->n_put = stats->n_put;
        msg->n_getkeys = stats->n_getkeys;
        msg->n_print = stats->n_print;

        msg->opcode = MESSAGE_T__OPCODE__OP_STATS + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        pthread_mutex_unlock(&mut);
        return 0;
    }

    return -1;
}